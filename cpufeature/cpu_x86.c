/* cpu_x86.c
 * 
 * C99/Python Author: Robert A. McLeod
 * Original Author  : Alexander J. Yee
 * Date Created     : 04/12/2014
 */

#pragma once
#ifndef _CPU_X86_H
#define _CPU_X86_H

//  Dependencies
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

struct cpu_x86{
    //  Vendor
    char* VendorID;

    // Core features
    int num_threads_per_core;   // if greater than 1 generally indicates Intel Hyperthreading (R) or other multi-threading architecture
    int num_virtual_cores;
    int num_physical_cores;

    int cache_line_size;  // Bytes
    int cache_L1_size;    // Bytes
    int cache_L2_size;    // Bytes
    int cache_L3_size;    // Bytes

    //  OS Features
    bool OS_x64;
    bool OS_AVX;
    bool OS_AVX512;

    //  Misc.
    bool HW_MMX;
    bool HW_x64;
    bool HW_ABM;
    bool HW_RDRAND;
    bool HW_BMI1;
    bool HW_BMI2;
    bool HW_ADX;
    bool HW_PREFETCHWT1;
    bool HW_MPX;

    //  SIMD: 128-bit
    bool HW_SSE;
    bool HW_SSE2;
    bool HW_SSE3;
    bool HW_SSSE3;
    bool HW_SSE41;
    bool HW_SSE42;
    bool HW_SSE4a;
    bool HW_AES;
    bool HW_SHA;

    //  SIMD: 256-bit
    bool HW_AVX;
    bool HW_XOP;
    bool HW_FMA3;
    bool HW_FMA4;
    bool HW_AVX2;

    //  SIMD: 512-bit
    bool HW_AVX512_F;
    bool HW_AVX512_PF;
    bool HW_AVX512_ER;
    bool HW_AVX512_CD;
    bool HW_AVX512_VL;
    bool HW_AVX512_BW;
    bool HW_AVX512_DQ;
    bool HW_AVX512_IFMA;
    bool HW_AVX512_VBMI;
};




#endif


//  Dependencies
#include "cpu_x86.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#   if _WIN32
#       include "cpu_x86_windows.c"
#   elif defined(__GNUC__) || defined(__clang__)
#       include "cpu_x86_linux.c"
#   else
#       error "No cpuid intrinsic defined for compiler."
#   endif
#else
#   error "No cpuid intrinsic defined for processor architecture."
#endif
// Global 
// We could pass this struct around as an argument, but since you can't have 
// two on a machine, it's a natural singleton
struct cpu_x86 *this_x86;


// void init_cpu_x86(void);
// void free_cpu_x86(void);
// void detect_host(void);

// void get_vendor_string(void);
// void detect_cores(void);
// void detect_cache(void);

// // Docs on cpuid can be found in the Intel(R) 64 and IA-32 Architectures 
// // Software Developer Manuals:
// // https://software.intel.com/en-us/articles/intel-sdm
// //
// // Presently in Vol 2A on pp. 3-190 to 3-205
// void cpuid(int32_t out[4], int32_t level, int32_t count);

// bool detect_OS_x64(void);
// bool detect_OS_AVX(void);
// bool detect_OS_AVX512(void);

void init_cpu_x86(void) {
    this_x86 = (struct cpu_x86 *)calloc(1, sizeof(struct cpu_x86));
}

void free_cpu_x86(void) {
    free(this_x86->VendorID);
    free(this_x86);
}

bool detect_OS_AVX(void) {
    //  Copied from: http://stackoverflow.com/a/22521619/922184

    bool avxSupported = false;

    int cpuInfo[4];
    cpuid(cpuInfo, 1, 0);

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
    {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        avxSupported = (xcrFeatureMask & 0x6) == 0x6;
    }

    return avxSupported;
}

bool detect_OS_AVX512(void) {
    if (!detect_OS_AVX())
        return false;

    uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    return (xcrFeatureMask & 0xe6) == 0xe6;
}

