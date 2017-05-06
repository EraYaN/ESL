/** ============================================================================
 *  @file   message_handler.c
 *
 *  @path
 *
 *  @desc   This is an application which sends messages to the DSP
 *          processor and receives them back using DSP/BIOS LINK.
 *          The message contents received are verified against the data
 *          sent to DSP.
 *
*/
/*  ----------------------------------- DSP/BIOS Link                   */
#include <dsplink.h>

/*  ----------------------------------- DSP/BIOS LINK API               */
#include <proc.h>
#include <msgq.h>
#include <pool.h>

/*  ----------------------------------- Application Header              */
#include "message_handler.h"
#include "system_os.h"

#include <stdio.h>
#include <stdlib.h>


#if defined (__cplusplus)
extern "C"
{
#endif /* defined (__cplusplus) */

    /* Number of arguments specified to the DSP application. */
#define NUM_ARGS 1

    /* Argument size passed to the control message queue */
#define ARG_SIZE 256
#define MATRIX_SIZE 3

    /* ID of the POOL used by matrix. */
#define SAMPLE_POOL_ID  0

    /*  Number of BUF pools in the entire memory pool */
#define NUMMSGPOOLS     4

    /* Number of messages in each BUF pool. */
#define NUMMSGINPOOL0   1
#define NUMMSGINPOOL1   2
#define NUMMSGINPOOL2   2
#define NUMMSGINPOOL3   4

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

// Control message data structure.
// Must contain a reserved space for the header
typedef struct ControlMsg
{
    MSGQ_MsgHeader header;
    Uint16 command;
    Uint32 arg1[MATRIX_SIZE][MATRIX_SIZE];
    Uint32 arg2[MATRIX_SIZE][MATRIX_SIZE];
    Uint32 prod[MATRIX_SIZE][MATRIX_SIZE];
} ControlMsg;

    /* Messaging buffer used by the application.
     * Note: This buffer must be aligned according to the alignment expected
     * by the device/platform. */
#define APP_BUFFER_SIZE DSPLINK_ALIGN (sizeof (ControlMsg), DSPLINK_BUF_ALIGN)

     /* Definitions required for the sample Message queue.
      * Using a Zero-copy based transport on the shared memory physical link. */
#if defined ZCPY_LINK
#define SAMPLEMQT_CTRLMSG_SIZE  ZCPYMQT_CTRLMSG_SIZE
    STATIC ZCPYMQT_Attrs mqtAttrs;
#endif

/* Message sizes managed by the pool */
STATIC Uint32 SampleBufSizes[NUMMSGPOOLS] =
{
    APP_BUFFER_SIZE,
    SAMPLEMQT_CTRLMSG_SIZE,
    DSPLINK_ALIGN(sizeof(MSGQ_AsyncLocateMsg), DSPLINK_BUF_ALIGN),
    DSPLINK_ALIGN(sizeof(MSGQ_AsyncErrorMsg), DSPLINK_BUF_ALIGN)
};

/* Number of messages in each pool */
STATIC Uint32 SampleNumBuffers[NUMMSGPOOLS] =
{
    NUMMSGINPOOL0,
    NUMMSGINPOOL1,
    NUMMSGINPOOL2,
    NUMMSGINPOOL3
};

    /* Definition of attributes for the pool based on physical link used by the transport */
#if defined ZCPY_LINK
    STATIC SMAPOOL_Attrs SamplePoolAttrs =
    {
        NUMMSGPOOLS,
        SampleBufSizes,
        SampleNumBuffers,
        TRUE   /* If allocating a buffer smaller than the POOL size, set this to FALSE */
    };
#endif

    /* Name of the first MSGQ on the GPP and on the DSP. */
    STATIC Char8 SampleGppMsgqName[DSP_MAX_STRLEN] = "GPPMSGQ1";
    STATIC Char8 SampleDspMsgqName[DSP_MAX_STRLEN] = "DSPMSGQ";

    /* Local GPP's and DSP's MSGQ Objects. */
    STATIC MSGQ_Queue SampleGppMsgq = (Uint32)MSGQ_INVALIDMSGQ;
    STATIC MSGQ_Queue SampleDspMsgq = (Uint32)MSGQ_INVALIDMSGQ;

    /* Place holder for the MSGQ name created on DSP */
    Char8 dspMsgqName[DSP_MAX_STRLEN];

    /* Extern declaration to the default DSP/BIOS LINK configuration structure. */
    extern LINKCFG_Object LINKCFG_config;

#if defined (VERIFY_DATA)
    /** ============================================================================
     *  @func   messagehandler_VerifyData
     *
     *  @desc   This function verifies the data-integrity of given message.
     *  ============================================================================
     */
    STATIC NORMAL_API DSP_STATUS messagehandler_VerifyData(IN MSGQ_Msg msg, IN Uint16 sequenceNumber);
#endif

//TODO[c]: no dynamic allocation
void MatrixPrint(Uint32 matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int i,j;
    for(i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++)
        {
            // SYSTEM_1Print(" %d",8);
            SYSTEM_1Print(" %d", matrix[i][j]);
            // SYSTEM_1Print(" %d",matrix[i][j]);
        }
        SYSTEM_0Print("\n");
    }
}

