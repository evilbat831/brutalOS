/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <bits/stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

struct icmphdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
    } un;
};

#define ICMP_ECHOREPLY 0       
#define ICMP_DEST_UNREACH 3    
#define ICMP_SOURCE_QUENCH 4  
#define ICMP_REDIRECT 5       
#define ICMP_ECHO 8           
#define ICMP_TIME_EXCEEDED 11 
#define ICMP_PARAMETERPROB 12  
#define ICMP_TIMESTAMP 13      // Timestamp Request
#define ICMP_TIMESTAMPREPLY 14 
#define ICMP_INFO_REQUEST 15   
#define ICMP_INFO_REPLY 16     
#define ICMP_ADDRESS 17        
#define ICMP_ADDRESSREPLY 18   
#define NR_ICMP_TYPES 18

__END_DECLS
