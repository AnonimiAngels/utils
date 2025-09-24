#!/usr/bin/env python3

# File: cmake/mng_impl.py

import argparse
import concurrent.futures
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path
from threading import Lock
from typing import Dict, Optional, Tuple


class logger:
	"""Simple logger"""

	def __init__(self, p_output=sys.stdout):
		self.m_output = p_output
		self.m_lock = Lock()

	def log(self, p_message: str, p_flush: bool = True) -> None:
		"""Thread-safe logging"""
		with self.m_lock:
			self.m_output.write(f"-- {p_message}\n")
			if p_flush:
				self.m_output.flush()

	def info(self, p_message: str) -> None:
		"""Log info message"""
		self.log(f"INFO:{p_message}")

	def error(self, p_message: str) -> None:
		"""Log error message"""
		self.log(f"ERROR:{p_message}")

	def success(self, p_message: str) -> None:
		"""Log success message"""
		self.log(f"SUCCESS:{p_message}")

	def status(self, p_status: str, p_name: str) -> None:
		"""Log status message in cmake format"""
		self.log(f"{p_status}:{p_name}")


# Global logger instance
g_logger = logger()


class download_helper:
	"""Optimized download utilities with progress reporting and retry logic"""

	@staticmethod
	def download_with_retry(p_url: str, p_dest: Path, p_retries: int = 3) -> bool:
		"""Download file with retry logic and progress reporting"""
		for attempt in range(p_retries):
			try:
				g_logger.info(f"Downloading from: {p_url}")

				# Create custom opener with timeout and better headers
				opener = urllib.request.build_opener()
				opener.addheaders = [
					('User-Agent', 'Package-Manager/1.0'),
					('Accept-Encoding', 'gzip, deflate')
				]
				urllib.request.install_opener(opener)

				# Get file size for progress reporting
				with urllib.request.urlopen(p_url, timeout=30) as response:
					file_size = int(response.headers.get('Content-Length', 0))

					if file_size > 0:
						g_logger.info(f"File size: {file_size / (1024*1024):.2f} MB")

					# Download with chunk reading for better memory usage
					chunk_size = 8192 * 16  # 128KB chunks
					downloaded = 0
					last_percent = -1

					with open(p_dest, 'wb') as f:
						while True:
							chunk = response.read(chunk_size)
							if not chunk:
								break
							f.write(chunk)
							downloaded += len(chunk)

							# Progress reporting
							if file_size > 0:
								progress = int((downloaded / file_size) * 100)
								if progress != last_percent and progress % 10 == 0:
									g_logger.info(f"Download progress: {progress}%")
									last_percent = progress

				g_logger.info("Download completed successfully")
				return True

			except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError) as e:
				g_logger.error(f"Download attempt {attempt + 1} failed: {e}")
				if attempt < p_retries - 1:
					wait_time = 2 ** attempt
					g_logger.info(f"Retrying in {wait_time} seconds...")
					time.sleep(wait_time)
					continue
				raise Exception(f"Failed to download after {p_retries} attempts: {e}")

		return False


class package_cache:
	"""Optimized cache management with file hashing and validation"""

	def __init__(self, p_cache_dir: Path):
		self.m_cache_dir = p_cache_dir
		self.m_cache_lock = Lock()
		self.m_metadata_cache = {}

	def get_package_hash(self, p_info: dict) -> str:
		"""Generate hash for package configuration"""
		hash_data = json.dumps(p_info, sort_keys=True)
		return hashlib.sha256(hash_data.encode()).hexdigest()[:16]

	def is_cache_valid(self, p_name: str, p_info: dict, p_package_dir: Path) -> bool:
		"""Fast cache validation with metadata checking"""
		if not p_package_dir.exists():
			return False

		cache_file = self.m_cache_dir / p_name / "CACHE" / ".cache"
		if not cache_file.exists():
			return False

		with self.m_cache_lock:
			# Use in-memory cache if available
			if p_name in self.m_metadata_cache:
				cached_info = self.m_metadata_cache[p_name]
			else:
				with open(cache_file, 'r') as f:
					cached_info = json.load(f)
					self.m_metadata_cache[p_name] = cached_info

		# Quick hash comparison
		return self.get_package_hash(cached_info) == self.get_package_hash(p_info)


