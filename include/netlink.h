/**
 * @file netlink.h
 * @brief Control Area Network - Ethernet - Gateway - Netlink Header (Utility)
 * @author Fabian Raab (fabian.raab@tum.de)
 * @date May, 2013
 * @copyright GNU Public License v3 or higher
 * @ingroup files
 * @{
 */

/*****************************************************************************
 * (C) Copyright 2013 Fabian Raab, Stefan Smarzly
 *
 * This file is part of CAN-Eth-GW.
 *
 * CAN-Eth-GW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CAN-Eth-GW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CAN-Eth-GW.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef __CAN_ETH_GW_UTILS_NETLINK_H__
#define __CAN_ETH_GW_UTILS_NETLINK_H__

#include <stdint.h>

/** This Flags are also defind in kernel in ce_gw_dev.h */
#define F_CAN_FD 0x00000001

/**
 * @enum gw_type
 * @see This enum are also defind in kernel in ce_gw_main.h
 */
enum gw_type {
	TYPE_NONE,/**< No Type. Schould normally not used. */
	TYPE_ETH, /**< Convert CAN Header to Ethernet Header */
	TYPE_NET, /**< Copy complete CAN Frame into Ethernet payload.
			 * e.g use CAN als Network Layer (Layer 3) Protocol. */
	TYPE_TCP, /**< Convert CAN Header into an IP/TCP Package */
	TYPE_UDP, /**< Convert CAN Header into an IP/UDP Package */
	__TYPE_MAX, /**< Maximum Type Number + 1 */
};
#define TYPE_MAX (__TYPE_MAX - 1)

/**
 * @enum
 * @brief Generic Netlink Commands
 * @details Set int values as Identifiers for userspace application.
 */
enum {
	CE_GW_C_UNSPEC,/**< Only a Dummy to skip index 0. */
	CE_GW_C_ECHO,  /**< Sends a message back. Calls ce_gw_nl_echo(). */
	CE_GW_C_ADD,   /**< Add a gateway. Calls ce_gw_netlink_add(). */
	CE_GW_C_DEL,   /**< Delate a gateway. Calls ce_gw_netlink_del(). */
	CE_GW_C_LIST,  /**< list active gateways. Calls ce_gw_netlink_list(). */
	__CE_GW_C_MAX, /**< Maximum Number of Commands plus 1 */
};
#define CE_GW_C_MAX (__CE_GW_C_MAX - 1) /**< Maximum Number of Commands */

/**
 * @fn int ce_gw_add(char *src_name, char *dst_name, uint8_t type,
 *            uint32_t flags)
 * @brief add a virtual ethernet device or a route
 * @param dst_name The textual name of the device wich will be the dst. OR the
 *                 name of the device, if you want to add a device.
 * @param src_name The textual name of the device wich will be the src. Must be
 *                 NULL if you want do add a device.
 * @param type The Type of the route. For adding dev some settings according to
 *             the type will be set.
 * @param flags The Flags of the route. For adding dev some settings according to
 *             the type will be set. See netlink.h for the falgs.
 * @ingroup net
 * @see related callbacks: nl_cb_general_errno()
 */
extern int ce_gw_add(char *dst_name, char *src_name, uint8_t type,
                     uint32_t flags);

/**
 * @fn int ce_gw_del(uint32_t id, char *dev_name)
 * @brief Deletes a device or a route in Kernel Module
 * @param id The id of the route you want to delete. param dev_name must be
 *        NULL when you want to delete a route.
 * @param dev_name The textual name of the virtual device you want to delete.
 *        The device must be previously added by ce_gw_add(). param id must be
 *        0 if you want to delete a device.
 * @pre param id must be == 0 OR param dev_name must be == NULL
 * @retval 0 on success
 * @retval <0 on failure
 * @ingroup net
 * @see related callbacks: nl_cb_general_errno()
 */
extern int ce_gw_del(uint32_t id, char *dev_name);

/**
 * @fn int ce_gw_list(uint32_t id)
 * @brief Print informations of actual active routes to stdout
 * @param id set it to 0 if you want to list all routes. Else set it to the
 * route id you want to print
 *           of the route for wich you want the informations printed.
 * @retval 0 on success
 * @retval <0 on failure
 * @ingroup net
 * @see related callbacks: nl_cb_list_entry(), nl_cb_list_finish(),
 *                   nl_cb_general_errno()
 * @todo perhaps return a list with the information, so that the caller
 * could parse the data himself (for example he will search for something
 * or want to pint out in an other method than the standart)
 */
extern int ce_gw_list(uint32_t id);

/**
 * @fn int ce_gw_echo(char *message)
 * @brief send a message and return the received message from Kernel.
 * (for testing)
 * @param message A String message you want to send.
 * @ingroup net
 * @retval The message you get from the Kernel. NULL on failure.
 */
extern int ce_gw_echo(char *message);

/**
 * @fn int nl_sk_fam_init(void)
 * @brief creakte socket and family and connect
 * @details     It creates the netlink socket and connect, as well as the
 *              netlink family. This function must called once before any other
 *              netlink operation.
 * @see nl_sk_fam_exit()
 * @ingroup net
 * @retval 0 on success, negative value on failure
 */
extern int nl_sk_fam_init(void);

/**
 * @fn void nl_sk_fam_exit(void)
 * @brief freeing the alloccated memory.
 * @detail The Function free the memory allocated by nl_sk_fam_init().
 * @ingroup net
 * @see nl_sk_fam_init()
 */
extern void nl_sk_fam_exit(void);

#endif

/**@}*/
