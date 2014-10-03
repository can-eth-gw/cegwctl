/**
 * @file netlink.c
 * @brief Control Area Network - Ethernet - Gateway - Netlink (Utility)
 * @author Fabian Raab (fabian.raab@tum.de)
 * @date May, 2013
 * @copyright GNU Public License v3 or higher
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/attr.h>
#include <netlink/addr.h>
#include <netlink/msg.h>
#include <netlink/errno.h>

#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/mngt.h>
#include "netlink.h"

/**
 * Netlink Family Settings
 */
#define GE_FAMILY_NAME "CE_GW"
#define GE_FAMILY_VERSION 1
#define USER_HDR_SIZE 0 /**< user header size */
#define NO_FLAG 0
#define IFACE_VERSION 0

/**
 * @enum
 * @brief Data which can be send and received.
 * @details Set int values as Identifiers for userspace application.
 */
enum {
	CE_GW_A_UNSPEC, /**< Only a Dummy to skip index 0. */
	CE_GW_A_DATA,	/**< NLA_STRING */
	CE_GW_A_SRC,	/**< NLA_STRING */
	CE_GW_A_DST,	/**< NLA_STRING */
	CE_GW_A_ID,	/**< NLA_U32 */
	CE_GW_A_FLAGS,	/**< NLA_U32 */
	CE_GW_A_TYPE,	/**< NLA_U8 */
	CE_GW_A_HNDL,	/**< NLA_U32 Handled Frames */
	CE_GW_A_DROP,	/**< NLA_U32 Dropped Frames */
	__CE_GW_A_MAX,	/**< Maximum Number of Attribute plus 1 */
};
#define CE_GW_A_MAX (__CE_GW_A_MAX - 1) /**< Maximum Number of Attribute */

/**
 * @brief Netlink Policy - Defines the Type for the Netlink Attributes
 */
static struct nla_policy ce_gw_genl_policy[CE_GW_A_MAX + 1] = {
	[CE_GW_A_DATA] = 	{ .type = NLA_STRING },
	[CE_GW_A_SRC] = 	{ .type = NLA_STRING },
	[CE_GW_A_DST] = 	{ .type = NLA_STRING },
	[CE_GW_A_ID] = 		{ .type = NLA_U32 },
	[CE_GW_A_FLAGS] = 	{ .type = NLA_U32 },
	[CE_GW_A_TYPE] = 	{ .type = NLA_U8 },
	[CE_GW_A_HNDL] = 	{ .type = NLA_U32 },
	[CE_GW_A_DROP] = 	{ .type = NLA_U32 },
};

struct genl_family *genl_fam; /**< Generic Netlink Family */
struct nl_sock *nl_sk; /**< Socket to Kernel Application */

/**
 * @struct flags
 * @brief Flags defined in netlink.h. Can used by flags2str().
 */
const struct flags {
	char *name;
} flags_array[] = {
	{ "CAN-FD"	},	/**< Flag with index 0 */
	{ 0		}	/**< End Delimiter */
};

/**
 * @fn char *flags2str(uint32_t bits, const struct flags *flags, size_t size)
 * @brief Convert Flags in ist textual represenation
 * @param bits The bits variable which will be checked with the highest bit
 * @param flags An Array with its textual representation the index of the
 * one array entry must be the same as the position in param bits from the left.
 * The Last entry in the array bust be {0}, because the function will stop here.
 * @param size the size of the retuned char pointer. Must be big enough
 * for all flags names plus 2 for < and > plus 1 for ',' after each flag
 * plus 1 for \0
 * @ingroup trans
 * @returns a char pointer in the form <Flag1,Flag2,Flag3,...> if the flags
 * flag1, flag2, flag3, ... are set. The Pointer has a ending \0.
 */
char *flags2str(uint32_t bits, const struct flags *flags, size_t size)
{
	char *str = malloc(size);
	str[0] = '<';
	str[1] = '\0';

	/* Iterate until the end of bits or thh end of flags array */
	for (int i = 0; i < sizeof(uint32_t) * 8 && flags[i].name != 0; ++i) {
		// extract the i-th bit
		int b = ((bits >> i) & 1);
		// b will be 1 if i-th bit is set, 0 otherwise

		if (b == 1) {
			strcat(str, flags[i].name);
			strcat(str, ",");
		}
	}
	int len = strlen(str);

	if (len <= 1) {
		strcat(str, ">");
	} else {
		str[len-1] = '>'; /* replace ',' with '>' */
	}

	return str;
}

/**
 * @struct enums
 * @brief Array of type "enum gw_type" in netlink.h. Can use by enum2str().
 */
