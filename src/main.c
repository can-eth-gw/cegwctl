/**
 * @file main.c
 * @defgroup files Core
 * @defgroup net Netlink
 * @defgroup trans Translation
 * @defgroup cb Callbacks
 * @brief Control Area Network - Ethernet - Gateway - Control (Utility)
 * @details The gateway is configured here.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* man getopt or
	http://www.gnu.org/software/libc/manual/html_node/Getopt.html */
#include <getopt.h>
#include <stdint.h>
#include <inttypes.h>
#include "netlink.h"

int verbose_flag;
int bidirectional_flag = 0;
uint32_t flags = 0;
uint8_t gw_type = TYPE_NET;

int main(int argc, char *argv[])
{
	int err = 0;

	err = nl_sk_fam_init();
	if (err != 0) {
		fprintf(stderr,
		        "Error during initialisation of Socket or Netlink "
		        "Family: %d\n", err);
		return EXIT_FAILURE;
	}

	int c;

	while (1) {
		static struct option long_options[] = {
			/* These options set a flag. */

			/* These options don't set a flag.
			   We distinguish them by their indices. */
			{"bidirectional", no_argument, 0, 'b'},
			{"can-fd",        no_argument, 0, 'f'},
			{"type",    required_argument, 0, 't'},
			{0, 0, 0, 0},
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "bft:",
		                 long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			printf ("option %s", long_options[option_index].name);

			if (optarg)
				printf (" with arg %s", optarg);
			printf ("\n");
			break;

		case 'b':
			bidirectional_flag = 1;
			break;

		case 'f':
			flags = flags | F_CAN_FD;
			break;

		case 't':
			if(!strcmp(optarg, "none")) {
				gw_type = TYPE_NONE;
			} else if(!strcmp(optarg, "eth")) {
				gw_type = TYPE_ETH;
			} else if (!strcmp(optarg, "net")) {
				gw_type = TYPE_NET;
			} else if (!strcmp(optarg, "tcp")) {
				gw_type = TYPE_TCP;
			} else if (!strcmp(optarg, "udp")) {
				gw_type = TYPE_UDP;
			} else {
				fprintf(stderr, "%s: Supported Types: "
				        "none, eth, net, tcp, udp\n", argv[0]);
				return EXIT_FAILURE;
			}

			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;

		default:
			abort();
		}
	}

	/* Instead of reporting ‘--verbose’
	   and ‘--brief’ as they are encountered,
	   we report the final status resulting from them. */
	if (verbose_flag)
		puts ("verbose flag is set\n");


	/***************************************************/
	/* Remaining command line arguments (not options). */
	/***************************************************/
	while (optind < argc) {

		/* add route SRC DST */
		if(!strcmp(argv[optind], "add") &&
		    !strcmp(argv[optind+1], "route") && optind+4 <= argc) {

			err = ce_gw_add(argv[optind+3], argv[optind+2],
			                gw_type, flags);
			if (err != 0) {
				fprintf(stderr, "%s: Error during add: %d",
				        argv[0], err);
				return EXIT_FAILURE;
			}

			if (bidirectional_flag == 1) {
				err = ce_gw_add(argv[optind+2], argv[optind+3],
				                gw_type, flags);
				if (err != 0) {
					fprintf(stderr, "%s: Error during "
					        "add: %d", argv[0], err);
					return EXIT_FAILURE;
				}
			}

			optind += 4;

			/* add dev [NAME] */
		} else if (!strcmp(argv[optind], "add") &&
		           !strcmp(argv[optind+1], "dev") && optind+2 <= argc) {

			if (optind+3 <= argc) {
				err = ce_gw_add(argv[optind+2], NULL, gw_type,
				                flags);
				if (err != 0) {
					fprintf(stderr, "%s: Error during "
					        "add: %d", argv[0], err);
					return EXIT_FAILURE;
				}

				optind += 3;
			} else {

				err = ce_gw_add("cegw%d", NULL, gw_type, flags);
				if (err != 0) {
					fprintf(stderr, "%s: Error during "
					        "add: %d", argv[0], err);
					return EXIT_FAILURE;
				}

				optind += 2;
			}

			/* del route ID */
		} else if(!strcmp(argv[optind], "del") &&
		          !strcmp(argv[optind+1], "route") && optind+3 <= argc ) {

			uintmax_t num = strtoumax(argv[optind+2], NULL, 0);
			if (num == UINTMAX_MAX && errno == ERANGE) {
				fprintf(stderr, "%s: Error: Parameter "
				        "ID is not a number %d\n",
				        argv[0], errno);
			}

			err = ce_gw_del((uint32_t) num, NULL);
			if (err != 0) {
				fprintf(stderr, "%s: Error during del: %d\n",
				        argv[0], err);
				return EXIT_FAILURE;
			}

			optind += 3;

			/* del dev DEV_NAME */
		} else if(!strcmp(argv[optind], "del") &&
		          !strcmp(argv[optind+1], "dev") && optind+3 <= argc ) {

			err = ce_gw_del(0, argv[optind+2]);
			if (err != 0) {
				fprintf(stderr, "%s: Error during del: %d\n",
				        argv[0], err);
				return EXIT_FAILURE;
			}

			optind += 3;

			/* echo MSG */
		} else if(!strcmp(argv[optind], "echo") && optind+2 <= argc) {

			err = ce_gw_echo(argv[optind+1]);
			if (err != 0) {
				fprintf(stderr, "%s: Error during echo: %d",
				        argv[0], err);
				return EXIT_FAILURE;
			}

			optind += 2;

			/* route */
		} else if(!strcmp(argv[optind], "route") && optind+1 <= argc) {

			if (optind+2 <= argc) {
				uintmax_t num = strtoumax(argv[optind+1],
				                          NULL, 0);
				if (num == UINTMAX_MAX && errno == ERANGE) {
					fprintf(stderr, "%s: Error: Parameter "
					        "ID is not a number %d\n",
					        argv[0], errno);
				}

				err = ce_gw_list(num);
				optind += 2;

			} else {
				err = ce_gw_list(0);
				optind += 1;
			}

			if (err != 0) {
				fprintf(stderr, "%s: Error during list: %d",
				        argv[0], err);
				return EXIT_FAILURE;
			}


			/* unrecognized command */
		} else {
			optind += 1;

			while(optind < argc) {
				printf(" %s", argv[optind]);
				optind += 1;
			}

			printf("'\n");
			return EXIT_FAILURE;
		}
	}

	nl_sk_fam_exit();
	return EXIT_SUCCESS;
}

/**@}*/