int **mat1, **mat2, **prod;

// Dynamically Allocated Matrix Init
void MatrixInit(int **matrix, int size, int salt) {
    int i,j;

    for(i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            matrix[i][j] = i+j*salt;
        }
    }
}

// Dynamically Allocated Matrix print
void MatrixPrint_new(int **matrix, int size) {
    int i,j;
    for(i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            // SYSTEM_1Print(" %d",8);
            SYSTEM_1Print(" %d", matrix[i][j]);
            // SYSTEM_1Print(" %d",matrix[i][j]);
        }
        SYSTEM_0Print("\n");
    }
}

// Dynamically Allocated Matrix Multiply
void MatrixMultiply(int **a, int **b, int **c, int size) {
    int i, j, k;
    for (i = 0;i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            c[i][j]=0;
            for(k=0;k<size;k++)
                c[i][j] = c[i][j]+a[i][k] * b[k][j];
        }
    }
}

/** ============================================================================
 *  @func   messagehandler_Create
 *
 *  @desc   This function allocates and initializes resources used by
 *          this application.
 *
 *  @modif  messagehandler_InpBufs , messagehandler_OutBufs
 *  ============================================================================
 */
NORMAL_API DSP_STATUS messagehandler_Create(IN Char8* dspExecutable, IN Char8* strNumIterations, IN Uint8 processorId)
{
    DSP_STATUS status = DSP_SOK;
    Uint32 numArgs = NUM_ARGS;
    MSGQ_LocateAttrs syncLocateAttrs;
    Char8* args[NUM_ARGS];

    SYSTEM_0Print("Entered messagehandler_Create ()\n");

    /* Create and initialize the proc object. */
    status = PROC_setup(NULL);

    /* Attach the Dsp with which the transfers have to be done. */
    if (DSP_SUCCEEDED(status))
    {
        status = PROC_attach(processorId, NULL);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("PROC_attach () failed. Status = [0x%x]\n", status);
        }
    }

    /* Open the pool. */
    if (DSP_SUCCEEDED(status))
    {
        status = POOL_open(POOL_makePoolId(processorId, SAMPLE_POOL_ID), &SamplePoolAttrs);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("POOL_open () failed. Status = [0x%x]\n", status);
        }
    }
    else
    {
        SYSTEM_1Print("PROC_setup () failed. Status = [0x%x]\n", status);
    }

    /* Open the GPP's message queue */
    if (DSP_SUCCEEDED(status))
    {
        status = MSGQ_open(SampleGppMsgqName, &SampleGppMsgq, NULL);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("MSGQ_open () failed. Status = [0x%x]\n", status);
        }
    }

    /* Set the message queue that will receive any async. errors */
    if (DSP_SUCCEEDED(status))
    {
        status = MSGQ_setErrorHandler(SampleGppMsgq, POOL_makePoolId(processorId, SAMPLE_POOL_ID));
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("MSGQ_setErrorHandler () failed. Status = [0x%x]\n", status);
        }
    }

    /* Load the executable on the DSP. */
    if (DSP_SUCCEEDED(status))
    {
        args[0] = strNumIterations;
        {
            status = PROC_load(processorId, dspExecutable, numArgs, args);
        }
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("PROC_load () failed. Status = [0x%x]\n", status);
        }
    }

    /* Start execution on DSP. */
    if (DSP_SUCCEEDED(status))
    {
        status = PROC_start(processorId);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("PROC_start () failed. Status = [0x%x]\n", status);
        }
    }

    /* Open the remote transport. */
    if (DSP_SUCCEEDED(status))
    {
        mqtAttrs.poolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
        status = MSGQ_transportOpen(processorId, &mqtAttrs);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("MSGQ_transportOpen () failed. Status = [0x%x]\n", status);
        }
    }

    /* Locate the DSP's message queue */
    /* At this point the DSP must open a message queue named "DSPMSGQ" */
    if (DSP_SUCCEEDED(status))
    {
        syncLocateAttrs.timeout = WAIT_FOREVER;
        status = DSP_ENOTFOUND;
        SYSTEM_2Sprint(dspMsgqName, "%s%d", (Uint32)SampleDspMsgqName, processorId);
        while ((status == DSP_ENOTFOUND) || (status == DSP_ENOTREADY))
        {
            status = MSGQ_locate(dspMsgqName, &SampleDspMsgq, &syncLocateAttrs);
            if ((status == DSP_ENOTFOUND) || (status == DSP_ENOTREADY))
            {
                SYSTEM_Sleep(100000);
            }
            else if (DSP_FAILED(status))
            {
                SYSTEM_1Print("MSGQ_locate () failed. Status = [0x%x]\n", status);
            }
        }
    }

    SYSTEM_0Print("Leaving messagehandler_Create ()\n");
    return status;
}


