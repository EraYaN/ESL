/** ============================================================================
 *  @file   da8xxgem_phy_shmem.h
 *
 *  @path   $(DSPLINK)/gpp/inc/sys/arch/DA8XXGEM/Linux/
 *
 *  @desc   Physical Interface Abstraction Layer for DA8XXGem.
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


#if !defined (DA8XXGEM_PHY_SHMEM_H)
#define DA8XXGEM_PHY_SHMEM_H


/*  ----------------------------------- DSP/BIOS Link               */
#include <dsplink.h>

/*  ----------------------------------- Hardware Abstraction Layer  */
#include <hal.h>


#if defined (__cplusplus)
extern "C" {
#endif


/** ============================================================================
 *  @name   DA8XXGEM_shmemInterface
 *
 *  @desc   Interface functions exported by the Shared Driver subcomponent.
 *  ============================================================================
 */
extern HAL_Interface DA8XXGEM_shmemInterface ;


/* ============================================================================
 *  @func   DA8XXGEM_phyShmemInit
 *
 *  @desc   Initializes Shared Driver/device.
 *
 *  @arg    halObject.
 *              HAL object.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              All other error conditions.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
DA8XXGEM_phyShmemInit (IN Pvoid halObj) ;


/* ============================================================================
 *  @func   DA8XXGEM_phyShmemExit
 *
 *  @desc   Finalizes Shared Driver/device.
 *
 *  @arg    halObject.
 *              HAL object.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              All other error conditions.
 *
 *  @enter  None.
 *
 *  @leave  None.
 *
 *  @see    None
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
DA8XXGEM_phyShmemExit (IN Pvoid halObj) ;


#if defined (__cplusplus)
}
#endif


#endif  /* !defined (DA8XXGEM_PHY_SHMEM_H) */
