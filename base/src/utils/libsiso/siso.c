/**
 * \file siso.c
 * \author Michal Kozubik <kozubik@cesnet.cz>
 * \brief Simple socket library for sending data over the network
 *
 * Copyright (C) 2014 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include "siso.h"

#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/time.h>
#include <time.h>

#include <fcntl.h>
#include <stdio.h>
#include <strings.h>

#define CHECK_PTR(_ptr_) if (!(_ptr_)) return (SISO_ERR)
#define CHECK_RETVAL(_retval_) if ((_retval_) != SISO_OK) return (SISO_ERR)

#define PERROR_LAST strerror(errno)

#define SISO_UDP_MAX 65000
#define SISO_MIN(_frst_, _scnd_) ((_frst_) > (_scnd_) ? (_scnd_) : (_frst_))

/**
* \brief Accepted connection types
*/
enum siso_conn_type {
   SC_UDP,
   SC_TCP,
   SC_SCTP,
   SC_UNKNOWN,
};


/**
 * \brief Message indexes
 */
enum SISO_ERRORS {
	SISO_ERR_OK,
	SISO_ERR_TYPE,
};

static const char *siso_messages[] = {
	"Everything OK",
	"Unknown connection type"
};

static const char *siso_sc_types[] = {
	"UDP", "TCP", "SCTP"
};

/**
 * \brief Main sisolib structure
 */
struct sisoconf_s {
	const char *last_error;		/**< last error message */
	enum siso_conn_type type;	/**< UDP/TCP/SCTP */
	struct addrinfo *servinfo;  /**< server information */
	int sockfd;					/**< socket descriptor */
	uint64_t max_speed;			/**< max sending speed */
	uint64_t act_speed;			/**< actual speed */
	struct timeval begin, end; /**< start/end time for limited transfers */
};


/**
 * \brief Constructor
 */
sisoconf *siso_create()
{
	/* allocate memory */
	sisoconf *conf = (sisoconf *) calloc(1, sizeof(sisoconf));
	if (!conf) {
		return NULL;
	}
	
	/* Set default message */
	conf->last_error = siso_messages[SISO_ERR_OK];
	
	return conf;
}

/**
 * \brief Destructor
 */
void siso_destroy(sisoconf *conf)
{
	/* Close socket and free resources */
	siso_close_connection(conf);
	free(conf);
}

/**
 * \brief Converts connection type from string to enum
 */
enum siso_conn_type siso_decode_type(sisoconf *conf, const char *type)
{
	int i;
	for (i = 0; i < (int) SC_UNKNOWN; ++i) {
		if (!strcasecmp(type, siso_sc_types[i])) {
			conf->type = (enum siso_conn_type) i;
			return SISO_OK;
		}
	}
	
	/* Unknown connection type */
	conf->last_error = siso_messages[SISO_ERR_TYPE];
	return SISO_ERR;
}

/**
 * Getters
 */
inline int siso_get_socket(sisoconf *conf)		{ return conf->sockfd; }
inline int siso_get_conn_type(sisoconf *conf)   { return conf->type; }
inline uint64_t siso_get_speed(sisoconf *conf)  { return conf->max_speed; }
inline const char *siso_get_last_err(sisoconf *conf){ return conf->last_error; }

/**
 * \brief Set unlimited speed
 */
void siso_unlimit_speed(sisoconf* conf)
{
	conf->max_speed = 0;
}

/**
 * \brief Set speed limit
 */
void siso_set_speed(sisoconf* conf, uint32_t limit, enum siso_units units)
{
	conf->max_speed = limit;
	
	switch (units) {
	case SU_KBYTE:
		conf->max_speed *= 1024;
		break;
	case SU_MBYTE:
		conf->max_speed *= 1024 * 1024;
		break;
	case SU_GBYTE:
		conf->max_speed *= 1024 * 1024 * 1024;
		break;
	default:
		break;
	}
}

/**
 * \brief Set speed limit
 */
void siso_set_speed_str(sisoconf* conf, const char* limit)
{
	conf->max_speed = strtoul(limit, NULL, 10);
	char last = limit[strlen(limit) - 1];
	
	switch (last) {
	case 'k': case 'K':
		conf->max_speed *= 1024;
		break;
	case 'm': case 'M':
		conf->max_speed *= 1024 * 1024;
		break;
	case 'g': case 'G':
		conf->max_speed *= 1024 * 1024 * 1024;
		break;
	default:
		break;
	}
}

