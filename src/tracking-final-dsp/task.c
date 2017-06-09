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

//Declaration of pointers to shared memory
unsigned char *frame;
float *weight;
float *model;
//float candidate[CFG_NUM_BINS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
float candidate[CFG_NUM_BINS] = { 1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10,1e-10 };
float *kernel;
int weightsize;
int modelsize;

int communicating = 1;

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

    //Calculation of sizes of shared memory
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

void pdf_representation(unsigned char *restrict frame, float *restrict kernel)
{
    int x, y;
    unsigned char curr_pixel_value;
    int bin_value;

#pragma MUST_ITERATE(RECT_ROWS,RECT_ROWS,)
    for (y = 0; y < RECT_ROWS; y++) {
#pragma MUST_ITERATE(RECT_COLS,RECT_COLS,)
        for (x = 0; x < RECT_COLS; x++) {
            curr_pixel_value = frame[y*RECT_COLS + x];

            bin_value = (curr_pixel_value >>4);

            candidate[bin_value] = candidate[bin_value] + kernel[y*RECT_COLS + x];
        }
    }
}

void CalWeight(unsigned char *restrict frame, float *restrict weight, float *restrict candidate, float *restrict model)
{
    float multipliers[CFG_NUM_BINS];
    int x=0, y=0, xy=0, bin;
    unsigned char curr_pixel;

    //Calculation of multipliers with forced unenrolling pragma's
#pragma MUST_ITERATE(CFG_NUM_BINS, CFG_NUM_BINS,)
#pragma UNROLL(CFG_NUM_BINS)
    for (bin = 0; bin < CFG_NUM_BINS; bin++) {
        multipliers[bin] = sqrt(model[bin]/candidate[bin]);
    }

    //Calculation of weights with coalesced loops for software pipelining.
#pragma MUST_ITERATE(RECT_SIZE, RECT_SIZE,)
    for (xy = 0; xy < RECT_SIZE; xy++) {
        curr_pixel = frame[y*RECT_COLS+x];
        weight[y*RECT_COLS+x] = multipliers[curr_pixel>>4];
        if (++x == RECT_COLS)
        {
            x = 0;
            y = y + 1;
        }
        
    }
}

Int Task_execute(Task_TransferInfo * info)
{

    while(communicating) {
        //wait for semaphore
        SEM_pend(&(info->notifySemObj), SYS_FOREVER);

        //invalidate cache
        BCACHE_inv((Ptr)frame, RECT_SIZE, TRUE);
        BCACHE_inv((Ptr)model, modelsize, TRUE);
        BCACHE_inv((Ptr)weight, weightsize, TRUE);
        BCACHE_inv((Ptr)kernel, weightsize, TRUE);

        pdf_representation(frame, kernel);

        //call the functionality to be performed by dsp
        CalWeight(frame, weight, candidate, model);

        //Writeback to shared memory
        BCACHE_wbInv((Ptr)weight, weightsize, TRUE);

        //notify that we are done
        NOTIFY_notify(ID_GPP,MPCSXFER_IPS_ID,MPCSXFER_IPS_EVENTNO, (Uint32)0);
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
        kernel = (float *)info;
    }
    else if (count>4) {
        if ((int)info == DSP_END_CALCULATIONS) {
            communicating = 0;
        }
    }

    SEM_post(&(mpcsInfo->notifySemObj));
}