const struct enums {
	char *name;
} type_array[] = {
	{ "NONE"	}, /**< TYPE_NONE with Index 0 */
	{ "ETH"		}, /**< TYPE_ETH  with Index 1 */
	{ "NET"		}, /**< TYPE_NET  with Index 2 */
	{ "TCP"		}, /**< TYPE_TCP  with Index 3 */
	{ "UDP"		}, /**< TYPE_UDP  with Index 4 */
	{ 0		}  /**< End Delimiter          */
};

/**
 * @brief Returns the textual representation of an numeric identifier (enum)
 * @param value the numeric identifier (value) of an enum
 * @param enums A array with the textual representation of the int value.
 * The Last entry in the array bust be {0}, because the function will stop here.
 * The index af an array entry must be the same as param value.
 * @param size the size of the retuned char pointer. Must be big enough
 * for the largest entry in the array.
 * @retval NULL if not found
 * @ingroup trans
 * @returns A char pointer with the textual representation of value ending
 * with \0.
 */
char *enum2str(int value, const struct enums *enums, size_t size, int max)
{
	char *str = malloc(size+1); /* +1 for \0 */
	str[0] = '\0';

	/* Iterate until the end of value or the end of enums array */
	for (int i = 0; i <= max && enums[i].name != 0; ++i) {
		if (i == value) {
			strcat(str, enums[i].name);
			return str;
		}
	}

	free(str);
	return NULL;
}


/**
 * @fn int nl_cb_general_errno(struct sockaddr_nl *nla,
 *                      struct nlmsgerr *nlerr, void *arg)
 * @brief A callback wich prints the errno to stderr.
 * @param nla Socket address informations
 * @param nlerr error Message
 * @raram arg a file
 * @details A callback wich prints the errno to stderr. In nl_sk_fam_init() it
 *          is delcared as standart error callback.
 * @ingroup cb
 * @retval NL_STOP
 */
int nl_cb_general_errno(struct sockaddr_nl *nla,
                        struct nlmsgerr *nlerr, void *arg)
{
	int err = nlerr->error;
	fprintf(stderr, "NETLINK returned Error: %s\n", strerror(err));

	return NL_STOP;
}

int ce_gw_add(char *dst_name, char *src_name, uint8_t type, uint32_t flags)
{
	int err = 0;
	struct nl_msg *msg;

	/* create */
	msg = nlmsg_alloc();
	if(msg == NULL) {
		fprintf(stderr,"add: Message allocation failed.\n");
		return -ENOMEM;
	}

	void *user_hdr;
	user_hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
	                       genl_family_get_id(genl_fam), USER_HDR_SIZE,
	                       NO_FLAG, CE_GW_C_ADD, IFACE_VERSION);
	if (user_hdr == NULL)
		fprintf(stderr, "add: Message Haeder creation failed\n");

	if (src_name != NULL) { /* indicates that it is add route command */
		NLA_PUT_STRING(msg, CE_GW_A_SRC, src_name);
	} /* else it is add dev command */

	/* Attributes needed by both, add route and add dev */
	NLA_PUT_STRING(msg, CE_GW_A_DST, dst_name);
	NLA_PUT_U8(msg, CE_GW_A_TYPE, type);
	NLA_PUT_U32(msg, CE_GW_A_FLAGS, flags);

	/* vaildate */
	struct nlmsghdr *msghdr = nlmsg_hdr(msg);
	err = genlmsg_validate(msghdr, USER_HDR_SIZE,
	                       CE_GW_A_MAX, ce_gw_genl_policy);
	if (err != 0) {
		fprintf(stderr, "add: Validation of Message Failed: %i\n", err);
		return -1;
	}

	/* send */
	nl_send_auto(nl_sk, msg);

	err = nl_wait_for_ack(nl_sk);
	if (err != 0) {
		fprintf(stderr,
		        "add: ACK is missing or Error returned. "
		        "Operation might fail: %i\n", err);
	}

	nlmsg_free(msg);

	return 0;

nla_put_failure:
	fprintf(stderr, "Attribute Modification failed: %d\n",-EMSGSIZE);
	nlmsg_free(msg);
	return -EMSGSIZE;
}

