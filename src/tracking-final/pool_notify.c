#include <stdlib.h>
#include <stdio.h>

#include <semaphore.h>
/*  ----------------------------------- DSP/BIOS Link                   */
#include <dsplink.h>

/*  ----------------------------------- DSP/BIOS LINK API               */
#include <proc.h>
#include <pool.h>
#include <mpcs.h>
#include <notify.h>


#include <util.h>
#include <util_global_dsp.h>
#if defined (DA8XXGEM)
#include <loaderdefs.h>
#endif

/*  ----------------------------------- Application Header              */
#include "pool_notify.h"
// #include <pool_notify_os.h>


/*  ============================================================================
*  @const   NUM_ARGS
*
*  @desc   Number of arguments specified to the DSP application.
*  ============================================================================
*/
#define NUM_ARGS                       1

/** ============================================================================
*  @name   SAMPLE_POOL_ID
*
*  @desc   ID of the POOL used for the sample.
*  ============================================================================
*/
#define SAMPLE_POOL_ID                 0

/** ============================================================================
*  @name   NUM_BUF_SIZES
*
*  @desc   Number of buffer pools to be configured for the allocator.
*  ============================================================================
*/
#define NUM_BUF_SIZES                  3

/** ============================================================================
*  @const  NUM_BUF_POOL0
*
*  @desc   Number of buffers in first buffer pool.
*  ============================================================================
*/
#define NUM_BUF_POOL0                  1 //Frame sized
#define NUM_BUF_POOL1                  2 //Weight sized
#define NUM_BUF_POOL2                  1 //Model sized

/*  ============================================================================
*  @const   pool_notify_INVALID_ID
*
*  @desc   Indicates invalid processor ID within the pool_notify_Ctrl structure.
*  ============================================================================
*/
#define pool_notify_INVALID_ID            (Uint32) -1

/** ============================================================================
*  @const  pool_notify_IPS_ID
*
*  @desc   The IPS ID to be used for sending notification events to the DSP.
*  ============================================================================
*/
#define pool_notify_IPS_ID                0

/** ============================================================================
*  @const  pool_notify_IPS_EVENTNO
*
*  @desc   The IPS event number to be used for sending notification events to
*          the DSP.
*  ============================================================================
*/
#define pool_notify_IPS_EVENTNO           5


/*  ============================================================================
*  @name   pool_notify_BufferSize
*
*  @desc   Size of buffer to be used for data transfer.
*  ============================================================================
*/


/** ============================================================================
*  @name   pool_notify_DataBuf
*
*  @desc   Pointer to the shared data buffer used by the pool_notify sample
*          application.
*  ============================================================================
*/
Uchar8 * poolFrame = NULL;
float * poolWeight = NULL;
float * poolModel = NULL;
float * poolKernel = NULL;

/** ============================================================================
*  @func   pool_notify_Notify
*
*  @desc   This function implements the event callback registered with the
*          NOTIFY component to receive notification indicating that the DSP-
*          side application has completed its setup phase.
*
*  @arg    eventNo
*              Event number associated with the callback being invoked.
*  @arg    arg
*              Fixed argument registered with the IPS component along with
*              the callback function.
*  @arg    info
*              Run-time information provided to the upper layer by the NOTIFY
*              component. This information is specific to the IPS being
*              implemented.
*
*  @ret    None.
*
*  @enter  None.
*
*  @leave  None.
*
*  @see    None.
*  ============================================================================
*/
STATIC Void pool_notify_Notify(Uint32 eventNo, Pvoid arg, Pvoid info);

sem_t sem;

/** ============================================================================
*  @func   pool_notify_Create
*
*  @desc   This function allocates and initializes resources used by
*          this application.
*
*  @modif  None
*  ============================================================================
*/


