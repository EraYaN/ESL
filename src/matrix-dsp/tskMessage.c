/** ============================================================================
 *  @file   tskMessage.c
 *
 *  @path
 *
 *  @desc   This is simple TSK based application that uses MSGQ. It receives
 *          and transmits messages from/to the GPP and runs the DSP
 *          application code (located in an external source file)
 *
 *  @ver    1.10
 */


 /*  ----------------------------------- DSP/BIOS Headers            */
#include "matrixcfg.h"
#include <gbl.h>
#include <sys.h>
#include <sem.h>
#include <msgq.h>
#include <pool.h>

/*  ----------------------------------- DSP/BIOS LINK Headers       */
#include <dsplink.h>
#include <platform.h>
#include <failure.h>

//malloc()
#include <stdlib.h>

/*  ----------------------------------- Sample Headers              */
#include "message_config.h"
#include "tskMessage.h"

#ifdef __cplusplus
extern "C" {
#endif


/* FILEID is used by SET_FAILURE_REASON macro. */
#define FILEID  FID_APP_C

/* Place holder for the MSGQ name created on DSP */
Uint8 dspMsgQName[DSP_MAX_STRLEN];

/* Number of iterations message transfers to be done by the application. */
extern Uint16 numTransfers;

// Dynamically Allocated Matrix
void matrixAlloc(uint32_t ***matrix, uint32_t size) {
    uint32_t i;
    *matrix = malloc(size * sizeof(**matrix));
    if (*matrix == NULL) {
        SET_FAILURE_REASON(888); //TODO[c]: wat is deze?! werkt dit goed (genoeg)
    }
    for (i = 0; i < size; i++) {
        (*matrix)[i] = malloc(size * sizeof(*(*matrix)[i]));
        if ((*matrix)[i] == NULL) {
            SET_FAILURE_REASON(888); //TODO[c]: wat is deze?! werkt dit goed (genoeg)
        }
    }
}

void matrixMultiplyDSP(uint32_t ***mat1, uint32_t ***mat2, Uint32 prod[SIZE_MAXIMUM][SIZE_MAXIMUM], Uint32 size) {
    int i, j, k;
    for (i = 0;i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            // prod[i][j]=0; //Already empty :) (cleared my GPP, out of timed section)
            for(k = 0; k < size; k++)
                prod[i][j] = prod[i][j] + (*mat1)[i][k] * (*mat2)[k][j];
        }
    }
}

/** ============================================================================
 *  @func   TSKMESSAGE_create
 *
 *  @desc   Create phase function for the TSKMESSAGE application. Initializes
 *          the TSKMESSAGE_TransferInfo structure with the information that will
 *          be used by the other phases of the application.
 *
 *  @modif  None.
 *  ============================================================================
 */
