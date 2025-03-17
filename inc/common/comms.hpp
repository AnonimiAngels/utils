#pragma once
#ifndef COMMUNICATIONS_HPP
#define COMMUNICATIONS_HPP

/**
 * @file communications.hpp
 * @author anonimi_angels (gutsulyak.vladyslav2000@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-11-13
 *
 * @copyright
 *The MIT License (MIT)
 *Copyright (c) 2024 anonimi_angels
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 *
 */

#include <cstddef>
#include <string>
#include <functional>
#include <memory>
#include <filesystem>

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netinet/in.h>

#include "loggers.hpp"

/**
 * @brief Class to handle SSH processes, such as executing commands, pulling and pushing files, creating connections, etc.
 * Uses libssh2 for the SSH connection and SFTP for file transfer
 */
class ssh_process
{
  public:
	/**
	 * @brief Construct a new ssh process object
	 *
	 */
	ssh_process();

	/**
	 * @brief Destroy the ssh process object
	 *
	 */
	~ssh_process() { close_ssh_connection(); }

	/**
	 * @brief Execute a command
	 *
	 * @param cmd Command to execute
	 * @param exec_res Return code
	 * @param exec_out Output
	 */
	void exec(const std::string& cmd);

	/**
	 * @brief Set the ip object
	 *
	 * @param in_ip IP address
	 */
	void set_ip(const std::string& in_ip) { m_ip = in_ip; }

	/**
	 * @brief Set the user object
	 *
	 * @param in_user Username
	 */
	void set_user(const std::string& in_user) { m_user = in_user; }

	/**
	 * @brief Set the pass object
	 *
	 * @param in_pass Password
	 */
	void set_pass(const std::string& in_pass) { m_pass = in_pass; }

	/**
	 * @brief Pull a file from the remote server at the specified path and check the SHA256 hash
	 *
	 * @param path_local Local path to save the file
	 * @param path_remote Remote path to the file
	 * @param check_sha Check the SHA256 hash
	 *
	 * @note If check_sha is true, the function will check the SHA256 hash of the file throwing an error if it does not match
	 */
	auto pull_file(const std::string& path_local, const std::string& path_remote, const bool& check_sha = false) -> void;

	/**
	 * @brief Push a file to the remote server at the specified path and check the SHA256 hash
	 *
	 * @param path_local Local path to the file
	 * @param path_remote Remote path to the file
	 * @param check_sha Check the SHA256 hash

	 * @note If check_sha is true, the function will check the SHA256 hash of the file throwing an error if it does not match
	 */
	auto push_file(const std::string& path_local, const std::string& path_remote, const bool& check_sha = false) -> void;

	/**
	 * @brief Close the SSH connection and free resources
	 */
	void close_ssh_connection();

	/**
	 * @brief Open an SSH connection
	 */
	void open_ssh_connection();

	/**
	 * @brief Set the env object
	 *
	 * @param env Name of the environment variable
	 * @param val Value of the environment variable
	 */
	auto set_env(const std::string& env, const std::string& val) -> void;

	/**
	 * @brief Get the state of the SSH connection
	 *	0: Not connected
	 *	1: Connected
	 * @return const uint8_t&
	 */
	[[nodiscard]] auto get_state() const -> bool { return m_state; }

	/**
	 * @brief Get the env object
	 *
	 * @param env Name of the environment variable
	 * @return std::string Value of the environment variable
	 */
	[[nodiscard]] auto get_env(const std::string& env) -> std::string;

	/**
	 * @brief Get the bash env object
	 *
	 * @param env Name of the environment variable
	 * @return std::string Value of the environment variable
	 */
	[[nodiscard]] auto get_bash_env(const std::string& env) -> std::string;

	/**
	 * @brief Get the output object
	 *
	 * @return std::string
	 */
	[[nodiscard]] auto get_output() const -> std::string { return m_buffer; }

	/**
	 * @brief Get the return code object
	 *
	 * @return int32_t
	 */
	[[nodiscard]] auto get_rc() const -> int32_t { return m_return_code; }

	/**
	 * @brief Function used as callback for logging
	 *
	 * @param callback Callback function
	 */
	auto set_log_callback(const std::function<void(const std::string&)>& callback) -> void { m_log_callback = callback; }

	/**
	 * @brief Function used to move to a given directory
	 *
	 * @param in_path Path to the directory
	 */
	auto move_to_dir(std::string_view&& in_path) -> void;

	enum class action_id : std::uint8_t
	{
		CHECK_FILE = 0,
		CHECK_DIR,
		CREATE_DIR
	};
	auto check_existence(std::filesystem::path in_path, action_id in_action) -> bool;

  private:
	auto while_eagain(std::function<std::int32_t()> in_func) -> std::int32_t;
	template <typename out_type_t> auto while_last_err_eagain(std::function<out_type_t()> in_func) -> out_type_t
	{
		out_type_t out = {};

		while (true)
		{
			out = in_func();

			if (out != out_type_t{}) { break; }

			if (libssh2_session_last_errno(m_ssh_session) == LIBSSH2_ERROR_EAGAIN) { wait_socket(); }
			else { parse_error_code(); }
		}

		return out;
	}