/** ============================================================================
 *  @func   messagehandler_Execute
 *
 *  @desc   This function implements the execute phase for this application.
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API DSP_STATUS messagehandler_Execute(IN Uint32 numIterations, Uint8 processorId)
{
    DSP_STATUS  status = DSP_SOK;
    Uint16 sequenceNumber = 0;
    Uint16 msgId = 0;
    Uint16 communicating = 1;
    Uint16 i,j;
    ControlMsg *msg;

    SYSTEM_0Print("Entered messagehandler_Execute ()\n");

    while (communicating){
        // Receive the message.
        status = MSGQ_get(SampleGppMsgq, WAIT_FOREVER, (MsgqMsg *)&msg);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("MSGQ_get () failed. Status = [0x%x]\n", status);
        }
        #if defined (VERIFY_DATA)
            /* Verify correctness of data received. */
            if (DSP_SUCCEEDED(status))
            {
                status = messagehandler_VerifyData(msg, sequenceNumber);
                if (DSP_FAILED(status))
                {
                    MSGQ_free((MsgqMsg)msg);
                }
            }
        #endif

        switch(msg->command) {
            default: { 
                break; 
            }
            case ERROR: {
                SYSTEM_0Print("ERROR msg received\n");
                communicating =0; // no response expected, #LEAVING!
                break;
            }
            case INIT_FROM_DSP: {
                SYSTEM_0Print("DSP awake!\n");
                // Now fill message with matrix A and B
                msg->command = MATRIX_A;
                for (i = 0; i < MATRIX_SIZE; i++) {
                    for (j = 0; j < MATRIX_SIZE; j++) {
                        msg->arg1[i][j] = i+j*2;
                        msg->arg2[i][j] = i+j*3;
                    }
                }
                #if defined (PROFILE)
                    SYSTEM_GetStartTime();
                #endif
                break;
            }
            case MATRIX_A: {
                SYSTEM_0Print("ERROR! matrix A received, should be send only\n");
                msg->command = SHDN; // ERROR, so shutdown
                break;
            }
            case MATRIX_B: {
                SYSTEM_0Print("ERROR! matrix B received, should be send only\n");
                msg->command = SHDN; // ERROR, so shutdown
                break;
            }
            case MATRIX_PROD: {
                #if defined (PROFILE)
                    SYSTEM_GetEndTime();
                #endif
                SYSTEM_0Print("product received:\n");
                MatrixPrint(msg->prod);
                communicating = 0; // done sending, #LEAVING!
                break;
            }
            case SHDN: {
                msg->command = SHDN; // ERROR, so shutdown
                break;
            }
        }

        // stop communicating, so do not send any more messages
        if (!communicating){
            MSGQ_free((MsgqMsg)msg);
            break;
        }

        // Send the same message received in earlier MSGQ_get() call.
        //TODO[c]: checking status here is a bit weird right?
        if (DSP_SUCCEEDED(status))
        {
            msgId = MSGQ_getMsgId(msg);
            MSGQ_setMsgId(msg, msgId);
            status = MSGQ_put(SampleDspMsgq, (MsgqMsg)msg);
            if (DSP_FAILED(status))
            {
                MSGQ_free((MsgqMsg)msg);
                SYSTEM_1Print("MSGQ_put () failed. Status = [0x%x]\n", status);
            }
        }

        sequenceNumber++;
        // Make sure that the sequenceNumber stays within the permitted range for applications.
        if (sequenceNumber == MSGQ_INTERNALIDSSTART)
        {
            sequenceNumber = 0;
        }

        #if !defined (PROFILE)
            if (DSP_SUCCEEDED(status) && ((i % 100) == 0))
            {
                SYSTEM_1Print("Transferred %ld messages\n", i);
            }
        #endif
    }

    #if defined (PROFILE)
        if (DSP_SUCCEEDED(status))
        {
            SYSTEM_GetProfileInfo();
        }
    #endif

    SYSTEM_0Print("Leaving messagehandler_Execute ()\n");

    return status;
}


