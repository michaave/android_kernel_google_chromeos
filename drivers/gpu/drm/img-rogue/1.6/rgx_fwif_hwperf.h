/*************************************************************************/ /*!
@File           rgx_fwif_hwperf.h
@Title          RGX HWPerf support
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Shared header between RGX firmware and Init process
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#ifndef RGX_FWIF_HWPERF_H
#define RGX_FWIF_HWPERF_H

#include "rgx_fwif_shared.h"
#include "rgx_hwperf_km.h"
#include "rgxdefs_km.h"


/*****************************************************************************/

/* Function pointer type for functions to check dynamic power state of counter blocks */
typedef IMG_BOOL (*PFN_RGXFW_HWPERF_CNTBLK_POWERED)(RGX_HWPERF_CNTBLK_ID eBlkType, IMG_UINT8 ui8UnitId);

/* This structure encodes properties of a type of performance counter block.
 * The structure is sometimes referred to as a block type descriptor. These
 * properties contained in this structure represent the columns in the
 * block type model table variable below. There values vary depending on
 * the build BVNC and core type.
 * Each direct block has a unique type descriptor and each indirect group has
 * a type descriptor. */
typedef struct _RGXFW_HWPERF_CNTBLK_TYPE_MODEL_
{
	/* Could use RGXFW_ALIGN_DCACHEL here but then we would waste 40% of the cache line? */
	IMG_UINT32 uiCntBlkIdBase;         /* The starting block id for this block type */
	IMG_UINT32 uiIndirectReg;          /* 0 if direct type otherwise the indirect control register to select indirect unit */
	IMG_UINT32 uiPerfReg;              /* RGX_CR_*_PERF register for this block type */
	IMG_UINT32 uiSelect0BaseReg;       /* RGX_CR_*_PERF_SELECT0 register for this block type */
	IMG_UINT32 uiBitSelectPreserveMask;/* Select register bits to preserve on programming, HW_ERN_41805 */
	IMG_UINT32 uiCounter0BaseReg;      /* RGX_CR_*_PERF_COUNTER_0 register for this block type */
	IMG_UINT8  uiNumCounters;          /* Number of counters in this block type */
	IMG_UINT8  uiNumUnits;             /* Number of instances of this block type in the core */
	IMG_UINT8  uiSelectRegModeShift;   /* Mode field shift value of select registers */
	IMG_UINT8  uiSelectRegOffsetShift; /* Interval between select registers, either 8 bytes or 16, hence << 3 or << 4 */
	IMG_CHAR   pszBlockNameComment[30];              /* Name of the PERF register. Used while dumping the perf counters to pdumps */
	PFN_RGXFW_HWPERF_CNTBLK_POWERED pfnIsBlkPowered; /* A function to determine dynamic power state for the block type */
} RGXFW_HWPERF_CNTBLK_TYPE_MODEL;

/* Used to instantiate a null row in the block type model table below where the
 * block is not supported for a given build BVNC. This is needed as the blockid
 * to block type lookup uses the table as well and clients may try to access blocks
 * not in the hardware. */
#define RGXFW_HWPERF_CNTBLK_TYPE_UNSUPPORTED(_blkid) {_blkid, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", NULL}

/* Structure to hold a block's parameters for passing between the BG context
 * and the IRQ context when applying a configuration request. */
typedef struct _RGXFWIF_HWPERF_CTL_BLK_
{
	IMG_BOOL				bValid;
	IMG_BOOL                bEnabled;
	IMG_UINT32              eBlockID;
	IMG_UINT32              uiCounterMask;
	IMG_UINT64  RGXFW_ALIGN aui64CounterCfg[RGX_HWPERF_CNTRS_IN_BLK];
}  RGXFWIF_HWPERF_CTL_BLK;

/* Structure used to hold the configuration of the non-mux counters blocks */
typedef struct _RGXFW_HWPERF_SELECT_
{
	IMG_UINT32            ui32NumSelectedCounters;
	IMG_UINT32            aui32SelectedCountersIDs[RGX_HWPERF_MAX_CUSTOM_CNTRS];
} RGXFW_HWPERF_SELECT;

/* Structure to hold the whole configuration request details for all blocks */
#define RGXFWIF_HWPERF_CTL_MAX_BLK_CFGS (RGX_CNTBLK_ID_DIRECT_LAST+RGX_HWPERF_MAX_INDIRECT_ADDR_BLKS)
typedef struct _RGXFWIF_HWPERF_CTL_
{
	IMG_BOOL             bResetOrdinal;
	IMG_UINT32           ui32EnabledBlksCount; /* Number of blocks enabled in the proceeding array.
	                                            * Used to optimise the no counters HW event function. */
	RGXFWIF_HWPERF_CTL_BLK sBlkCfg[RGXFWIF_HWPERF_CTL_MAX_BLK_CFGS];
	IMG_UINT32           ui32SelectedCountersBlockMask; /* Provide an optimisation for the checking of the configured
	                                                       blocks for non-mutex counters */
	RGXFW_HWPERF_SELECT RGXFW_ALIGN SelCntr[RGX_HWPERF_MAX_CUSTOM_BLKS];
} UNCACHED_ALIGN RGXFWIF_HWPERF_CTL;

/* NOTE: The switch statement in this function must be kept in alignment with
 * the enumeration RGX_HWPERF_CNTBLK_ID defined in rgx_hwperf_km.h. ASSERTs may
 * result if not.
 * The function provides a hash lookup to get a handle on the global store for
 * a block's configuration store from it's block ID.
 */