NORMAL_API DSP_STATUS pool_notify_Create(IN Char8 *dspExecutable, bufferInit bufferSizes, IN Char8 * strBuffersize)
{
    DSP_STATUS      status = DSP_SOK;
    Uint32          numArgs = NUM_ARGS;
    Void *          dspFrame = NULL;
    Void *          dspWeight = NULL;
    Void *          dspModel = NULL;
    Void *          dspKernel = NULL;
    Uint32          numBufs[NUM_BUF_SIZES] = { NUM_BUF_POOL0,NUM_BUF_POOL1, NUM_BUF_POOL2 };
    Uint32          size[NUM_BUF_SIZES];
    SMAPOOL_Attrs   poolAttrs;
    Char8 *         args[NUM_ARGS];

    sem_init(&sem, 0, 0);

    // Create and initialize the proc object.
    status = PROC_setup(NULL);

    // Attach the Dsp with which the transfers have to be done.
    if (DSP_SUCCEEDED(status))
    {
        status = PROC_attach(PROCESSOR_ID, NULL);
        if (DSP_FAILED(status))
        {
            printf("PROC_attach () failed. Status = [0x%x]\n", (int)status);
        }
    }
    else {
        printf("PROC_setup () failed. Status = [0x%x]\n", (int)status);
    }

    // Open the pool.
    if (DSP_SUCCEEDED(status)) {
        size[0] = bufferSizes.frameAligned;  // Frame
        size[1] = bufferSizes.regionAligned; // Weight and kernel
        size[2] = bufferSizes.modelAligned;  // Targetmodel
        poolAttrs.bufSizes = (Uint32 *)&size;
        poolAttrs.numBuffers = (Uint32 *)&numBufs;
        poolAttrs.numBufPools = NUM_BUF_SIZES;
        poolAttrs.exactMatchReq = FALSE;
        status = POOL_open(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID), &poolAttrs);
        if (DSP_FAILED(status)) {
            printf("POOL_open () failed. Status = [0x%x]\n", (int)status);
        }
    }

    // Allocate the data buffer for the frame to be used for the application.
    if (DSP_SUCCEEDED(status)) {
        status = POOL_alloc(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
            (Void **)&poolFrame,
            bufferSizes.frameAligned);

        // Get the translated DSP address to be sent to the DSP.
        if (DSP_SUCCEEDED(status)) {
            status = POOL_translateAddr(
                POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
                &dspFrame,
                AddrType_Dsp,
                (Void *)poolFrame,
                AddrType_Usr);

            if (DSP_FAILED(status)) {
                printf("POOL_translateAddr () DataBuf Frame failed."
                    " Status = [0x%x]\n",
                    (int)status);
            }
        }
        else {
            printf("POOL_alloc() DataBuf Frame failed. Status = [0x%x]\n", (int)status);
        }
    }

    //Allocate databuffer for targetModel
    if (DSP_SUCCEEDED(status)) {
        status = POOL_alloc(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
            (Void **)&poolModel,
            bufferSizes.modelAligned);

        // Get the translated DSP address to be sent to the DSP.
        if (DSP_SUCCEEDED(status)) {
            status = POOL_translateAddr(
                POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
                &dspModel,
                AddrType_Dsp,
                (Void *)poolModel,
                AddrType_Usr);

            if (DSP_FAILED(status)) {
                printf("POOL_translateAddr () DataBuf model failed."
                    " Status = [0x%x]\n",
                    (int)status);
            }
        }
        else {
            printf("POOL_alloc() DataBuf model failed. Status = [0x%x]\n", (int)status);
        }
    }


    //Allocate databuffer for kernel
    if (DSP_SUCCEEDED(status)) {
        status = POOL_alloc(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
            (Void **)&poolKernel,
            bufferSizes.regionAligned);

        // Get the translated DSP address to be sent to the DSP.
        if (DSP_SUCCEEDED(status)) {
            status = POOL_translateAddr(
                POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
                &dspKernel,
                AddrType_Dsp,
                (Void *)poolKernel,
                AddrType_Usr);

            if (DSP_FAILED(status)) {
                printf("POOL_translateAddr () DataBuf kernel failed."
                    " Status = [0x%x]\n",
                    (int)status);
            }
        }
        else {
            printf("POOL_alloc() DataBuf kernel failed. Status = [0x%x]\n", (int)status);
        }
    }


    //Allocate databuffer for weight
    if (DSP_SUCCEEDED(status)) {
        status = POOL_alloc(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
            (Void **)&poolWeight,
            bufferSizes.regionAligned);

        // Get the translated DSP address to be sent to the DSP.
        if (DSP_SUCCEEDED(status)) {
            status = POOL_translateAddr(
                POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
                &dspWeight,
                AddrType_Dsp,
                (Void *)poolWeight,
                AddrType_Usr);

            if (DSP_FAILED(status)) {
                printf("POOL_translateAddr () DataBuf weight failed."
                    " Status = [0x%x]\n",
                    (int)status);
            }
        }
        else {
            printf("POOL_alloc() DataBuf weight failed. Status = [0x%x]\n", (int)status);
        }
    }

    /*
    *  Register for notification that the DSP-side application setup is
    *  complete.
    */
    if (DSP_SUCCEEDED(status)) {
        status = NOTIFY_register(PROCESSOR_ID,
            pool_notify_IPS_ID,
            pool_notify_IPS_EVENTNO,
            (FnNotifyCbck)pool_notify_Notify,
            0/* vladms XFER_SemPtr*/);
        if (DSP_FAILED(status)) {
            printf("NOTIFY_register () failed Status = [0x%x]\n",
                (int)status);
        }
    }

    // Load the executable on the DSP.
    if (DSP_SUCCEEDED(status)) {
        args[0] = strBuffersize;
        {
            status = PROC_load(PROCESSOR_ID, dspExecutable, numArgs, args);
        }

        if (DSP_FAILED(status)) {
            printf("PROC_load () failed. Status = [0x%x]\n", (int)status);
        }
    }

    // Start execution on DSP.
    if (DSP_SUCCEEDED(status)) {
        status = PROC_start(PROCESSOR_ID);
        if (DSP_FAILED(status)) {
            printf("PROC_start () failed. Status = [0x%x]\n",
                (int)status);
        }
    }


    // Wait for the DSP-side application to indicate that it has completed its
    // setup. The DSP-side application sends notification of the IPS event
    // when it is ready to proceed with further execution of the application.

    printf("Waiting for DSP\n");
    if (DSP_SUCCEEDED(status)) {
        // wait for initialization 
        sem_wait(&sem);
    }

    /*
    *  Send notifications to the DSP with information about the address of the
    *  control structure and data buffer to be used by the application.
    *
    */
    //Transfer frame
    status = NOTIFY_notify(PROCESSOR_ID,
        pool_notify_IPS_ID,
        pool_notify_IPS_EVENTNO,
        (Uint32)dspFrame);
    if (DSP_FAILED(status))
    {
        printf("NOTIFY_notify () DataBuf frame failed."
            " Status = [0x%x]\n",
            (int)status);
    }

    // Transfer model
    status = NOTIFY_notify(PROCESSOR_ID,
        pool_notify_IPS_ID,
        pool_notify_IPS_EVENTNO,
        (Uint32)dspModel);
    if (DSP_FAILED(status))
    {
        printf("NOTIFY_notify () DataBuf model failed."
            " Status = [0x%x]\n",
            (int)status);
    }

    // Transfer weight
    status = NOTIFY_notify(PROCESSOR_ID,
        pool_notify_IPS_ID,
        pool_notify_IPS_EVENTNO,
        (Uint32)dspWeight);
    if (DSP_FAILED(status))
    {
        printf("NOTIFY_notify () DataBuf weight failed."
            " Status = [0x%x]\n",
            (int)status);
    }

    // Transfer kernel
    status = NOTIFY_notify(PROCESSOR_ID,
        pool_notify_IPS_ID,
        pool_notify_IPS_EVENTNO,
        (Uint32)dspKernel);
    if (DSP_FAILED(status))
    {
        printf("NOTIFY_notify () DataBuf kernel failed."
            " Status = [0x%x]\n",
            (int)status);
    }

