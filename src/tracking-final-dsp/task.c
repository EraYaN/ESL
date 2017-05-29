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

#define NUM_BINS 16
#define MODELSIZE (NUM_BINS*3)

extern Uint16 MPCSXFER_BufferSize ;


unsigned char* frame;
float * weight;
float * model;
int rectHeight;
int rectWidth;
int rectsize;
int framesize;

static Void Task_notify (Uint32 eventNo, Ptr arg, Ptr info) ;

Int Task_create (Task_TransferInfo ** infoPtr)
{
    Int status    = SYS_OK ;
    Task_TransferInfo * info = NULL ;

    /* Allocate Task_TransferInfo structure that will be initialized
     * and passed to other phases of the application */
    if (status == SYS_OK) 
	{
        *infoPtr = MEM_calloc (DSPLINK_SEGID,
                               sizeof (Task_TransferInfo),
                               0) ; /* No alignment restriction */
        if (*infoPtr == NULL) 
		{
            status = SYS_EALLOC ;
        }
        else 
		{
            info = *infoPtr ;
        }
    }

    /* Fill up the transfer info structure */
    if (status == SYS_OK) 
	{
        info->dataBuf       = NULL ; /* Set through notification callback. */
        info->bufferSize    = MPCSXFER_BufferSize ;
        SEM_new (&(info->notifySemObj), 0) ;
    }

    /*
     *  Register notification for the event callback to get control and data
     *  buffer pointers from the GPP-side.
     */
    if (status == SYS_OK) 
	{
        status = NOTIFY_register (ID_GPP,
                                  MPCSXFER_IPS_ID,
                                  MPCSXFER_IPS_EVENTNO,
                                  (FnNotifyCbck) Task_notify,
                                  info) ;
        if (status != SYS_OK) 
		{
            return status;
        }
    }

    /*
     *  Send notification to the GPP-side that the application has completed its
     *  setup and is ready for further execution.
     */
    if (status == SYS_OK) 
	{
        status = NOTIFY_notify (ID_GPP,
                                MPCSXFER_IPS_ID,
                                MPCSXFER_IPS_EVENTNO,
                                (Uint32) 0) ; /* No payload to be sent. */
        if (status != SYS_OK) 
		{
            return status;
        }
    }

    /*
     *  Wait for the event callback from the GPP-side to post the semaphore
     *  indicating receipt of the data buffer pointer and image width and height.
     */
    SEM_pend (&(info->notifySemObj), SYS_FOREVER) ;
    SEM_pend (&(info->notifySemObj), SYS_FOREVER) ;
	SEM_pend(&(info->notifySemObj), SYS_FOREVER);
	SEM_pend(&(info->notifySemObj), SYS_FOREVER);
	SEM_pend(&(info->notifySemObj), SYS_FOREVER);
	SEM_pend(&(info->notifySemObj), SYS_FOREVER);

	rectsize = rectHeight*rectWidth;

    return status ;
}


void sum_dsp() 
{
	//Test function
	weight[0] = frame[0] + model[0];
	frame[0] = 1;
}

/*void CalWeight()
{
	float multipliers[NUM_BINS];
	int x, y, bin, curr_pixel;
	static int counter = 0;

	for (bin = 0; bin < NUM_BINS; bin++) {
		multipliers[bin] = sqrt(buf->target_model_row[bin] / buf->target_candidate_row[bin]);
	}

	for (y = 0; y < rectHeight; y++) {
		for (x = 0; x < rectWidth; x++) {
			curr_pixel = buf->next_frame_rect[y*rectWidth + x];
			buf->weight[y*rectWidth + x] = multipliers[curr_pixel >> 4];
		}
	}
	// buf->weight[0] = 1.00 * counter++;

}*/

Int Task_execute (Task_TransferInfo * info)
{

    //wait for semaphore
	SEM_pend (&(info->notifySemObj), SYS_FOREVER);

	//invalidate cache
    BCACHE_inv((Ptr)frame, framesize, TRUE);
	BCACHE_inv((Ptr)model, MODELSIZE, TRUE);
	BCACHE_inv((Ptr)weight, rectsize, TRUE);

	//call the functionality to be performed by dsp
    sum_dsp();

	//Writeback data
	BCACHE_wbInv((Ptr)model, MODELSIZE, TRUE);

	//notify that we are done
    NOTIFY_notify(ID_GPP,MPCSXFER_IPS_ID,MPCSXFER_IPS_EVENTNO,(Uint32)0);
	//notify the result
    NOTIFY_notify(ID_GPP,MPCSXFER_IPS_ID,MPCSXFER_IPS_EVENTNO,(Uint32)1);

    return SYS_OK;
}

Int Task_delete (Task_TransferInfo * info)
{
    Int    status     = SYS_OK ;
    /*
     *  Unregister notification for the event callback used to get control and
     *  data buffer pointers from the GPP-side.
     */
    status = NOTIFY_unregister (ID_GPP,
                                MPCSXFER_IPS_ID,
                                MPCSXFER_IPS_EVENTNO,
                                (FnNotifyCbck) Task_notify,
                                info) ;

    /* Free the info structure */
    MEM_free (DSPLINK_SEGID,
              info,
              sizeof (Task_TransferInfo)) ;
    info = NULL ;

    return status ;
}


static Void Task_notify (Uint32 eventNo, Ptr arg, Ptr info)
{
    static int count = 0;
    Task_TransferInfo * mpcsInfo = (Task_TransferInfo *) arg ;

    (Void) eventNo ; /* To avoid compiler warning. */

    count++;
    if (count==1) {
        frame =(unsigned char*)info ;
    }
    if (count==2) {
        framesize = (int)info;
    }
	if (count == 3) {
		model = (float*)info;
	}
	if (count == 4) {
		rectHeight = (int)info;
	}
	if (count == 5) {
		weight = (float*)info;
	}
	if (count == 6) {
		rectWidth = (int)info;
	}

    SEM_post(&(mpcsInfo->notifySemObj));
}
