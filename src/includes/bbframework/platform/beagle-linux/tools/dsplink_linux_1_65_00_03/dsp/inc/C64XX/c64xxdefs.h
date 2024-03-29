/** ============================================================================
 *  @file   c64xxdefs.h
 *
 *  @path   $(DSPLINK)/dsp/inc/C64XX/
 *
 *  @desc   Defines which are common to C64XX device type.
 *
 *  @ver    1.65.00.03
 *  ============================================================================
 *  Copyright (C) 2002-2009, Texas Instruments Incorporated -
 *  http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  ============================================================================
 */


#if !defined (C64XXDEFS_H)
#define C64XXDEFS_H


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  @const  SHMEM_INTERFACE
 *
 *  @desc   Interface number for shared memory interface.
 *  ============================================================================
 */
#define SHMEM_INTERFACE    0

/** ============================================================================
 *  @const  PCI_INTERFACE
 *
 *  @desc   Interface number for PCI interface.
 *  ============================================================================
 */
#define PCI_INTERFACE       1

/** ============================================================================
 *  @const  VLYNQ_INTERFACE
 *
 *  @desc   Interface number for VLYNQ interface.
 *  ============================================================================
 */
#define VLYNQ_INTERFACE     2

/** ============================================================================
 *  @const  DSPLINK_BUF_ALIGN
 *
 *  @desc   Value of Align parameter to alloc/create calls.
 *  ============================================================================
 */
#define DSPLINK_BUF_ALIGN     128

/** ============================================================================
 *  @const  DSP_MAUSIZE
 *
 *  @desc   Size of the DSP MAU (in bytes).
 *  ============================================================================
 */
#define DSP_MAUSIZE           1

/** ============================================================================
 *  @const  CACHE_L2_LINESIZE
 *
 *  @desc   Line size of DSP L2 cache (in bytes).
 *  ============================================================================
 */
#define CACHE_L2_LINESIZE     128

/** ============================================================================
 *  @const  ADD_PADDING
 *
 *  @desc   Macro to add padding to a structure.
 *  ============================================================================
 */
#define ADD_PADDING(padVar, count)  Uint16 padVar [count] ;

#if defined (MSGQ_COMPONENT)
/** ============================================================================
 *  @const  PCPYMQT_CTRLMSG_SIZE
 *
 *  @desc   Size (in MADUs) of the control messages used by the PCPY MQT.
 *  ============================================================================
 */
#define PCPYMQT_CTRLMSG_SIZE  128

/** ============================================================================
 *  @const  ZCPYMQT_CTRLMSG_SIZE
 *
 *  @desc   Size (in MADUs) of the control messages used by the ZCPY MQT.
 *  ============================================================================
 */
#define ZCPYMQT_CTRLMSG_SIZE  128
#endif /* defined (MSGQ_COMPONENT) */

/*  ============================================================================
 *  @const  DSPLINK_ALIGN
 *
 *  @desc   Macro to align a number to a specified value.
 *          x: The number to be aligned
 *          y: The value that the number should be aligned to.
 *  ============================================================================
 */
#define DSPLINK_ALIGN(x, y) (Uint32)((Uint32)((x + y - 1) / y) * y)

/** ============================================================================
 *  @const  DSPLINK_16BIT_PADDING
 *
 *  @desc   Padding required for alignment of a 16-bit value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define DSPLINK_16BIT_PADDING  ((CACHE_L2_LINESIZE - sizeof (Uint16)) / 2)

/** ============================================================================
 *  @const  DSPLINK_32BIT_PADDING
 *
 *  @desc   Padding required for alignment of a 32-bit value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define DSPLINK_32BIT_PADDING  ((CACHE_L2_LINESIZE - sizeof (Uint32)) / 2)

/** ============================================================================
 *  @const  DSPLINK_BOOL_PADDING
 *
 *  @desc   Padding required for alignment of a Boolean value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define DSPLINK_BOOL_PADDING  ((CACHE_L2_LINESIZE - sizeof (Bool)) / 2)

/** ============================================================================
 *  @const  DSPLINK_PTR_PADDING
 *
 *  @desc   Padding required for alignment of a pointer value (for L2 cache)
 *          in 16-bit words.
 *  ============================================================================
 */