	/**
	 * @brief Connect to the remote host
	 *
	 */
	auto connect_to_rhost() -> void;

	/**
	 * @brief Parse the last error code and throw an error
	 *
	 */
	auto parse_error_code() -> void;

	/**
	 * @brief Send a command to the remote server
	 */
	void send_ssh_command(const std::string& cmd);

	auto set_channel_env() -> void;

	/**
	 * @brief Check the SHA256 hash of a file
	 *
	 * @param path_local Local path to the file
	 * @param path_remote Remote path to the file
	 * @return true If the SHA256 hash matches
	 * @return false If the SHA256 hash does not match
	 */
	[[nodiscard]] auto check_sha(const std::string& path_local, const std::string& path_remote) -> bool;

	/**
	 * @brief Get the file from the remote server at the specified path to the local path
	 *
	 * @param local_path Location to save the file
	 * @param remote_path Path to the file on the remote server
	 */
	void request_sftp_file(const std::string& path_local, const std::string& path_remote);

	/**
	 * @brief Put the file to the remote server at the specified path
	 *
	 * @param local_path Local path to the file
	 * @param remote_path Remote path to the file
	 */
	void send_sftp_file(const std::string& path_local, const std::string& path_remote);

	/**
	 * @brief Open an SSH channel
	 */
	void open_ssh_channel();

	/**
	 * @brief Close the SSH channel
	 *
	 * @return true If the channel was closed
	 */
	auto close_ssh_channel() -> bool;

	/**
	 * @brief Open an SFTP session
	 *
	 * @note Will close the current session if open and open a new one
	 */
	void open_sftp_session();

	/**
	 * @brief Creates a new SFTP handle for reading a file
	 *
	 * @param path Path to the file to read
	 */
	void open_sftp_handle_read(const std::string& path);

	/**
	 * @brief Creates a new SFTP handle for writing a file
	 *
	 * @param path Path to the file to write
	 */
	void open_sftp_handle_write(const std::string& path);

	/**
	 * @brief Default SFTP handle creation
	 *
	 * @param path Path to the file
	 * @param flags Open mode flags
	 * @param mode File permissions
	 */
	void open_sftp_handle(const std::string& path, const int32_t& flags, const int32_t& mode);

	/**
	 * @brief Close the SFTP handle
	 */
	void close_sftp_handle();

	/**
	 * @brief Wait for the socket to be ready
	 *
	 * @return int
	 */
	auto wait_socket() -> int32_t;

	/**
	 * @brief Normalize path
	 */
	static auto normalize_path(const std::string& path) -> std::string;

	/**
	 * @brief Throw an error based on the last error code
	 *
	 */
	auto throw_error() -> void;

	/**
	 * @brief Remote IP address
	 *
	 */
	std::string m_ip;

	/**
	 * @brief Remote username
	 *
	 */
	std::string m_user;

	/**
	 * @brief Remote password
	 *
	 */
	std::string m_pass;

	/**
	 * @brief Buffer for SSH output
	 *
	 */
	std::string m_buffer;

	/**
	 * @brief SSH host address
	 *
	 */
	uint32_t m_ssh_hostaddr;

	/**
	 * @brief SSH socket
	 *
	 */
	libssh2_socket_t m_ssh_socket;

	/**
	 * @brief Remote socket
	 *
	 */
	struct sockaddr_in m_ssh_sin;

	/**
	 * @brief SSH fingerprint
	 *
	 */
	std::string m_ssh_fingerprint;

	/**
	 * @brief SSH return code
	 *
	 */
	int32_t m_ssh_rc;

	/**
	 * @brief Return code of the command
	 *
	 */
	int32_t m_return_code;

	/**
	 * @brief SSH session
	 *
	 */
	LIBSSH2_SESSION* m_ssh_session = nullptr;

	/**
	 * @brief SSH channel
	 *
	 */
	LIBSSH2_CHANNEL* m_ssh_channel = nullptr;

	/**
	 * @brief Known hosts
	 *
	 */
	LIBSSH2_KNOWNHOSTS* m_ssh_known_hosts = nullptr;

	/**
	 * @brief SFTP session
	 *
	 */
	LIBSSH2_SFTP* m_sftp_session = nullptr;

	/**
	 * @brief SFTP handle
	 *
	 */
	LIBSSH2_SFTP_HANDLE* m_sftp_handle = nullptr;

	/**
	 * @brief SFTP file info
	 *
	 */
	LIBSSH2_SFTP_ATTRIBUTES m_sftp_file_info = {0, 0, 0, 0, 0, 0, 0};

	/**
	 * @brief Block size for file transfer
	 *
	 */
	const size_t m_sftp_block_size = static_cast<const size_t>(64 * 1024);

	/**
	 * @brief State of the SSH connection
	 *
	 */
	bool m_state = false;

	std::unique_ptr<utils::logger> m_logger = nullptr;

	std::function<void(const std::string&)> m_log_callback = nullptr;
};

#endif // COMMUNICATIONS_HPP