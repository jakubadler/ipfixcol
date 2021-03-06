/**
 * \file siso.h
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

#ifndef SISO_H
#define	SISO_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <inttypes.h>
#include <sys/types.h>
    
    #define SISO_OK  0
    #define SISO_ERR 1
    
    /**
     * \brief Main structure
     */
    typedef struct sisoconf_s sisoconf;
    
    /**
     * \brief Accepted units
     */
    enum siso_units {
        SU_BYTE,
        SU_KBYTE,
        SU_MBYTE,
        SU_GBYTE
    };
    
    /**
     * \brief Constructor
     * 
     * @return new sisoconf object
     */
    sisoconf *siso_create();
    
    /**
     * \brief Destructor
     */
    void siso_destroy(sisoconf *conf);
    
    /**
     * \brief Get socket descriptor
     * 
     * @param conf sisoconf configuration
     * @return socket
     */
    int siso_get_socket(sisoconf *conf);
    
    /**
     * \brief Get connection type
     * 
     * @param conf sisoconf configuration
     * @return connection type
     */
    int siso_get_conn_type(sisoconf *conf);
    
    /**
     * \brief Get speed limit
     * 
     * @param conf sisoconf configuration
     * @return 0 if not set
     */
    uint64_t siso_get_speed(sisoconf *conf);
    
    /**
     * \brief Get last error description
     * 
     * @param conf sisoconf configuration
     * @return error message
     */
    const char *siso_get_last_err(sisoconf *conf);

    /**
     * \brief Set unlimited speed
     * 
     * @param conf sisoconf configuration
     */
    void siso_unlimit_speed(sisoconf* conf);
    
    /**
     * \brief Set max. speed limit
     * 
     * @param conf sisoconf configuration
     * @param limit speed limit
     * @param units data size units
     */
    void siso_set_speed(sisoconf* conf, uint32_t limit, enum siso_units units);
    
    
    /**
     * \brief Set max. speed limit
     * 
     * @param conf sisoconf configuration
     * @param limit speed limit (with optional suffix K,M,G)
     */
    void siso_set_speed_str(sisoconf* conf, const char* limit);
    
    /**
     * \brief Create new connection
     *      Each sisoconf object can hold only one connection,
     *      any previous connection will be closed
     * 
     * @param conf sisoconf configuration
     * @param ip IPv4/6 address
     * @param port port number
     * @param type connection type (case insensitive)
     * @return SISO_OK or SISO_ERR and sets error message (see siso_get_last_err for details)
     */
    int siso_create_connection(sisoconf* conf, const char* ip, const char *port, const char *type);
    
    /**
     * \brief Close current connection
     *          it is automatically called in siso_destroy
     * 
     * @param conf sisoconf configuration
     */
    void siso_close_connection(sisoconf *conf);

    /**
     * \brief Set operations non blocking
     * 
     * @param conf sisoconf configuration
     * @return SISO_OK or SISO_ERR and sets error message (see siso_get_last_err for details)
     */
    int siso_set_nonblocking(sisoconf *conf);
    
    /**
     * \brief Send data
     * 
     * @param conf sisoconf configuration
     * @param data data to be sent
     * @param length data length
     * @return SISO_OK or SISO_ERR and sets error message (see siso_get_last_err for details)
     */
    int siso_send(sisoconf *conf, const char *data, ssize_t length);

#ifdef	__cplusplus
}
#endif

#endif	/* SISO_H */

