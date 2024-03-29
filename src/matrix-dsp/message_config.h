/** ============================================================================
 *  @file   matrix_config.h
 *
 *  @path
 *
 *  @desc   Header file for MSGQ and POOL configurations for matrix.
 *
 *  @ver    1.10
 *  ============================================================================
 *  Copyright (c) Texas Instruments Incorporated 2002-2009
 *
 *  Use of this software is controlled by the terms and conditions found in the
 *  license agreement under which this software has been supplied or provided.
 *  ============================================================================
 */


#if !defined (message_CONFIG_)
#define message_CONFIG_

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

    /*  ----------------------------------- DSP/BIOS Headers            */
#include "matrixcfg.h"
#include <msgq.h>
#include <pool.h>

/*  ----------------------------------- DSP/BIOS LINK Headers       */
#include <dsplink.h>

#if defined (MSGQ_ZCPY_LINK)
#include <zcpy_mqt.h>
#include <sma_pool.h>
#endif /* if defined (MSGQ_ZCPY_LINK) */

#include <stdint.h>
#include "util.h"

/* Name of the MSGQ on the GPP and on the DSP. */
#define GPP_MSGQNAME        "GPPMSGQ1"
#define DSP_MSGQNAME        "DSPMSGQ"

/* ID of the POOL used by matrix. */
#define SAMPLE_POOL_ID      0

//TODO[c]: can be moved to a general *.c & *.h file, to use for both gpp and dsp project
// Message Commands
// NOTE: has te be consistent across both gpp and dsp projects
typedef enum {
    ERROR         = 0x00,
    INIT_FROM_DSP = 0x01,
    MATRIX_A      = 0x02,
    MATRIX_B      = 0x03,
    MATRIX_PROD   = 0x04,
    SHDN          = 0xFF
} CMD_t;

//TODO[c]: can be moved to a general *.c & *.h file, to use for both gpp and dsp project
/* Control message data structure. */
/* Must contain a reserved space for the header */
typedef struct ControlMsg
{
    MSGQ_MsgHeader header;
    Uint16         command;
    uint32_t       size;
    uint32_t       matrix[SIZE_MAXIMUM][SIZE_MAXIMUM];
} ControlMsg;

    /* Messaging buffer used by the application.
     * Note: This buffer must be aligned according to the alignment expected
     * by the device/platform. */
#define APP_BUFFER_SIZE DSPLINK_ALIGN (sizeof (ControlMsg), DSPLINK_BUF_ALIGN)

     /* Number of pools configured in the system. */
#define NUM_POOLS          1

/* Number of local message queues */
#define NUM_MSG_QUEUES     1

/* Number of BUF pools in the entire memory pool */
#define NUM_MSG_POOLS      4

/* Number of messages in each BUF pool. */
#define NUM_MSG_IN_POOL0   1
#define NUM_MSG_IN_POOL1   2
#define NUM_MSG_IN_POOL2   2
#define NUM_MSG_IN_POOL3   4


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#endif /* !defined (message_CONFIG_) */