#ifdef DEBUG
    printf("Leaving pool_notify_Create ()\n");
#endif

    return status;
}

#include <sys/time.h>

long long get_usec(void);

long long get_usec(void)
{
  long long r;
  struct timeval t;
  gettimeofday(&t,NULL);
  r=t.tv_sec*1000000+t.tv_usec;
  return r;
}

/** ============================================================================
 *  @func   pool_notify_Execute
 *
 *  @desc   This function implements the execute phase for this application.
 *
 *  @modif  None
 *  ============================================================================
 */
// NORMAL_API DSP_STATUS pool_notify_Execute(void)
NORMAL_API DSP_STATUS pool_notify_Execute(IN Uint8 info)
{
    DSP_STATUS status = DSP_SOK;

    //TODO[c]: timinglong long start;

    //TODO[c]: timing start = get_usec();

    //TODO[c]: not used/reimplemented
    // #if !defined(DSP)
    // printf(" Result is %d \n", sum_dsp(pool_notify_DataBuf,pool_notify_BufferSize));
    // #endif!

    NOTIFY_notify(PROCESSOR_ID, pool_notify_IPS_ID, pool_notify_IPS_EVENTNO, info);
    // #endif
    //TODO[c]: timingprintf("Sum execution time %lld us.\n", get_usec() - start);

    return status;
}

/** ============================================================================
*  @func   pool_notify_Wait
*
*  @desc   This function implements the execute phase for this application.
*
*  @modif  None
*  ============================================================================
*/
// NORMAL_API DSP_STATUS pool_notify_Wait(void)
NORMAL_API DSP_STATUS pool_notify_Wait()
{
    DSP_STATUS status = DSP_SOK;

#ifdef DEBUG
    printf("Entered pool_notify_Wait ()\n");
#endif
    sem_wait(&sem);

    return status;
}

