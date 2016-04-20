
#pragma once

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"

#ifdef AE_OS_WIN
#include <cstdlib>
#include <string.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUG_VERSION  0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

#define CURVE_TOLERANCE 0.05f

enum
{
    UNWARP_INPUT = 0,
    UNWARP_INPUT_FOV,
    UNWARP_SCALE,
    UNWARP_NUM_PARAMS
};

extern "C" {
    DllExport PF_Err
        EntryPointFunc(
            PF_Cmd   cmd,
            PF_InData  *in_data,
            PF_OutData  *out_data,
            PF_ParamDef  *params[],
            PF_LayerDef  *output,
            void   *extra);
}