void get_vendor_string(void) {
    int32_t info[4];
    this_x86->VendorID = calloc(13, sizeof(char));

    cpuid(info, 0, 0);
    // Ordering here is very strange...
    memcpy(this_x86->VendorID + 0, &info[1], 4);
    memcpy(this_x86->VendorID + 4, &info[3], 4);
    memcpy(this_x86->VendorID + 8, &info[2], 4);
    // Calloc takes care of the trailing null
}

// input:  eax = functionnumber, ecx = 0
// output: eax = output[0], ebx = output[1], ecx = output[2], edx = output[3]

void detect_cores(void) {
    int info0[4], info1[4];

    // Core topology (0x0B)
    // So the first level (eax=0x0B, ecx=0x00) is hyperthreading/processors, and 
    // the next level (eax=0x0B, ecx=0x01) is cores
    cpuid(info0, 0x0B, 0x00);
    cpuid(info1, 0x0B, 0x01);
    // printf( "x0B,0x00: Processors: %d, %d, %d, %d\n", info[0], info[1], info[2], info[3]);
    // printf( "x0B,0x01: Cores:      %d, %d, %d, %d\n", info1[0], info1[1], info1[2], info1[3]);

    // I'm not sure if it's possible to count physical processors in this way 
    // for 2,4 CPU machines, as there it's usually better to have one process
    // per physical CPU in my experience...
    this_x86->num_threads_per_core = info0[1] & 0xFFFF;
    this_x86->num_virtual_cores = info1[1] & 0xFFFF;
    this_x86->num_physical_cores = this_x86->num_virtual_cores / this_x86->num_threads_per_core;
}

void detect_cache(void) {
    int info[4];
    
    cpuid(info, 0x80000006, 0);
    this_x86->cache_line_size = info[2] & (int)255;

    /*  
        0x02: Cache info
        This appears to be a (deprecated) large enumerator that is used for 
        TLB info.  The L1/2/3 cache info does not appear here anymore on 
        modern CPUs, probably because they ran out of enum values.
        
        Selected enums:
        0x00 -> nothing
        0xF0 -> 64 byte prefetch
        0xF1 -> 128 byte prefetch
        0xFF -> cache info present here */

    // unsigned char c_enums[16];
    // cpuid(c_enums, 0x02, 0);
    // printf( "x02: eax cache enums: __, %X, %X, %X\n", c_enums[1], c_enums[2], c_enums[3]);
    // printf( "x02: ebx cache enums: %X, %X, %X, %X\n", c_enums[4], c_enums[5], c_enums[6], c_enums[7]);
    // printf( "x02: ecx cache enums: %X, %X, %X, %X\n", c_enums[8], c_enums[9], c_enums[10], c_enums[11]);
    // printf( "x02: edx cache enums: %X, %X, %X, %X\n", c_enums[12], c_enums[13], c_enums[14], c_enums[15]);

    /*  
        0x04: Deterministic Cache Parameters by level
        This holds the L1/2/3 cache size that we want
    */
    int level, ways, partitions, line_size, sets, total_size;
    for(int I = 1; I < 10; I++) {
        cpuid(info, 0x04, I);
        level      = (info[0] & 112) >> 5;
        ways       = 1 + ((info[1] & 4290772992) >> 22); // 10 bits width << 22 bits shifted
        partitions = 1 + ((info[1] & 4190208) >> 12);    // 10 bits width << 12 bits shifted
        line_size  = 1 + (info[1] & 4095);               // 12 bits width << 0 bits shifted
        sets       = 1 + (info[2]);
        total_size = ways * partitions * line_size * sets;
        // printf( "0x04: Cache L%d:: ways: %d, partitions: %d, line_size: %d, sets: %d, total: %d\n", level, ways, partitions, line_size, sets, total_size);

        if (level == 0) {
            break;
        } else if (level == 1) {
            this_x86->cache_L1_size = total_size;
        }
        else if (level == 2) {
            this_x86->cache_L2_size = total_size;
        }
        else if (level == 3) {
            this_x86->cache_L3_size = total_size;
        }
    }

}