#define DSPLINK_PTR_PADDING  ((CACHE_L2_LINESIZE - sizeof (Void *)) / 2)

/** ============================================================================
 *  @const  DRV_PADDING
 *
 *  @desc   Padding required for DSP L2 cache line alignment within DRV
 *          control structure.
 *  ============================================================================
 */
#define DRV_CTRL_SIZE      (   (sizeof (Uint32) * 23)                      \
                            +  (sizeof (Char) * DSP_MAX_STRLEN)            \
                            +  (sizeof (Uint32) * 4))

#define DRV_PADDING        ((     DSPLINK_ALIGN (DRV_CTRL_SIZE,            \
                                                 DSPLINK_BUF_ALIGN)        \
                             -    DRV_CTRL_SIZE) / 2)

/** ============================================================================
 *  @const  IPS_EVENT_ENTRY_PADDING
 *
 *  @desc   Padding length for IPS event entry.
 *  ============================================================================
 */
#define IPS_EVENT_ENTRY_PADDING (  (CACHE_L2_LINESIZE                          \
                                - ((sizeof (Uint32)) * 3)) / 2)

/** ============================================================================
 *  @const  IPS_CTRL_PADDING
 *
 *  @desc   Padding length for IPS control structure.
 *  ============================================================================
 */
#define IPS_CTRL_PADDING (  (CACHE_L2_LINESIZE                              \
                          - (sizeof (Void *) * 6)) / 2)

/** ============================================================================
 *  @const  DSPLINKIPS_CTRL_PADDING
 *
 *  @desc   Padding length for the DSPLINKIPS shared configuration structure.
 *  ============================================================================
 */
#define DSPLINKIPS_CTRL_PADDING    (  (CACHE_L2_LINESIZE                    \
                                    - (   sizeof (Uint32)                   \
                                       +  (sizeof (Uint32) * 6))) / 2)

#if defined (POOL_COMPONENT)
/** ============================================================================
 *  @const  DSPLINKPOOL_CTRL_PADDING
 *
 *  @desc   Padding length for the DSPLINKPOOL shared configuration structure.
 *  ============================================================================
 */
#define DSPLINKPOOL_CTRL_PADDING    (  (CACHE_L2_LINESIZE                    \
                                     - (   sizeof (Uint32)                   \
                                        +  (sizeof (Uint32) * 5))) / 2)
#endif /* if defined (POOL_COMPONENT) */

#if defined (MSGQ_COMPONENT)
/** ============================================================================
 *  @const  DSPLINKMQT_CTRL_PADDING
 *
 *  @desc   Padding length for the MQT shared configuration structure.
 *  ============================================================================
 */
#define DSPLINKMQT_CTRL_PADDING    (  (CACHE_L2_LINESIZE                    \
                                    - (   sizeof (Uint32)                   \
                                       +  (sizeof (Uint32) * 5))) / 2)
#endif /* if defined (MSGQ_COMPONENT) */

#if defined (CHNL_COMPONENT)
/** ============================================================================
 *  @const  DSPLINKDATA_CTRL_PADDING
 *
 *  @desc   Padding length for the Data driver shared configuration structure.
 *  ============================================================================
 */
#define DSPLINKDATA_CTRL_PADDING    (  (CACHE_L2_LINESIZE                    \
                                     - (   sizeof (Uint32)                   \
                                        +  (sizeof (Uint32) * 9))) / 2)
#endif /* if defined (CHNL_COMPONENT) */

#if defined (MPCS_COMPONENT)
/** ============================================================================
 *  @const  MPCSOBJ_PROC_PADDING
 *
 *  @desc   Padding required for alignment of MPCS object for a processor (for
 *          L2 cache) in 16-bit words.
 *  ============================================================================
 */
#define MPCSOBJ_PROC_PADDING ((CACHE_L2_LINESIZE - sizeof (MPCS_ProcObj)) / 2)

/** ============================================================================
 *  @const  MPCS_CTRL_PADDING
 *
 *  @desc   Padding length for MPCS control region.
 *  ============================================================================
 */
#define MPCS_CTRL_PADDING ((    CACHE_L2_LINESIZE                          \
                            -   (  (sizeof (Uint32) * 5)                   \
                                 + (sizeof (Void *)))) / 2)