#ifdef INLINE_IS_PRAGMA
#pragma inline(rgxfw_hwperf_get_block_ctl)
#endif
static INLINE RGXFWIF_HWPERF_CTL_BLK* rgxfw_hwperf_get_block_ctl(
		RGX_HWPERF_CNTBLK_ID eBlockID, RGXFWIF_HWPERF_CTL *psHWPerfInitData)
{
	IMG_INT32 i32Idx = -1;

	/* Hash the block ID into a control configuration array index */
	switch(eBlockID)
	{
		case RGX_CNTBLK_ID_TA:
		case RGX_CNTBLK_ID_RASTER:
		case RGX_CNTBLK_ID_HUB:
		case RGX_CNTBLK_ID_TORNADO:
		case RGX_CNTBLK_ID_JONES:
		case RGX_CNTBLK_ID_BF:
		case RGX_CNTBLK_ID_BT:
		case RGX_CNTBLK_ID_RT:
		case RGX_CNTBLK_ID_BX_TU:
		case RGX_CNTBLK_ID_SH:
		{
			i32Idx = eBlockID;
			break;
		}
		case RGX_CNTBLK_ID_TPU_MCU0:
		case RGX_CNTBLK_ID_TPU_MCU1:
		case RGX_CNTBLK_ID_TPU_MCU2:
		case RGX_CNTBLK_ID_TPU_MCU3:
		case RGX_CNTBLK_ID_TPU_MCU4:
		case RGX_CNTBLK_ID_TPU_MCU5:
		case RGX_CNTBLK_ID_TPU_MCU6:
		case RGX_CNTBLK_ID_TPU_MCU7:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + (eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
			break;
		}
		case RGX_CNTBLK_ID_USC0:
		case RGX_CNTBLK_ID_USC1:
		case RGX_CNTBLK_ID_USC2:
		case RGX_CNTBLK_ID_USC3:
		case RGX_CNTBLK_ID_USC4:
		case RGX_CNTBLK_ID_USC5:
		case RGX_CNTBLK_ID_USC6:
		case RGX_CNTBLK_ID_USC7:
        case RGX_CNTBLK_ID_USC8:
        case RGX_CNTBLK_ID_USC9:
        case RGX_CNTBLK_ID_USC10:
        case RGX_CNTBLK_ID_USC11:
        case RGX_CNTBLK_ID_USC12:
        case RGX_CNTBLK_ID_USC13:
        case RGX_CNTBLK_ID_USC14:
        case RGX_CNTBLK_ID_USC15:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST +
					(eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
			break;
		}
		case RGX_CNTBLK_ID_TEXAS0:
		case RGX_CNTBLK_ID_TEXAS1:
        case RGX_CNTBLK_ID_TEXAS2:
        case RGX_CNTBLK_ID_TEXAS3:
		case RGX_CNTBLK_ID_TEXAS4:
		case RGX_CNTBLK_ID_TEXAS5:
		case RGX_CNTBLK_ID_TEXAS6:
		case RGX_CNTBLK_ID_TEXAS7:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST +
					RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER + (eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
			break;
		}
		case RGX_CNTBLK_ID_RASTER0:
		case RGX_CNTBLK_ID_RASTER1:
        case RGX_CNTBLK_ID_RASTER2:
        case RGX_CNTBLK_ID_RASTER3:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + RGX_HWPERF_PHANTOM_DUST_BLKS * RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST +
			    RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER + (RGX_HWPERF_PHANTOM_NONDUST_BLKS-1) * RGX_HWPERF_INDIRECT_BY_PHANTOM
			    + (eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
			break;
		}
		case RGX_CNTBLK_ID_BLACKPEARL0:
        case RGX_CNTBLK_ID_BLACKPEARL1:
        case RGX_CNTBLK_ID_BLACKPEARL2:
        case RGX_CNTBLK_ID_BLACKPEARL3:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + RGX_HWPERF_PHANTOM_DUST_BLKS * RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST +
                RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER + (RGX_HWPERF_PHANTOM_NONDUST_BLKS-1) * RGX_HWPERF_INDIRECT_BY_PHANTOM
                + (eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
            break;
		}
		case RGX_CNTBLK_ID_PBE0:
        case RGX_CNTBLK_ID_PBE1:
        case RGX_CNTBLK_ID_PBE2:
        case RGX_CNTBLK_ID_PBE3:
        case RGX_CNTBLK_ID_PBE4:
        case RGX_CNTBLK_ID_PBE5:
        case RGX_CNTBLK_ID_PBE6:
        case RGX_CNTBLK_ID_PBE7:
        case RGX_CNTBLK_ID_PBE8:
        case RGX_CNTBLK_ID_PBE9:
        case RGX_CNTBLK_ID_PBE10:
        case RGX_CNTBLK_ID_PBE11:
        case RGX_CNTBLK_ID_PBE12:
        case RGX_CNTBLK_ID_PBE13:
        case RGX_CNTBLK_ID_PBE14:
        case RGX_CNTBLK_ID_PBE15:
		{
			i32Idx = RGX_CNTBLK_ID_DIRECT_LAST + RGX_HWPERF_PHANTOM_DUST_BLKS * RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST +
                RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER + RGX_HWPERF_PHANTOM_NONDUST_BLKS * RGX_HWPERF_INDIRECT_BY_PHANTOM
                + (eBlockID & RGX_CNTBLK_ID_UNIT_MASK);
			break;
		}
		default:
		{
			return NULL;
		}
	}
	if ((i32Idx < 0) || (i32Idx >= RGXFWIF_HWPERF_CTL_MAX_BLK_CFGS))
	{
		return NULL;
	}
	return &psHWPerfInitData->sBlkCfg[i32Idx];
}

#endif