Int TSKMESSAGE_create(TSKMESSAGE_TransferInfo** infoPtr)
{
    Int status = SYS_OK;
    MSGQ_Attrs msgqAttrs = MSGQ_ATTRS;
    TSKMESSAGE_TransferInfo* info = NULL;
    MSGQ_LocateAttrs syncLocateAttrs;

    /* Allocate TSKMESSAGE_TransferInfo structure that will be initialized
     * and passed to other phases of the application */
    *infoPtr = MEM_calloc(DSPLINK_SEGID, sizeof(TSKMESSAGE_TransferInfo), DSPLINK_BUF_ALIGN);
    if (*infoPtr == NULL)
    {
        status = SYS_EALLOC;
        SET_FAILURE_REASON(status);
    }
    else
    {
        info = *infoPtr;
        info->numTransfers = numTransfers;
        info->localMsgq = MSGQ_INVALIDMSGQ;
        info->locatedMsgq = MSGQ_INVALIDMSGQ;
    }

    if (status == SYS_OK)
    {
        /* Set the semaphore to a known state. */
        SEM_new(&(info->notifySemObj), 0);

        /* Fill in the attributes for this message queue. */
        msgqAttrs.notifyHandle = &(info->notifySemObj);
        msgqAttrs.pend = (MSGQ_Pend)SEM_pendBinary;
        msgqAttrs.post = (MSGQ_Post)SEM_postBinary;

        SYS_sprintf((Char *)dspMsgQName, "%s%d", DSP_MSGQNAME, GBL_getProcId());

        /* Creating message queue */
        status = MSGQ_open((String)dspMsgQName, &info->localMsgq, &msgqAttrs);
        if (status != SYS_OK)
        {
            SET_FAILURE_REASON(status);
        }
        else
        {
            /* Set the message queue that will receive any async. errors. */
            MSGQ_setErrorHandler(info->localMsgq, SAMPLE_POOL_ID);

            /* Synchronous locate.                           */
            /* Wait for the initial startup message from GPP */
            status = SYS_ENOTFOUND;
            while ((status == SYS_ENOTFOUND) || (status == SYS_ENODEV))
            {
                syncLocateAttrs.timeout = SYS_FOREVER;
                status = MSGQ_locate(GPP_MSGQNAME, &info->locatedMsgq, &syncLocateAttrs);
                if ((status == SYS_ENOTFOUND) || (status == SYS_ENODEV))
                {
                    TSK_sleep(1000);
                }
                else if (status != SYS_OK)
                {
                    #if !defined (LOG_COMPONENT)
                        LOG_printf(&trace, "MSGQ_locate (msgqOut) failed. Status = 0x%x\n", status);
                    #endif
                }
            }
        }
        /* Initialize the sequenceNumber */
        info->sequenceNumber = 0;
    }

    return status;
}


/** ============================================================================
 *  @func   TSKMESSAGE_execute
 *
 *  @desc   Execute phase function for the TSKMESSAGE application. Application
 *          receives a message, verifies the id and executes the DSP processing.
 *
 *  @modif  None.
 *  ============================================================================
 */