/** ============================================================================
*  @func   pool_notify_Delete
*
*  @desc   This function releases resources allocated earlier by call to
*          pool_notify_Create ().
*          During cleanup, the allocated resources are being freed
*          unconditionally. Actual applications may require stricter check
*          against return values for robustness.
*
*  @modif  None
*  ============================================================================
*/
NORMAL_API Void pool_notify_Delete(bufferInit bufferSizes)
{
    DSP_STATUS status = DSP_SOK;
    DSP_STATUS tmpStatus = DSP_SOK;

#ifdef DEBUG
    printf("Entered pool_notify_Delete ()\n");
#endif

    /*
    *  Stop execution on DSP.
    */
    status = PROC_stop(PROCESSOR_ID);
    if (DSP_FAILED(status)) {
        printf("PROC_stop () failed. Status = [0x%x]\n", (int)status);
    }

    /*
    *  Unregister for notification of event registered earlier.
    */
    tmpStatus = NOTIFY_unregister(PROCESSOR_ID,
        pool_notify_IPS_ID,
        pool_notify_IPS_EVENTNO,
        (FnNotifyCbck)pool_notify_Notify,
        0/* vladms pool_notify_SemPtr*/);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("NOTIFY_unregister () failed Status = [0x%x]\n",
            (int)status);
    }

    /*
    *  Free the memory allocated for the data buffer.
    */
    tmpStatus = POOL_free(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
        (Void *)poolFrame,
        bufferSizes.frameAligned);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("POOL_free () DataBuf frame failed. Status = [0x%x]\n",
            (int)status);
    }

    tmpStatus = POOL_free(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
        (Void *)poolWeight,
        bufferSizes.regionAligned);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("POOL_free () DataBuf weight failed. Status = [0x%x]\n",
            (int)status);
    }

    tmpStatus = POOL_free(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
        (Void *)poolModel,
        bufferSizes.modelAligned);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("POOL_free () DataBuf model failed. Status = [0x%x]\n",
            (int)status);
    }

    tmpStatus = POOL_free(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID),
        (Void *)poolKernel,
        bufferSizes.regionAligned);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("POOL_free () DataBuf kernel failed. Status = [0x%x]\n",
            (int)status);
    }

    /*
    *  Close the pool
    */
    tmpStatus = POOL_close(POOL_makePoolId(PROCESSOR_ID, SAMPLE_POOL_ID));
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("POOL_close () failed. Status = [0x%x]\n", (int)status);
    }

    /*
    *  Detach from the processor
    */
    tmpStatus = PROC_detach(PROCESSOR_ID);
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("PROC_detach () failed. Status = [0x%x]\n", (int)status);
    }

    /*
    *  Destroy the PROC object.
    */
    tmpStatus = PROC_destroy();
    if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
        status = tmpStatus;
        printf("PROC_destroy () failed. Status = [0x%x]\n", (int)status);
    }

#ifdef DEBUG
    printf("Leaving pool_notify_Delete ()\n");
#endif
}


/** ============================================================================
*  @func   pool_notify_Main
*
*  @desc   Entry point for the application
*
*  @modif  None
*  ============================================================================
*/
NORMAL_API Void pool_notify_Init(IN Char8 *dspExecutable, bufferInit bufferSizes, IN Char8 *strBuffersize)
{
    DSP_STATUS status = DSP_SOK;

#ifdef DEBUG
    printf("========== Sample Application : pool_notify ==========\n");
#endif

    if (dspExecutable == NULL) {
        status = DSP_EINVALIDARG;
        printf("ERROR! Invalid arguments specified for pool_notify application\n");
    }

    //  Validate the buffer size and number of iterations specified.
#ifdef DEBUG
    printf("Allocated a buffer of %d bytes\n", (int)pool_notify_BufferSize);
#endif

    // Specify the dsp executable file name and the buffer size for pool_notify creation phase.
    status = pool_notify_Create(dspExecutable, bufferSizes, strBuffersize);

    if (!DSP_SUCCEEDED(status)) {
        printf("ERROR! pool_notify_Create() failed!\n");
    }
    printf("====================================================\n");
}

/** ----------------------------------------------------------------------------
*  @func   pool_notify_Notify
*
*  @desc   This function implements the event callback registered with the
*          NOTIFY component to receive notification indicating that the DSP-
*          side application has completed its setup phase.
*
*  @modif  None
*  ----------------------------------------------------------------------------
*/
STATIC Void pool_notify_Notify(Uint32 eventNo, Pvoid arg, Pvoid info)
{
#ifdef DEBUG
    printf("Notification %8d \n", (int)info);
#endif
    // Post the semaphore.
    if ((int)info == 0) { // DSP finished
        DEBUGP("Result of DSP is in! \t ");
        sem_post(&sem);
    }
    else {
        printf("ERROR wrong result of DSP = %d\n", (int)info);
    }
}