int ce_gw_del(uint32_t id, char *dev_name)
{
	int err;
	struct nl_msg *msg;
	if (id != 0 && dev_name != NULL) {
		err = -EINVAL;
		return err;
	}

	/* create */
	msg = nlmsg_alloc();
	if(msg == NULL) {
		fprintf(stderr,"del: Message allocation failed.\n");
		return -1;
	}

	void *user_hdr;
	user_hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
	                       genl_family_get_id(genl_fam), USER_HDR_SIZE,
	                       NO_FLAG, CE_GW_C_DEL, IFACE_VERSION);
	if (user_hdr == NULL)
		fprintf(stderr, "del: Message Haeder creation failed\n");

	NLA_PUT_U32(msg, CE_GW_A_ID, id);
	if (dev_name != NULL) { /* del dev is called */
		NLA_PUT_STRING(msg, CE_GW_A_DST, dev_name);
	}

	/* vaildate */
	struct nlmsghdr *msghdr = nlmsg_hdr(msg);
	err = genlmsg_validate(msghdr, USER_HDR_SIZE,
	                       CE_GW_A_MAX, ce_gw_genl_policy);
	if (err != 0) {
		fprintf(stderr, "del: Validation of Message Failed: %i\n", err);
		return -1;
	}

	/* send */
	nl_send_auto(nl_sk, msg);

	err = nl_wait_for_ack(nl_sk);
	if (err != 0) {
		fprintf(stderr,
		        "add: ACK is missing or Error returned. "
		        "Operation might fail: %i\n", err);
	}

	nlmsg_free(msg);
	return 0;

nla_put_failure:
	fprintf(stderr, "Attribute Modification failed: %d\n",-EMSGSIZE);
	nlmsg_free(msg);
	return -EMSGSIZE;
}

/**
 * @fn int nl_cb_list_entry(struct nl_msg *msg, void *arg)
 * @brief will be called for every multipart message and prints informations to
 *        stdout.
 * @param msg Netlink Message
 * @raram arg a file
 * @retval NL_OK
 * @ingroup cb
 * @see defined as callback in ce_gw_list()
 */
int nl_cb_list_entry(struct nl_msg *msg, void *arg)
{
	int err;

	struct nlmsghdr *msghdr = nlmsg_hdr(msg);

	struct nlattr *attrs[CE_GW_A_MAX+1];
	err = genlmsg_parse(msghdr, USER_HDR_SIZE, attrs,
	                    CE_GW_A_MAX, ce_gw_genl_policy);
	if (err < 0)
		fprintf(stderr, "ERROR Kernel Message Parsing Failed\n");

	char *a_msg_src_data = (char *)nla_data(attrs[CE_GW_A_SRC]);
	char *a_msg_dst_data = (char *)nla_data(attrs[CE_GW_A_DST]);
	uint32_t *a_msg_id_data = (uint32_t *)nla_data(attrs[CE_GW_A_ID]);
	uint32_t *a_msg_flags_data = (uint32_t *)nla_data(attrs[CE_GW_A_FLAGS]);
	uint8_t *a_msg_type_data = (uint8_t *)nla_data(attrs[CE_GW_A_TYPE]);
	uint32_t *a_msg_hndl_data = (uint32_t *)nla_data(attrs[CE_GW_A_HNDL]);
	uint32_t *a_msg_drop_data = (uint32_t *)nla_data(attrs[CE_GW_A_DROP]);

	char *type_str;
	type_str = enum2str(*a_msg_type_data, type_array, 6, TYPE_MAX);
	char *flags_str;
	/* 256 should be big enough */
	flags_str = flags2str(*a_msg_flags_data, flags_array, 256);

	printf(" %-8d %-6s %-6s %-6s %-8d %-8d %s\n", *a_msg_id_data,
	       a_msg_src_data, a_msg_dst_data, type_str, *a_msg_hndl_data,
	       *a_msg_drop_data, flags_str);

	free(type_str);
	free(flags_str);
	return NL_OK;
}

/**
 * @fn int nl_cb_list_finish(struct nl_msg *msg, void *arg)
 * @brief will be called at the end of a multipart message.
 * @param msg Netlink Message
 * @raram arg a file
 * @retval NL_STOP
 * @ingroup cb
 * @see defined as callback in ce_gw_list()
 */
int nl_cb_list_finish(struct nl_msg *msg, void *arg)
{
	return NL_STOP;
}

int ce_gw_list(uint32_t id)
{
	int err;
	struct nl_msg *msg;

	/* create */
	msg = nlmsg_alloc();
	if(msg == NULL) {
		fprintf(stderr,"echo: Message allocation failed.\n");
		return -1;
	}

	void *user_hdr;
	user_hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
	                       genl_family_get_id(genl_fam), USER_HDR_SIZE,
	                       NO_FLAG, CE_GW_C_LIST, IFACE_VERSION);
	if (user_hdr == NULL)
		fprintf(stderr, "echo: Message Haeder creation failed.\n");

	NLA_PUT_U32(msg, CE_GW_A_ID, id);

	/* vaildate */
	struct nlmsghdr *msghdr = nlmsg_hdr(msg);
	err = genlmsg_validate(msghdr, 0, CE_GW_A_MAX, ce_gw_genl_policy);
	if (err != 0) {
		fprintf(stderr, "echo: Validation of Message Failed: %i\n",
		        err);
		return -1;
	}

	/* send */
	nl_socket_disable_auto_ack(nl_sk);
	nl_send_auto(nl_sk, msg);

	/* create callback system */
	struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl_cb_list_entry, NULL);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl_cb_list_finish, NULL);
	nl_cb_err(cb, NL_CB_CUSTOM, nl_cb_general_errno, NULL);

	printf(" ID       SRC    DST    TYPE   HANDLED  DROPPED  FLAGS\n");

	nl_recvmsgs(nl_sk, cb);

	nl_cb_put(cb);
	nlmsg_free(msg);

	return 0;