Int TSKMESSAGE_execute(TSKMESSAGE_TransferInfo* info)
{
    Int status = SYS_OK;
    ControlMsg* msg;
    Uint16 communicating = 1;

    uint32_t **mat1, **mat2;
    uint32_t size;
    uint32_t i, j;

    /* Allocate and send the message */
    status = MSGQ_alloc(SAMPLE_POOL_ID, (MSGQ_Msg*)&msg, APP_BUFFER_SIZE);

    if (status == SYS_OK)
    {
        MSGQ_setMsgId((MSGQ_Msg)msg, info->sequenceNumber);
        MSGQ_setSrcQueue((MSGQ_Msg)msg, info->localMsgq);
        msg->command = INIT_FROM_DSP;
        // SYS_sprintf(msg->arg1, "DSP is awake!");

        status = MSGQ_put(info->locatedMsgq, (MSGQ_Msg)msg);
        if (status != SYS_OK)
        {
            /* Must free the message */
            MSGQ_free((MSGQ_Msg)msg);
            SET_FAILURE_REASON(status);
        }
    }
    else
    {
        SET_FAILURE_REASON(status);
    }

    while (communicating){
        /* Receive a message from the GPP */
        status = MSGQ_get(info->localMsgq, (MSGQ_Msg*)&msg, SYS_FOREVER);
        if (status == SYS_OK)
        {
            /* Check if the message is an asynchronous error message */
            if (MSGQ_getMsgId((MSGQ_Msg)msg) == MSGQ_ASYNCERRORMSGID)
            {
                #if !defined (LOG_COMPONENT)
                    LOG_printf(&trace, "Transport error Type = %d", ((MSGQ_AsyncErrorMsg *)msg)->errorType);
                #endif
                /* Must free the message */
                MSGQ_free((MSGQ_Msg)msg);
                status = SYS_EBADIO;
                SET_FAILURE_REASON(status);
            }
            /* Check if the message received has the correct sequence number */
            else if (MSGQ_getMsgId((MSGQ_Msg)msg) != info->sequenceNumber)
            {
                #if !defined (LOG_COMPONENT)
                    LOG_printf(&trace, "Out of sequence message!");
                #endif
                MSGQ_free((MSGQ_Msg)msg);
                status = SYS_EBADIO;
                SET_FAILURE_REASON(status);
            }
            else
            {
                /* Include your control flag or processing code here */
                switch(msg->command) {
                    default: { 
                        break; 
                    }
                    case ERROR: {
                        msg->command = ERROR;
                        communicating =0; // no response expected, #LEAVING!
                        break;
                    }
                    case INIT_FROM_DSP: {
                        msg->command = ERROR;
                        communicating =0; // no response expected, #LEAVING!
                        break;
                    }
                    case MATRIX_A: {
                        //TODO[c]: maybe do a consistency check (for size) for all messages received would be nice?
                        size = msg->size;
                        matrixAlloc(&mat1, size);
                        for (i = 0; i < size; i++) {
                            for (j = 0; j < size; j++) {
                                //TODO[c]: implement using memcpy or something
                                mat1[i][j] = msg->matrix[i][j];
                            }
                        }
                        //memcpy matrix A
                        msg->command = MATRIX_A; //ACK matrix A, (request B)
                        break;
                    }
                    case MATRIX_B: {
                        matrixAlloc(&mat2, msg->size);
                        for (i = 0; i < size; i++) {
                            for (j = 0; j < size; j++) {
                                //TODO[c]: implement using memcpy or something
                                mat2[i][j] = msg->matrix[i][j];
                            }
                        }
                        msg->command = MATRIX_B; //ACK matrix B, (request go for calculation)
                        break;
                    }
                    case MATRIX_PROD: {
                        msg->command = MATRIX_PROD;
                        matrixMultiplyDSP(&mat1,&mat2,(msg->matrix),msg->size);
                        communicating =0; // no response expected, #LEAVING!
                        break;
                    }
                    case SHDN: {
                        msg->command = ERROR;
                        communicating =0; // no response expected, #LEAVING!
                        break;
                    }
                }

                /* Increment the sequenceNumber for next received message */
                info->sequenceNumber++;
                /* Make sure that sequenceNumber stays within the range of iterations */
                if (info->sequenceNumber == MSGQ_INTERNALIDSSTART)
                {
                    info->sequenceNumber = 0;
                }
                MSGQ_setMsgId((MSGQ_Msg)msg, info->sequenceNumber);
                MSGQ_setSrcQueue((MSGQ_Msg)msg, info->localMsgq);

                /* Send the message back to the GPP */
                status = MSGQ_put(info->locatedMsgq, (MSGQ_Msg)msg);
                if (status != SYS_OK)
                {
                    SET_FAILURE_REASON(status);
                }
            }
        }
        else
        {
            SET_FAILURE_REASON(status);
        }
    }
    return status;
}


/** ============================================================================
 *  @func   TSKMESSAGE_delete
 *
 *  @desc   Delete phase function for the TSKMESSAGE application. It deallocates
 *          all the resources of allocated during create phase of the
 *          application.
 *
 *  @modif  None.
 *  ============================================================================
 */
Int TSKMESSAGE_delete(TSKMESSAGE_TransferInfo* info)
{
    Int status = SYS_OK;
    Int tmpStatus = SYS_OK;
    Bool freeStatus = FALSE;

    /* Release the located message queue */
    if (info->locatedMsgq != MSGQ_INVALIDMSGQ)
    {
        status = MSGQ_release(info->locatedMsgq);
        if (status != SYS_OK)
        {
            SET_FAILURE_REASON(status);
        }
    }

    /* Reset the error handler before deleting the MSGQ that receives */
    /* the error messages.                                            */
    MSGQ_setErrorHandler(MSGQ_INVALIDMSGQ, POOL_INVALIDID);

    /* Delete the message queue */
    if (info->localMsgq != MSGQ_INVALIDMSGQ)
    {
        tmpStatus = MSGQ_close(info->localMsgq);
        if ((status == SYS_OK) && (tmpStatus != SYS_OK))
        {
            status = tmpStatus;
            SET_FAILURE_REASON(status);
        }
    }

    /* Free the info structure */
    freeStatus = MEM_free(DSPLINK_SEGID, info, sizeof(TSKMESSAGE_TransferInfo));
    if ((status == SYS_OK) && (freeStatus != TRUE))
    {
        status = SYS_EFREE;
        SET_FAILURE_REASON(status);
    }
    return status;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
