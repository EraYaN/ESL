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
int communicating=0;
int compdfrep=0;

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
    int x=0, y=0, xyz=0, bin, z=0;
    unsigned char curr_pixel;

#pragma MUST_ITERATE(NUM_BINS, NUM_BINS,)
#pragma UNROLL(NUM_BINS)
    for (bin = 0; bin < (NUM_BINS*3); bin++) {
        multipliers[bin] = sqrt(model[bin]/candidate[bin]);
    }

#pragma MUST_ITERATE(RECT_SIZE*3, RECT_SIZE*3,)
    for (xyz = 0; xyz < RECT_SIZE*3; xyz++) {
        curr_pixel = frame[y*RECT_WIDTH+x + z * RECT_SIZE];
        weight[y*RECT_WIDTH+x] = multipliers[curr_pixel>>4 + z*NUM_BINS];
        if (++x == RECT_WIDTH)
        {
            x = 0;
            y = y + 1;
        }
        if (y == RECT_HEIGHT)
        {
            y = 0;
            z = z + 1;
        }
        
    }
}

/*void pdf_representation(unsigned char *restrict frame, float *restrict candidate)
{
    unsigned char curr_pixel_value;
    float bin_value;


    for (int i = 0; i < RECT_HEIGHT; i++) {
        col_index = rect.x;
        for (int j = 0; j < RECT_WIDTH; j++) {
            curr_pixel_value = frame.at<cv::Vec3b>(row_index, col_index);
            bin_value[0] = (curr_pixel_value[0] / bin_width);
            bin_value[1] = (curr_pixel_value[1] / bin_width);
            bin_value[2] = (curr_pixel_value[2] / bin_width);
            pdf_model.at<float>(0, bin_value[0]) += kernel.at<float>(i, j);
            pdf_model.at<float>(1, bin_value[1]) += kernel.at<float>(i, j);
            pdf_model.at<float>(2, bin_value[2]) += kernel.at<float>(i, j);
            col_index++;
        }
        row_index++;
    }
}*/

Int Task_execute(Task_TransferInfo * info)
{
    //TODO[c]: not used anymore
    //int sum;
    // Uint16 k;

    static int counter = 1;

    // for (k = 0; k < 3*32; k++) {
    while(1) {

        SEM_pend(&(info->notifySemObj), SYS_FOREVER);
        if (compdfrep)
        {
            compdfrep--;
        }
        if(communicating){
            //wait for semaphore

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
        if ((int)info == 1)
            communicating++;

        if ((int)info == 2)
            compdfrep++;
    }

    SEM_post(&(mpcsInfo->notifySemObj));
}