/**
 * \brief Get server informations
 */
int siso_getaddrinfo(sisoconf *conf, const char *ip, const char *port)
{
    struct addrinfo hints;
	
    /* Get server address */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (conf->type == SC_UDP) ? SOCK_DGRAM : SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &(conf->servinfo)) != 0) {
		conf->last_error = PERROR_LAST;
        return SISO_ERR;
    }
	
    return SISO_OK;
}

/**
 * \brief Create new socket
 */
int siso_create_socket(sisoconf *conf)
{
	conf->sockfd = socket(conf->servinfo->ai_family, conf->servinfo->ai_socktype, conf->servinfo->ai_protocol);
	if (conf->sockfd == -1) {
		conf->last_error = PERROR_LAST;
		return SISO_ERR;
	}
	
	return SISO_OK;
}

/**
 * \brief Connect socket to server
 */
int siso_connect(sisoconf *conf)
{
	if (connect(conf->sockfd, conf->servinfo->ai_addr, conf->servinfo->ai_addrlen) < 0) {
		conf->last_error = PERROR_LAST;
		return SISO_ERR;
	}
	
	return SISO_OK;
}

/**
 * \brief Create new connection
 */
int siso_create_connection(sisoconf* conf, const char* ip, const char *port, const char *type)
{
	CHECK_PTR(conf);
	
	/* Close previous connection */
	if (conf->sockfd > 0) {
		siso_close_connection(conf);
	}
	
	/* Set connection type */
	int ret = siso_decode_type(conf, type);
	CHECK_RETVAL(ret);
	
	/* Get server info */
	ret = siso_getaddrinfo(conf, ip, port);
	CHECK_RETVAL(ret);

    /* Create socket */
	ret = siso_create_socket(conf);
	CHECK_RETVAL(ret);
	
	/* Connect */
	if (conf->type != SC_UDP) {
		ret = siso_connect(conf);
		CHECK_RETVAL(ret);
	}
	
	return SISO_OK;
}

/**
 * \brief Close actual connection
 */
void siso_close_connection(sisoconf *conf)
{
	if (conf && conf->sockfd > 0) {
		close(conf->sockfd);
		conf->sockfd = -1;
	}
}

int siso_set_nonblocking(sisoconf *conf)
{
	CHECK_PTR(conf);
	
	if (fcntl(conf->sockfd, F_SETFL, O_NONBLOCK) != 0) {
		conf->last_error = PERROR_LAST;
		return SISO_ERR;
	}
	
	return SISO_OK;
}

/**
 * \brief Send data
 */
int siso_send(sisoconf *conf, const char *data, ssize_t length)
{
	CHECK_PTR(conf);
	
	/* Pointer to data to be sent */
	const char *ptr = data;
	
	/* data sent per cycle */
	ssize_t sent_now = 0;
	
	/* Size of remaining data */
    ssize_t todo = length;
	
    while (todo > 0) {
        /* Send data */
		switch (conf->type) {
		case SC_UDP:
			sent_now = sendto(conf->sockfd, ptr, SISO_MIN(todo, SISO_UDP_MAX), 0, conf->servinfo->ai_addr, conf->servinfo->ai_addrlen);
			break;
		case SC_TCP:
		case SC_SCTP:
			sent_now = send(conf->sockfd, ptr, todo, 0);
			break;
		default:
			break;
		}

        /* Check for errors */
        if (sent_now == -1) {
			conf->last_error = PERROR_LAST;
            return SISO_ERR;
        }
		
		/* Skip sent data */
		ptr  += sent_now;
		todo -= sent_now;
		
		/* check speed limit */
		conf->act_speed += sent_now;
		if (conf->max_speed && conf->act_speed >= conf->max_speed) {
			gettimeofday(&(conf->end), NULL);
			
			/* Should sleep? */
			double elapsed = conf->end.tv_usec - conf->begin.tv_usec;
			if (elapsed < 1000000.0) {
				usleep(1000000.0 - elapsed);
				gettimeofday(&(conf->end), NULL);
			}
			
			/* reinit values */
			conf->begin = conf->end;
			conf->act_speed = 0;
		}
		
    }

	return SISO_OK;
}