
#ifdef _DEBUG
//#define BENCHMARKING 1
#endif

#include "FisheyeUnwarp.h"
#include <stdio.h>

#if BENCHMARKING 
#include <Windows.h>
#endif

static PF_Err
About(
    PF_InData *in_data,
    PF_OutData *out_data)
{
    sprintf_s(out_data->return_msg, "Fisheye Unwarp v%d.%d\r"
        "Translates video shot with a fish-eye lens to a planar projection.",
        MAJOR_VERSION,
        MINOR_VERSION);

    return PF_Err_NONE;
}

static PF_Err
GlobalSetup(
    PF_OutData *out_data)
{
    out_data->my_version = PF_VERSION(MAJOR_VERSION,
        MINOR_VERSION,
        BUG_VERSION,
        STAGE_VERSION,
        BUILD_VERSION);

    out_data->out_flags = PF_OutFlag_PIX_INDEPENDENT;
    out_data->out_flags2 = PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG;

    return PF_Err_NONE;
}

static PF_Err
ParamsSetup(
    PF_InData *in_data,
    PF_OutData *out_data)
{
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);

    PF_ADD_FLOAT_SLIDER("Input FOV",
        10.0f, 170.f,   // Valid Min, Max
        30.0f, 140.f,   // Slider Min, Max
        CURVE_TOLERANCE,
        96.0f,          // Default
        2,
        PF_ValueDisplayFlag_NONE,
        false,
        UNWARP_INPUT_FOV);

    AEFX_CLR_STRUCT(def);

    PF_ADD_FLOAT_SLIDER("Scale",
        0.1f, 10.f,     // Valid Min, Max
        0.2f, 2.0f,     // Slider Min, Max
        CURVE_TOLERANCE,
        1.0f,           // Default
        2,
        PF_ValueDisplayFlag_NONE,
        false,
        UNWARP_SCALE);

    AEFX_CLR_STRUCT(def);

    out_data->num_params = UNWARP_NUM_PARAMS;

    return err;
}

static PF_Err
Render(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE;

    struct FuncData {
        PF_InData* in_data;
        PF_LayerDef* input;
        PF_LayerDef* output;
        PF_Pixel8* inputPixels;
        PF_Pixel8* outputPixels;
        double inputFov;
        double outputFov;
        PF_SampPB sampleParams;
        double P;
        double Q;
    } closure;

    closure.in_data = in_data;
    closure.input = &params[UNWARP_INPUT]->u.ld;
    closure.output = output;
    in_data->utils->get_pixel_data8(closure.input, nullptr, &closure.inputPixels);
    in_data->utils->get_pixel_data8(closure.output, nullptr, &closure.outputPixels);
    memset(&closure.sampleParams, 0, sizeof(PF_SampPB));
    closure.sampleParams.x_radius = INT2FIX(1);
    closure.sampleParams.y_radius = INT2FIX(1);
    closure.sampleParams.src = closure.input;

    double inputFov = params[UNWARP_INPUT_FOV]->u.fs_d.value;
    double scale = params[UNWARP_SCALE]->u.fs_d.value;
    double outputFov = inputFov;
    double outputFovTan = tan(outputFov * 0.5 * M_PI / 180.0);
    double invHalfOutputWidth = 2.0 / double(output->width) / scale;
    closure.P = invHalfOutputWidth * outputFovTan;

    double halfInputWidth = double(closure.input->width) * 0.5;
    double invSinInputFov = 1.0 / sin(inputFov * 0.5 * M_PI / 180.0);
    closure.Q = halfInputWidth * invSinInputFov;

    auto F = [](void* refconPV, A_long thread_indexL, A_long outputY, A_long iterationsL)
    {
        const FuncData* closure = (const FuncData*)refconPV;

        double outputCenterX = double(closure->output->width) * 0.5;
        double outputCenterY = double(closure->output->height) * 0.5;

        double inputCenterX = double(closure->input->width) * 0.5;
        double inputCenterY = double(closure->input->height) * 0.5;

        PF_UtilCallbacks* utils = closure->in_data->utils;
        PF_ProgPtr effect_ref = closure->in_data->effect_ref;

        PF_Pixel8* outputPixelP = (PF_Pixel8*)((char*)closure->outputPixels + outputY * closure->output->rowbytes);

        // Parts of the distortion math that are loop invariant
        double P = closure->P;
        double Q = closure->Q;
        double Psq = P * P;
        double PQ = P * Q;
        double relativeY = double(outputY) - outputCenterY;
        double XsqBias = relativeY * relativeY * Psq + 1;

        for (double relativeX = -outputCenterX; relativeX < outputCenterX; relativeX += 1.0)
        {
#if 0
            // R is the distance between output center and this pixel
            double R = sqrt(relativeX * relativeX + relativeY * relativeY);
            // Angle between the view vector (Z) and vector to this pixel
            double angle = atan(R * P);
            // New distance under the orthographic fisheye projection
            double newR = Q * sin(angle);
            double distortion = newR / R;
#else
            // Same result as the above math, optimized using trigonometry
            double distortion = PQ / sqrt(relativeX * relativeX * Psq + XsqBias);
#endif

            double sampleX = relativeX * distortion + inputCenterX;
            double sampleY = relativeY * distortion + inputCenterY;

            utils->subpixel_sample(effect_ref, FLOAT2FIX(sampleX), FLOAT2FIX(sampleY), &closure->sampleParams, outputPixelP);

            outputPixelP++;
        }


        return PF_Err(PF_Err_NONE);
    };

#if BENCHMARKING 
    LARGE_INTEGER cbegin, cend, cfreq;
    QueryPerformanceCounter(&cbegin);
#endif

    if (in_data->utils->begin_sampling)
        in_data->utils->begin_sampling(in_data->effect_ref, PF_Quality_HI, PF_MF_Alpha_PREMUL, &closure.sampleParams);

    err = in_data->utils->iterate_generic(closure.output->height, &closure, F);

    if (in_data->utils->end_sampling)
        in_data->utils->end_sampling(in_data->effect_ref, PF_Quality_HI, PF_MF_Alpha_PREMUL, &closure.sampleParams);

#if BENCHMARKING 
    QueryPerformanceCounter(&cend);
    QueryPerformanceFrequency(&cfreq);
    double time = double(cend.QuadPart - cbegin.QuadPart) / double(cfreq.QuadPart);
    char buf[100];
    sprintf_s(buf, "Distort time = %.3f ms, current_time = %d\n", time * 1e3, in_data->current_time);
    OutputDebugStringA(buf);
#endif

    return err;
}

DllExport	PF_Err
EntryPointFunc(
    PF_Cmd cmd,
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output,
    void *extra)
{
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
        case PF_Cmd_ABOUT:
            ERR(About(in_data, out_data));
            break;
        case PF_Cmd_GLOBAL_SETUP:
            ERR(GlobalSetup(out_data));
            break;
        case PF_Cmd_PARAMS_SETUP:
            ERR(ParamsSetup(in_data, out_data));
            break;
        case PF_Cmd_RENDER:
            ERR(Render(in_data, out_data, params, output));
            break;
        }
    }
    catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