/** ============================================================================
 *  @const  MPCS_ENTRY_PADDING
 *
 *  @desc   Padding length for MPCS_Entry.
 *  ============================================================================
 */
#define MPCS_ENTRY_PADDING ((   CACHE_L2_LINESIZE                     \
                             -  (  (sizeof (Ptr))                     \
                                 + (DSP_MAX_STRLEN * sizeof (Char))   \
                                 + (sizeof (Uint16) * 2))) / 2)

/** ============================================================================
 *  @const  MPCS_TURN_PADDING
 *
 *  @desc   Pad length for MPCS turn member.
 *  ============================================================================
 */
#define MPCS_TURN_PADDING ((  CACHE_L2_LINESIZE                                \
                          - (sizeof (Uint16))) /2)
#endif /* if defined (MPCS_COMPONENT) */

#if defined (RINGIO_COMPONENT)
/** ============================================================================
 *  @const  RINGIO_ENTRY_PADDING
 *
 *  @desc   Padding length for RingIO_Entry.
 *  ============================================================================
 */
#define RINGIO_ENTRY_PADDING ((  CACHE_L2_LINESIZE                           \
                               - (  (RINGIO_NAME_MAX_LEN * sizeof (Char))    \
                                  + (sizeof (Void *) * 2)                    \
                                  + (sizeof (Uint16) * 5))) / 2)

/** ============================================================================
 *  @const  RINGIO_CTRL_PADDING
 *
 *  @desc   Padding length for RINGIO control region.
 *  ============================================================================
 */
#define RINGIO_CTRL_PADDING ((    CACHE_L2_LINESIZE                          \
                              -   (  (sizeof (Uint32) * 5)                   \
                                   + (sizeof (Void *)))) / 2)

/*  ============================================================================
 *  @const  RINGIO_CLIENT_PADDING
 *
 *  @desc   Padding length for Client structure.
 *  ============================================================================
 */
#define RINGIO_CLIENT_PADDING ((  CACHE_L2_LINESIZE                            \
                                - (  sizeof (RingIO_BufPtr)* 2                 \
                                   + sizeof (Uint32) * 11                      \
                                   + sizeof (RingIO_NotifyFunc)                \
                                   + sizeof (RingIO_NotifyParam)               \
                                   + sizeof (RingIO_ControlStruct *)           \
                                   + sizeof (Void *)                           \
                                   + sizeof (Uint16))) / 2)

/** ============================================================================
 *  @const  RINGIO_CONTROLSTRUCT_PADDING
 *
 *  @desc   Padding length for control structure.
 *  ============================================================================
 */
#define RINGIO_CONTROLSTRUCT_PADDING ((  CACHE_L2_LINESIZE                     \
                                       - (  (sizeof (Uint32) * 14)             \
                                          + sizeof (Int32)                     \
                                          + (sizeof (RingIO_BufPtr)* 2)        \
                                          + sizeof (Void *))) /2)
#endif /* if defined (RINGIO_COMPONENT) */

#if defined (MPLIST_COMPONENT)
/** ============================================================================
 *  @const  MPLIST_ENTRY_PADDING
 *
 *  @desc   Padding length for MPLIST_Entry.
 *  ============================================================================
 */
#define MPLIST_ENTRY_PADDING ((  CACHE_L2_LINESIZE                       \
                               - (  (DSP_MAX_STRLEN * sizeof (Char))     \
                                  + (sizeof (Void *))                    \
                                  + (sizeof (Uint16) * 2))) / 2)         \

/** ============================================================================
 *  @const  MPLIST_CTRL_PADDING
 *
 *  @desc   Padding length for MPLIST control region.
 *  ============================================================================
 */
#define MPLIST_CTRL_PADDING ((    CACHE_L2_LINESIZE                          \
                              -   (  (sizeof (Uint32) * 5)                   \
                                   + (sizeof (Void *)))) / 2)

/** ============================================================================
 *  @const  MPLIST_LIST_PADDING
 *
 *  @desc   Padding length for MPLIST_List.
 *  ============================================================================
 */
#define MPLIST_LIST_PADDING ((    CACHE_L2_LINESIZE                          \
                              -   (sizeof (Void *) * 2)) / 2)
#endif /* if defined (MPLIST_COMPONENT) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif /* !defined (C64XXDEFS_H) */