class git_helper:
	"""Optimized git operations with full recursive clones"""

	@staticmethod
	def full_clone(p_repo: str, p_dest: Path, p_tag: Optional[str] = None) -> bool:
		"""Perform full recursive clone"""
		g_logger.info(f"Cloning repository: {p_repo}")
		if p_tag:
			g_logger.info(f"Using tag/branch: {p_tag}")

		cmd = ['git', 'clone']

		if p_tag:
			cmd.extend(['--branch', p_tag])

		# Add recursive options without shallow
		cmd.extend([
			'--recurse-submodules',
			'--quiet',
			'--shallow-submodules',
			'--single-branch',
			'--depth', '1',
			p_repo,
			str(p_dest)
		])

		# Set git config for better performance
		env = os.environ.copy()
		env['GIT_HTTP_LOW_SPEED_LIMIT'] = '1000'
		env['GIT_HTTP_LOW_SPEED_TIME'] = '10'

		g_logger.info(f"Executing {' '.join(cmd)}")
		result = subprocess.run(cmd, capture_output=True, text=True, env=env, timeout=600)

		if result.returncode == 0:
			g_logger.info("Repository cloned successfully")
			return True
		else:
			g_logger.error(f"Clone failed: {result.stderr}")
			return False

	@staticmethod
	def update_full(p_dest: Path) -> bool:
		"""Update repository with full history"""
		try:
			g_logger.info(f"Updating repository at: {p_dest}")

			# Fetch all changes
			g_logger.info("Fetching all changes...")
			subprocess.run(
				['git', 'fetch', '--all', '--quiet'],
				cwd=p_dest, check=True, timeout=120
			)

			# Reset to latest
			g_logger.info("Resetting to latest...")
			subprocess.run(
				['git', 'reset', '--hard', 'origin/HEAD', '--quiet'],
				cwd=p_dest, check=True, timeout=30
			)

			# Update submodules recursively
			g_logger.info("Updating submodules...")
			subprocess.run(
				['git', 'submodule', 'update', '--init', '--recursive', '--quiet'],
				cwd=p_dest, check=False, timeout=120
			)

			g_logger.info("Repository updated successfully")
			return True
		except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
			g_logger.error(f"Update failed: {e}")
			return False