nla_put_failure:
	fprintf(stderr, "Attribute Modification failed: %d\n",-EMSGSIZE);
	nlmsg_free(msg);
	return -1;
}

/**
 * @fn int nl_cb_echo_answer(struct nl_msg *msg, void *arg)
 * @brief Callback witch receive the message sen bach by kernel
 * @param msg Netlink Message
 * @raram arg a file
 * @retval NL_OK
 * @ingroup cb
 * @see defined as callback in ce_gw_echo()
 */
int nl_cb_echo_answer(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *msghdr = nlmsg_hdr(msg);
	struct genlmsghdr *gemsghdr =  nlmsg_data(msghdr);

	struct nlattr *a_msg = genlmsg_attrdata(gemsghdr, 0);
	char * a_msg_data = (char *) nla_data(a_msg);
	printf("kernel says: %s\n", a_msg_data);

	return NL_OK;
}

int ce_gw_echo(char *message)
{
	int rc; /* return codes */
	struct nl_msg *msg;

	/* create */
	msg = nlmsg_alloc();
	if(msg == NULL) {
		fprintf(stderr,"echo: Message allocation failed.\n");
		return -1;
	}

	void *user_hdr;
	user_hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
	                       genl_family_get_id(genl_fam), USER_HDR_SIZE,
	                       NO_FLAG, CE_GW_C_ECHO, IFACE_VERSION);
	if (user_hdr == NULL)
		fprintf(stderr, "echo: Message Haeder creation failed.\n");

	NLA_PUT_STRING(msg, CE_GW_A_DATA, message);

	/* vaildate */
	struct nlmsghdr *msghdr = nlmsg_hdr(msg);
	if ((rc = genlmsg_validate(msghdr,0,CE_GW_A_MAX, ce_gw_genl_policy)) != 0) {
		fprintf(stderr, "echo: Validation of Message Failed: %i\n", rc);
		return -1;
	}

	/* send */
	nl_socket_disable_auto_ack(nl_sk);
	nl_send_auto(nl_sk, msg);

	/* create callback system */
	struct nl_cb *cb = nl_cb_alloc(NL_CB_DEBUG);
	nl_cb_set(cb, NL_CB_MSG_IN, NL_CB_CUSTOM , nl_cb_echo_answer, NULL);
	nl_recvmsgs(nl_sk, cb);
	nl_cb_put(cb);


	nlmsg_free(msg);

	return 0;

nla_put_failure:
	fprintf(stderr, "Attribute Modification failed: %d\n",-EMSGSIZE);
	nlmsg_free(msg);
	return -1;
}


int nl_sk_fam_init(void)
{
	int err;

	/* create gneric netlink socket */
	nl_sk = nl_socket_alloc();
	if ( nl_sk == NULL ) {
		fprintf (stderr, "Socket allocation failed.\n");
		return -1;
	}

	err = genl_connect(nl_sk);
	if (err != 0) {
		fprintf (stderr, "Connetion to socket failed: %d\n", err);
		return err;
	}

	err = nl_socket_modify_err_cb(nl_sk, NL_CB_CUSTOM,
	                              nl_cb_general_errno, NULL);
	if (err != 0) {
		fprintf (stderr, "Error Callback modification failed: %d\n",
		         err);
	}

	/* create generic netlink family */
	genl_fam = genl_family_alloc();
	if (genl_fam == NULL) {
		fprintf (stderr, "Family allocation failed.\n");
		return -1;
	}

	genl_family_set_name(genl_fam, GE_FAMILY_NAME);
	genl_family_set_version(genl_fam, GE_FAMILY_VERSION);
	genl_family_set_maxattr(genl_fam, CE_GW_A_MAX);
	genl_family_set_hdrsize(genl_fam, USER_HDR_SIZE);

	genl_family_set_id(genl_fam,
	                   (err = genl_ctrl_resolve(nl_sk, GE_FAMILY_NAME)));
	if (err < 0) {
		fprintf(stderr,
		        "Could not resolve Netlink Family ID from kernel. "
		        "Is the module loaded?: %d\n", err);
		return err;
	}

	return 0;
}


void nl_sk_fam_exit(void)
{
	nl_close(nl_sk);
	genl_family_put(genl_fam);
	nl_socket_free(nl_sk);
}