/** ============================================================================
 *  @func   messagehandler_Delete
 *
 *  @desc   This function releases resources allocated earlier by call to
 *          messagehandler_Create ().
 *          During cleanup, the allocated resources are being freed
 *          unconditionally. Actual applications may require stricter check
 *          against return values for robustness.
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API Void messagehandler_Delete(Uint8 processorId)
{
    DSP_STATUS status = DSP_SOK;
    DSP_STATUS tmpStatus = DSP_SOK;

    SYSTEM_0Print("Entered messagehandler_Delete ()\n");

    /* Release the remote message queue */
    status = MSGQ_release(SampleDspMsgq);
    if (DSP_FAILED(status))
    {
        SYSTEM_1Print("MSGQ_release () failed. Status = [0x%x]\n", status);
    }

    /* Close the remote transport */
    tmpStatus = MSGQ_transportClose(processorId);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("MSGQ_transportClose () failed. Status = [0x%x]\n", status);
    }

    /* Stop execution on DSP. */
    tmpStatus = PROC_stop(processorId);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("PROC_stop () failed. Status = [0x%x]\n", status);
    }

    /* Reset the error handler before deleting the MSGQ that receives */
    /* the error messages.                                            */
    tmpStatus = MSGQ_setErrorHandler(MSGQ_INVALIDMSGQ, MSGQ_INVALIDMSGQ);

    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("MSGQ_setErrorHandler () failed. Status = [0x%x]\n", status);
    }

    /* Close the GPP's message queue */
    tmpStatus = MSGQ_close(SampleGppMsgq);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("MSGQ_close () failed. Status = [0x%x]\n", status);
    }

    /* Close the pool */
    tmpStatus = POOL_close(POOL_makePoolId(processorId, SAMPLE_POOL_ID));
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("POOL_close () failed. Status = [0x%x]\n", status);
    }

    /* Detach from the processor */
    tmpStatus = PROC_detach(processorId);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("PROC_detach () failed. Status = [0x%x]\n", status);
    }

    /* Destroy the PROC object. */
    tmpStatus = PROC_destroy();
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
    {
        status = tmpStatus;
        SYSTEM_1Print("PROC_destroy () failed. Status = [0x%x]\n", status);
    }

    SYSTEM_0Print("Leaving messagehandler_Delete ()\n");
}