void detect_host(void) {
    //  OS Features
    this_x86->OS_x64 = detect_OS_x64();
    this_x86->OS_AVX = detect_OS_AVX();
    this_x86->OS_AVX512 = detect_OS_AVX512();
    
    //  Vendor
    get_vendor_string();
    detect_cores();
    detect_cache();

    int info[4];
    cpuid(info, 0, 0);
    int nIds = info[0];

    cpuid(info, 0x80000000, 0);
    uint32_t nExIds = info[0];
    

    //  Detect Features
    if (nIds >= 0x00000001) {
        cpuid(info, 0x00000001, 0);
        this_x86->HW_MMX    = (info[3] & ((int)1 << 23)) != 0;
        this_x86->HW_SSE    = (info[3] & ((int)1 << 25)) != 0;
        this_x86->HW_SSE2   = (info[3] & ((int)1 << 26)) != 0;
        this_x86->HW_SSE3   = (info[2] & ((int)1 <<  0)) != 0;

        this_x86->HW_SSSE3  = (info[2] & ((int)1 <<  9)) != 0;
        this_x86->HW_SSE41  = (info[2] & ((int)1 << 19)) != 0;
        this_x86->HW_SSE42  = (info[2] & ((int)1 << 20)) != 0;
        this_x86->HW_AES    = (info[2] & ((int)1 << 25)) != 0;

        this_x86->HW_AVX    = (info[2] & ((int)1 << 28)) != 0;
        this_x86->HW_FMA3   = (info[2] & ((int)1 << 12)) != 0;

        this_x86->HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
    }
    if (nIds >= 0x00000007) {
        cpuid(info, 0x00000007, 0);
        this_x86->HW_AVX2         = (info[1] & ((int)1 <<  5)) != 0;

        this_x86->HW_BMI1         = (info[1] & ((int)1 <<  3)) != 0;
        this_x86->HW_BMI2         = (info[1] & ((int)1 <<  8)) != 0;
        this_x86->HW_ADX          = (info[1] & ((int)1 << 19)) != 0;
        this_x86->HW_MPX          = (info[1] & ((int)1 << 14)) != 0;
        this_x86->HW_SHA          = (info[1] & ((int)1 << 29)) != 0;
        this_x86->HW_PREFETCHWT1  = (info[2] & ((int)1 <<  0)) != 0;

        this_x86->HW_AVX512_F     = (info[1] & ((int)1 << 16)) != 0;
        this_x86->HW_AVX512_CD    = (info[1] & ((int)1 << 28)) != 0;
        this_x86->HW_AVX512_PF    = (info[1] & ((int)1 << 26)) != 0;
        this_x86->HW_AVX512_ER    = (info[1] & ((int)1 << 27)) != 0;
        this_x86->HW_AVX512_VL    = (info[1] & ((int)1 << 31)) != 0;
        this_x86->HW_AVX512_BW    = (info[1] & ((int)1 << 30)) != 0;
        this_x86->HW_AVX512_DQ    = (info[1] & ((int)1 << 17)) != 0;
        this_x86->HW_AVX512_IFMA  = (info[1] & ((int)1 << 21)) != 0;
        this_x86->HW_AVX512_VBMI  = (info[2] & ((int)1 <<  1)) != 0;
    }
    if (nExIds >= 0x80000001) {
        cpuid(info, 0x80000001, 0);
        this_x86->HW_x64   = (info[3] & ((int)1 << 29)) != 0;
        this_x86->HW_ABM   = (info[2] & ((int)1 <<  5)) != 0;
        this_x86->HW_SSE4a = (info[2] & ((int)1 <<  6)) != 0;
        this_x86->HW_FMA4  = (info[2] & ((int)1 << 16)) != 0;
        this_x86->HW_XOP   = (info[2] & ((int)1 << 11)) != 0;
    }
}
