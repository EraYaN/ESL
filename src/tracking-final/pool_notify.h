#if !defined (pool_notify_H)
#define pool_notify_H

/*  ----------------------------------- DSP/BIOS Link                 */
#include <dsplink.h>
#include <util_global_dsp.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


/** ============================================================================
 *  @const  ID_PROCESSOR
 *
 *  @desc   The processor id of the processor being used.
 *  ============================================================================
 */
#define ID_PROCESSOR       0

 //Pointers to shared databuffers
extern Uchar8 * poolFrame;
extern float * poolWeight;
extern float * poolModel;
extern float * poolCandidate;

//Class for determining buffer sizes for initialization.
class bufferInit
{
public:
    int frame;
    int frameAligned;
    int rectHeight;
    int rectWidth;
    int region;
    int regionAligned;
    int modelAligned;
    bufferInit(cv::Mat frame, cv::Rect rect);
};

/** ============================================================================
*  @func   pool_notify_Create
*
*  @desc   This function allocates and initializes resources used by
*          this application.
*
*  @arg    dspExecutable
*              DSP executable name.
*  @arg    strBufferSize
*              String representation of buffer size to be used
*              for data transfer.
*  @arg    strNumIterations
*              Number of iterations a data buffer is transferred between
*              GPP and DSP in string format.
*  @arg    processorId
*             Id of the DSP Processor.
*
*  @ret    DSP_SOK
*              Operation successfully completed.
*          DSP_EFAIL
*              Resource allocation failed.
*
*  @enter  None
*
*  @leave  None
*
*  @see    pool_notify_Delete
*  ============================================================================
*/
NORMAL_API DSP_STATUS pool_notify_Create(IN Char8 *dspExecutable, bufferInit bufferSizes, IN Uint8 processorId, IN Char8 * strBuffersize);

/** ============================================================================
*  @func   pool_notify_Execute
*
*  @desc   This function implements the execute phase for this application.
*
*  @arg    numIterations
*              Number of times to send the message to the DSP.
*  @arg    processorId
*             Id of the DSP Processor.
*
*  @ret    DSP_SOK
*              Operation successfully completed.
*          DSP_EFAIL
*              MESSAGE execution failed.
*
*  @enter  None
*
*  @leave  None
*
*  @see    pool_notify_Delete , pool_notify_Create
*  ============================================================================
*/
NORMAL_API DSP_STATUS pool_notify_Execute(IN Uint8 info);

NORMAL_API DSP_STATUS pool_notify_Wait();


/** ============================================================================
*  @func   pool_notify_Delete
*
*  @desc   This function releases resources allocated earlier by call to
*          pool_notify_Create ().
*          During cleanup, the allocated resources are being freed
*          unconditionally. Actual applications may require stricter check
*          against return values for robustness.
*
*  @arg    processorId
*             Id of the DSP Processor.
*
*  @ret    DSP_SOK
*              Operation successfully completed.
*          DSP_EFAIL
*              Resource deallocation failed.
*
*  @enter  None
*
*  @leave  None
*
*  @see    pool_notify_Create
*  ============================================================================
*/
NORMAL_API Void pool_notify_Delete(IN Uint8 processorId, bufferInit bufferSizes);


/** ============================================================================
*  @func   pool_notify_Main
*
*  @desc   The OS independent driver function for the MESSAGE sample
*          application.
*
*  @arg    dspExecutable
*              Name of the DSP executable file.
*  @arg    strBufferSize
*              Buffer size to be used for data-transfer in string format.
*  @arg    strNumIterations
*              Number of iterations a data buffer is transferred between
*              GPP and DSP in string format.
*  @arg    strProcessorId
*             ID of the DSP Processor in string format.
*
*  @ret    None
*
*  @enter  None
*
*  @leave  None
*
*  @see    pool_notify_Create, pool_notify_Execute, pool_notify_Delete
*  ============================================================================
*/
NORMAL_API Void pool_notify_Init(IN Char8 * dspExecutable, bufferInit bufferSizes, IN Char8 * strBuffersize);





#endif /* !defined (pool_notify_H) */