/*  ----------------------------------- DSP/BIOS Headers            */
#include <std.h>
#include <gbl.h>
#include <log.h>
#include <swi.h>
#include <sys.h>
#include <tsk.h>
#include <pool.h>

/*  ----------------------------------- DSP/BIOS LINK Headers       */
#include <failure.h>
#include <dsplink.h>
#include <platform.h>
#include <notify.h>
#include <bcache.h>
/*  ----------------------------------- Sample Headers              */
#include <pool_notify_config.h>
#include <task.h>

#include <util.h>
#include <util_global_dsp.h> 
#include <math.h>


unsigned char* frame;
float * weight;
float * model;
float * candidate;
int weightsize;
int modelsize;

extern Uint16 MPCSXFER_BufferSize;

static Void Task_notify (Uint32 eventNo, Ptr arg, Ptr info);

Int Task_create(Task_TransferInfo ** infoPtr)
{
    Int status    = SYS_OK;
    Task_TransferInfo * info = NULL;

    /* Allocate Task_TransferInfo structure that will be initialized
     * and passed to other phases of the application */
    if (status == SYS_OK) {
        *infoPtr = MEM_calloc (DSPLINK_SEGID,
                               sizeof (Task_TransferInfo),
                               0); /* No alignment restriction */
        if (*infoPtr == NULL) {
            status = SYS_EALLOC;
        } else {
            info = *infoPtr;
        }
    }

    /* Fill up the transfer info structure */
    if (status == SYS_OK) {
        info->dataBuf       = NULL; /* Set through notification callback. */
        info->bufferSize    = MPCSXFER_BufferSize;
        SEM_new(&(info->notifySemObj), 0);
    }

    /*
     *  Register notification for the event callback to get control and data
     *  buffer pointers from the GPP-side.
     */
    if (status == SYS_OK) {
        status = NOTIFY_register (ID_GPP,
                                  MPCSXFER_IPS_ID,
                                  MPCSXFER_IPS_EVENTNO,
                                  (FnNotifyCbck) Task_notify,
                                  info);
        if (status != SYS_OK) {
            return status;
        }
    }

    /*
     *  Send notification to the GPP-side that the application has completed its
     *  setup and is ready for further execution.
     */
    if (status == SYS_OK) {
        status = NOTIFY_notify (ID_GPP,
                                MPCSXFER_IPS_ID,
                                MPCSXFER_IPS_EVENTNO,
                                (Uint32) 0); /* No payload to be sent. */
        if (status != SYS_OK) {
            return status;
        }
    }

    modelsize = 48 * sizeof(float);
    weightsize = RECT_SIZE * sizeof(float);

    /*
     *  Wait for the event callback from the GPP-side to post the semaphore
     *  indicating receipt of the data buffer pointer and image width and height.
     */
    SEM_pend(&(info->notifySemObj), SYS_FOREVER);
    SEM_pend(&(info->notifySemObj), SYS_FOREVER);
    SEM_pend(&(info->notifySemObj), SYS_FOREVER);
    SEM_pend(&(info->notifySemObj), SYS_FOREVER);

    return status;
}

// unsigned char* buf;
int communicating = 1;

//TODO[c]: not used anymore
/*int sum_dsp() 
{
    int sum = 0,i;
    for (i=0;i<length;i++) {
       sum = sum + buf[i];
    }
    return sum;
}*/


void CalWeight(unsigned char *restrict frame, float *restrict weight, float *restrict candidate, float *restrict model)
{
    float multipliers[NUM_BINS];
    int x=0, y=0, xy=0, bin;
    unsigned char curr_pixel;

#pragma MUST_ITERATE(NUM_BINS, NUM_BINS,)
#pragma UNROLL(NUM_BINS)
    for (bin = 0; bin < NUM_BINS; bin++) {
        multipliers[bin] = sqrt(model[bin]/candidate[bin]);
    }

    _nassert((int)frame % 8 == 0); // input1 is 64-bit aligned
    _nassert((int)weight % 8 == 0); // input2 is 64-bit aligned
    _nassert((int)multipliers % 8 == 0); // output is 64-bit aligned
#pragma MUST_ITERATE(RECT_HEIGHT*RECT_WIDTH, RECT_HEIGHT*RECT_WIDTH,)
    for (xy = 0; xy < RECT_HEIGHT*RECT_WIDTH; xy++) {
        curr_pixel = frame[y*RECT_WIDTH+x];
        weight[y*RECT_WIDTH+x] = multipliers[curr_pixel>>4];
        if (++x == RECT_WIDTH)
        {
            x = 0;
            y = y + 1;
        }
        
    }
}

Int Task_execute(Task_TransferInfo * info)
{
    //TODO[c]: not used anymore
    //int sum;
    // Uint16 k;

    static int counter = 1;

    // for (k = 0; k < 3*32; k++) {
    while(1) {
        if(communicating){
            //wait for semaphore
            SEM_pend(&(info->notifySemObj), SYS_FOREVER);

            //invalidate cache
            BCACHE_inv((Ptr)frame, RECT_SIZE, TRUE);
            BCACHE_inv((Ptr)model, modelsize, TRUE);
            BCACHE_inv((Ptr)weight, weightsize, TRUE);
            BCACHE_inv((Ptr)candidate, modelsize, TRUE);

            //call the functionality to be performed by dsp
            //sum = sum_dsp();
            CalWeight(frame, weight, candidate, model);

            BCACHE_wbInv((Ptr)weight, weightsize, TRUE);

            //notify that we are done
            NOTIFY_notify(ID_GPP,MPCSXFER_IPS_ID,MPCSXFER_IPS_EVENTNO, (Uint32)0);

            //notify the result
            NOTIFY_notify(ID_GPP,MPCSXFER_IPS_ID,MPCSXFER_IPS_EVENTNO, counter++);
            // }
            communicating--;
        }
    }
    return SYS_OK;
}

Int Task_delete(Task_TransferInfo * info)
{
    Int    status     = SYS_OK;
    /*
     *  Unregister notification for the event callback used to get control and
     *  data buffer pointers from the GPP-side.
     */
    status = NOTIFY_unregister (ID_GPP,
                                MPCSXFER_IPS_ID,
                                MPCSXFER_IPS_EVENTNO,
                                (FnNotifyCbck) Task_notify,
                                info);

    /* Free the info structure */
    MEM_free (DSPLINK_SEGID,
              info,
              sizeof (Task_TransferInfo));
    info = NULL;

    return status;
}

static Void Task_notify(Uint32 eventNo, Ptr arg, Ptr info)
{
    static int count = 0;
    Task_TransferInfo * mpcsInfo = (Task_TransferInfo *)arg;

    (Void)eventNo; /* To avoid compiler warning. */

    count++;
    if (count == 1) {
        frame = (unsigned char*)info;
    }
    if (count == 2) {
        model = (float*)info;
    }
    if (count == 3) {
        weight = (float*)info;
    }
    if (count == 4) {
        candidate = (float *)info;
    }
    else if (count>4) {
        // communicating = ((count -3) == (int)info);
        communicating += (int)info;
    }

    SEM_post(&(mpcsInfo->notifySemObj));
}
