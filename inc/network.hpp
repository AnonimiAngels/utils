#pragma once
#ifndef NETWORK_HPP
	#define NETWORK_HPP

	#include <algorithm>
	#include <cstdint>
	#include <cstring>
	#include <string>
	#include <vector>

	#include <ifaddrs.h>
	#include <linux/if_packet.h>
	#include <net/if.h>
	#include <sys/socket.h>
	#include <unistd.h>

namespace utils
{
	namespace network
	{
		struct interface_info
		{
			std::string name;
			std::string mac_address;
			std::int32_t interface_index;
			std::int32_t interface_type;
			std::uint32_t flags;
			std::int32_t permanence_score;
		};

		namespace network_constants
		{
			static constexpr std::int32_t k_mac_address_length = 6;
			static constexpr std::int32_t k_hex_chars_per_byte = 2;
			static constexpr std::int32_t k_mac_string_length  = (k_mac_address_length * k_hex_chars_per_byte) + (k_mac_address_length - 1);
		}	 // namespace network_constants

		class network_utils
		{
		private:
			static auto is_valid_mac_address(const std::uint8_t* p_mac_bytes) -> bool
			{
				// Check if MAC is all zeros
				bool all_zeros = true;
				for (std::int32_t idx_for = 0; idx_for < network_constants::k_mac_address_length; ++idx_for)
				{
					if (p_mac_bytes[idx_for] != 0)
					{
						all_zeros = false;
						break;
					}
				}
				if (all_zeros)
				{
					return false;
				}

				// Check if MAC is multicast (first bit of first byte is 1)
				if ((p_mac_bytes[0] & 0x01) != 0)
				{
					return false;
				}

				return true;
			}

			static auto format_mac_address(const std::uint8_t* p_mac_bytes) -> std::string
			{
				std::string result;
				result.reserve(network_constants::k_mac_string_length);

				for (std::int32_t idx_for = 0; idx_for < network_constants::k_mac_address_length; ++idx_for)
				{
					if (idx_for > 0)
					{
						result += ':';
					}

					const std::uint8_t byte_value  = p_mac_bytes[idx_for];
					const std::uint8_t high_nibble = (byte_value >> 4) & 0x0F;
					const std::uint8_t low_nibble  = byte_value & 0x0F;

					result += static_cast<char>(high_nibble < 10 ? '0' + high_nibble : 'A' + high_nibble - 10);
					result += static_cast<char>(low_nibble < 10 ? '0' + low_nibble : 'A' + low_nibble - 10);
				}

				return result;
			}

			static auto is_virtual_interface(const std::string& p_interface_name) -> bool
			{
				// Common virtual interface prefixes/names
				static const std::vector<std::string> virtual_prefixes = {
					"lo",		  // loopback
					"veth",		  // virtual ethernet
					"docker",	  // docker interfaces
					"br-",		  // bridge interfaces
					"virbr",	  // virtual bridge
					"vmnet",	  // vmware interfaces
					"vbox",		  // virtualbox interfaces
					"tun",		  // tunnel interfaces
					"tap",		  // tap interfaces
					"ppp",		  // point-to-point
					"wwan",		  // wireless wan (often USB modems)
					"dummy",	  // dummy interfaces
					"sit",		  // ipv6 in ipv4 tunnels
					"teql",		  // traffic equalizer
					"ifb",		  // intermediate functional block
					"macvlan",	  // mac vlan
					"macvtap",	  // mac vtap
					"vcan",		  // virtual can
					"vxcan",	  // virtual crossover can
					"nlmon",	  // netlink monitor
					"bond",		  // bonding interfaces
					"team"		  // team interfaces
				};

				for (const auto& prefix : virtual_prefixes)
				{
					if (p_interface_name.find(prefix) == 0)
					{
						return true;
					}
				}

				return false;
			}

			static auto calculate_permanence_score(const interface_info& p_interface) -> std::int32_t
			{
				std::int32_t score = 0;

				// Higher score = more permanent

				// Prefer non-virtual interfaces
				if (!is_virtual_interface(p_interface.name))
				{
					score += 100;
				}

				// Prefer ethernet interfaces over wireless
				if (p_interface.name.find("eth") == 0 || p_interface.name.find("enp") == 0 || p_interface.name.find("eno") == 0)
				{
					score += 50;
				}

				// Wireless interfaces get lower score
				if (p_interface.name.find("wlan") == 0 || p_interface.name.find("wlp") == 0 || p_interface.name.find("wifi") == 0)
				{
					score += 20;
				}

				// Prefer interfaces with lower indices (often built-in)
				if (p_interface.interface_index > 0)
				{
					score += std::max(0, 20 - p_interface.interface_index);
				}

				// Prefer interfaces that are up
				if ((p_interface.flags & IFF_UP) != 0)
				{
					score += 10;
				}

				// Prefer interfaces that support broadcast
				if ((p_interface.flags & IFF_BROADCAST) != 0)
				{
					score += 5;
				}

				// Prefer interfaces that are not point-to-point
				if ((p_interface.flags & IFF_POINTOPOINT) == 0)
				{
					score += 5;
				}

				return score;
			}

		public:
			static auto get_all_network_interfaces() -> std::vector<interface_info>
			{
				std::vector<interface_info> interfaces;

				struct ifaddrs* ifaddr = nullptr;
				if (getifaddrs(&ifaddr) == -1)
				{
					return interfaces;
				}

				for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
				{
					if (ifa->ifa_addr == nullptr)
					{
						continue;
					}

					// We only care about packet socket addresses (AF_PACKET) for MAC addresses
					if (ifa->ifa_addr->sa_family != AF_PACKET)
					{
						continue;
					}

					auto* sll = reinterpret_cast<struct sockaddr_ll*>(ifa->ifa_addr);

					// Skip if no hardware address
					if (sll->sll_halen != network_constants::k_mac_address_length)
					{
						continue;
					}

					// Check if MAC address is valid
					if (!is_valid_mac_address(sll->sll_addr))
					{
						continue;
					}

					interface_info info;
					info.name			  = ifa->ifa_name;
					info.mac_address	  = format_mac_address(sll->sll_addr);
					info.interface_index  = static_cast<std::int32_t>(sll->sll_ifindex);
					info.interface_type	  = static_cast<std::int32_t>(sll->sll_hatype);
					info.flags			  = ifa->ifa_flags;
					info.permanence_score = calculate_permanence_score(info);

					interfaces.push_back(info);
				}

				freeifaddrs(ifaddr);

				// Sort by permanence score (highest first)
				std::sort(interfaces.begin(),
						  interfaces.end(),
						  [](const interface_info& p_left, const interface_info& p_right) -> bool { return p_left.permanence_score > p_right.permanence_score; });

				return interfaces;
			}

			static auto get_permanent_mac_address() -> std::string
			{
				auto interfaces = get_all_network_interfaces();

				if (interfaces.empty())
				{
					return "";
				}

				// Return the MAC address of the most permanent interface
				return interfaces[0].mac_address;
			}

			static auto get_best_interface_info() -> interface_info
			{
				auto interfaces = get_all_network_interfaces();

				if (interfaces.empty())
				{
					return {};
				}

				return interfaces[0];
			}
		};
	}	 // namespace network
}	 // namespace utils

#endif	  // NETWORK_HPP