class package_manager:
	def __init__(self, p_cache_dir):
		self.m_cache_dir = Path(p_cache_dir)
		self.m_cache_dir.mkdir(parents=True, exist_ok=True)
		self.m_cache = package_cache(self.m_cache_dir)
		self.m_download_helper = download_helper()
		self.m_git_helper = git_helper()

		# Thread pool for parallel operations
		self.m_executor = concurrent.futures.ThreadPoolExecutor(max_workers=4)

	def get_cache_file(self, p_name) -> Path:
		cache_dir = self.m_cache_dir / p_name / "CACHE"
		cache_dir.mkdir(parents=True, exist_ok=True)
		return cache_dir / ".cache"

	def get_package_dir(self, p_name) -> Path:
		return self.m_cache_dir / p_name / p_name

	def load_cached_info(self, p_name) -> dict:
		cache_file = self.get_cache_file(p_name)
		if cache_file.exists():
			with open(cache_file, 'r') as f:
				return json.load(f)
		return {}

	def save_cached_info(self, p_name, p_info):
		cache_file = self.get_cache_file(p_name)
		with open(cache_file, 'w') as f:
			json.dump(p_info, f, indent=2)

	def needs_refetch(self, p_name, p_new_info) -> bool:
		"""Optimized cache checking using hashes"""
		package_dir = self.get_package_dir(p_name)
		return not self.m_cache.is_cache_valid(p_name, p_new_info, package_dir)

	def clear_package(self, p_name):
		g_logger.info(f"Clearing package cache for: {p_name}")
		package_parent = self.m_cache_dir / p_name
		if package_parent.exists():
			shutil.rmtree(package_parent)
			# Clear from memory cache
			if p_name in self.m_cache.m_metadata_cache:
				del self.m_cache.m_metadata_cache[p_name]
			g_logger.info(f"Package {p_name} cleared successfully")
		else:
			g_logger.info(f"Package {p_name} not found in cache")

	def clone_repo(self, p_name, p_repo, p_tag=None):
		"""Repository cloning with full recursive clone"""
		package_dir = self.get_package_dir(p_name)
		package_dir.parent.mkdir(parents=True, exist_ok=True)

		g_logger.info(f"Starting clone for package: {p_name}")

		# Perform full recursive clone
		if not self.m_git_helper.full_clone(p_repo, package_dir, p_tag):
			raise Exception(f"Failed to clone {p_name} from {p_repo}")

	def update_repo(self, p_name):
		"""Repository update with full history"""
		package_dir = self.get_package_dir(p_name)
		if not package_dir.exists():
			return False

		return self.m_git_helper.update_full(package_dir)

	def download_archive(self, p_name, p_url):
		"""Optimized archive download with better extraction"""
		package_dir = self.get_package_dir(p_name)
		package_dir.parent.mkdir(parents=True, exist_ok=True)

		g_logger.info(f"Starting archive download for package: {p_name}")
		download_file = self.m_cache_dir / f"{p_name}_download"

		# Download with retry and progress
		self.m_download_helper.download_with_retry(p_url, download_file)

		try:
			g_logger.info(f"Extracting archive for {p_name}...")

			# Optimized extraction based on file type
			if p_url.endswith('.zip'):
				g_logger.info("Detected ZIP archive")
				with zipfile.ZipFile(download_file, 'r') as zip_ref:
					# Extract directly to target if possible
					members = zip_ref.namelist()
					g_logger.info(f"Archive contains {len(members)} files")
					common_prefix = os.path.commonpath(members) if members else ''

					if common_prefix and '/' in common_prefix:
						g_logger.info(f"Stripping common prefix: {common_prefix}")
						# Extract with path stripping
						for idx, member in enumerate(members):
							if idx % 100 == 0 and idx > 0:
								g_logger.info(f"Extracted {idx}/{len(members)} files...")
							if member.startswith(common_prefix):
								target = package_dir / member[len(common_prefix)+1:]
								if member.endswith('/'):
									target.mkdir(parents=True, exist_ok=True)
								else:
									target.parent.mkdir(parents=True, exist_ok=True)
									with zip_ref.open(member) as source, open(target, 'wb') as dest:
										shutil.copyfileobj(source, dest)
					else:
						zip_ref.extractall(package_dir)
			else:
				g_logger.info("Detected TAR archive")
				# Optimized tar extraction
				with tarfile.open(download_file, 'r:*') as tar_ref:
					members = tar_ref.getmembers()
					g_logger.info(f"Archive contains {len(members)} files")
					common_prefix = os.path.commonpath([m.name for m in members]) if members else ''

					if common_prefix and '/' in common_prefix:
						g_logger.info(f"Stripping common prefix: {common_prefix}")
						# Extract with path stripping
						for idx, member in enumerate(members):
							if idx % 100 == 0 and idx > 0:
								g_logger.info(f"Extracted {idx}/{len(members)} files...")
							if member.name.startswith(common_prefix):
								member.name = member.name[len(common_prefix)+1:]
								if member.name:
									tar_ref.extract(member, package_dir)
					else:
						tar_ref.extractall(package_dir)

			g_logger.info(f"Extraction completed for {p_name}")
		finally:
			download_file.unlink(missing_ok=True)
			g_logger.info("Cleaned up temporary download file")

	def process_package(self, p_args):
		name = p_args.name
		package_dir = self.get_package_dir(name)

		g_logger.info(f"Processing package: {name}")

		# Don't include options in cache info - they're for CMake only
		new_info = {
			'version': p_args.version or '',
			'git_tag': p_args.git_tag or '',
			'github_repository': p_args.github_repository or '',
			'git_repository': p_args.git_repository or '',
			'url': p_args.url or ''
			# 'options' removed - not part of cache identity
		}

		if p_args.version:
			g_logger.info(f"Version: {p_args.version}")
		if p_args.git_tag:
			g_logger.info(f"Git tag: {p_args.git_tag}")

		# Quick cache validation
		g_logger.info("Checking cache validity...")
		if package_dir.exists() and self.needs_refetch(name, new_info):
			g_logger.status("REFETCH", name)
			self.clear_package(name)

		if package_dir.exists():
			if p_args.keep_updated and (p_args.git_repository or p_args.github_repository):
				g_logger.status("UPDATE", name)
				g_logger.info("Attempting to update existing package...")
				if not self.update_repo(name):
					# If update fails, refetch
					g_logger.info("Update failed, clearing cache for refetch...")
					self.clear_package(name)
				else:
					g_logger.status("EXISTS", str(package_dir))
					return
			else:
				g_logger.status("CACHED", name)
				g_logger.info("Using cached package")
				g_logger.status("EXISTS", str(package_dir))
				return

		# Download if not cached
		g_logger.info("Package not in cache, downloading...")
		if p_args.github_repository:
			git_repo = f"https://github.com/{p_args.github_repository}.git"
			g_logger.info(f"Using GitHub repository: {p_args.github_repository}")
		else:
			git_repo = p_args.git_repository
			if git_repo:
				g_logger.info(f"Using git repository: {git_repo}")

		if git_repo:
			git_tag = p_args.git_tag or (f"v{p_args.version}" if p_args.version else None)
			self.clone_repo(name, git_repo, git_tag)
		elif p_args.url:
			g_logger.info(f"Using URL: {p_args.url}")
			self.download_archive(name, p_args.url)
		else:
			raise Exception(f"No source specified for {name}")

		g_logger.info("Saving package metadata...")
		self.save_cached_info(name, new_info)
		g_logger.success(str(package_dir))

	def __del__(self):
		"""Cleanup thread pool"""
		if hasattr(self, 'm_executor'):
			self.m_executor.shutdown(wait=False)


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--cache-dir', required=True)
	parser.add_argument('--name', required=True)
	parser.add_argument('--version')
	parser.add_argument('--git-tag')
	parser.add_argument('--github-repository')
	parser.add_argument('--git-repository')
	parser.add_argument('--url')
	parser.add_argument('--keep-updated', action='store_true')
	parser.add_argument('--download-only', action='store_true')
	parser.add_argument('--options', nargs='*')
	parser.add_argument('--clear-cache', action='store_true')
	parser.add_argument('--clear-package')

	args = parser.parse_args()

	manager = package_manager(args.cache_dir)

	if args.clear_cache:
		if args.clear_package:
			manager.clear_package(args.clear_package)
		else:
			shutil.rmtree(args.cache_dir, ignore_errors=True)
			Path(args.cache_dir).mkdir(parents=True, exist_ok=True)
		g_logger.info("CLEARED")
		return

	manager.process_package(args)


if __name__ == '__main__':
	main()