/** ============================================================================
 *  @func   messagehandler_Main
 *
 *  @desc   Entry point for the application
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API Void messagehandler_Main(IN Char8* dspExecutable, IN Char8* strNumIterations, IN Char8* strProcessorId)
{
    DSP_STATUS status = DSP_SOK;
    Uint32 numIterations = 0;
    Uint8 processorId = 0;
    Uint32 i;

    SYSTEM_0Print("========== Sample Application : matrix ==========\n");

    if ((dspExecutable != NULL) && (strNumIterations != NULL))
    {
        numIterations = SYSTEM_Atoi(strNumIterations);

        if (numIterations > 0xFFFF)
        {
            status = DSP_EINVALIDARG;
            SYSTEM_1Print("ERROR! Invalid arguments specified for matrix application.\n Max iterations = %d\n", 0xFFFF);
        }
        else
        {

                mat1 = malloc(MATRIX_SIZE * sizeof *mat1);
                for (i = 0; i < MATRIX_SIZE; i++)
                {
                    mat1[i] = malloc(MATRIX_SIZE * sizeof *mat1[i]);
                }
                mat2 = malloc(MATRIX_SIZE * sizeof *mat2);
                for (i = 0; i < MATRIX_SIZE; i++)
                {
                    mat2[i] = malloc(MATRIX_SIZE * sizeof *mat2[i]);
                }
                prod = malloc(MATRIX_SIZE * sizeof *prod);
                for (i = 0; i < MATRIX_SIZE; i++)
                {
                    prod[i] = malloc(MATRIX_SIZE * sizeof *prod[i]);
                }

                MatrixInit(mat1,MATRIX_SIZE,2);
                MatrixInit(mat2,MATRIX_SIZE,3);

                MatrixMultiply(mat1,mat2,prod,MATRIX_SIZE);

                SYSTEM_0Print("mat1:\n");
                MatrixPrint_new(mat1,MATRIX_SIZE);
                SYSTEM_0Print("mat2:\n");
                MatrixPrint_new(mat2,MATRIX_SIZE);
                SYSTEM_0Print("prod:\n");
                MatrixPrint_new(prod,MATRIX_SIZE);


            processorId = SYSTEM_Atoi(strProcessorId);

            if (processorId >= MAX_DSPS)
            {
                SYSTEM_1Print("== Error: Invalid processor id %d specified ==\n", processorId);
                status = DSP_EFAIL;
            }
            /* Specify the dsp executable file name for message creation phase. */
            if (DSP_SUCCEEDED(status))
            {
                status = messagehandler_Create(dspExecutable, strNumIterations, processorId);

                /* Execute the message execute phase. */
                if (DSP_SUCCEEDED(status))
                {
                    status = messagehandler_Execute(numIterations, processorId);
                }

                /* Perform cleanup operation. */
                messagehandler_Delete(processorId);
            }
        }
    }
    else
    {
        status = DSP_EINVALIDARG;
        SYSTEM_0Print("ERROR! Invalid arguments specified for matrix application\n");
    }
    SYSTEM_0Print("====================================================\n");
}

#if defined (VERIFY_DATA)
/** ============================================================================
 *  @func   messagehandler_VerifyData
 *
 *  @desc   This function verifies the data-integrity of given buffer.
 *
 *  @modif  None
 *  ============================================================================
 */
STATIC NORMAL_API DSP_STATUS messagehandler_VerifyData(IN MSGQ_Msg msg, IN Uint16 sequenceNumber)
{
    DSP_STATUS status = DSP_SOK;
    Uint16 msgId;

    /* Verify the message */
    msgId = MSGQ_getMsgId(msg.header);
    if (msgId != sequenceNumber)
    {
        status = DSP_EFAIL;
        SYSTEM_0Print("ERROR! Data integrity check failed\n");
    }

    return status;
}
#endif /* if defined (VERIFY_DATA) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */