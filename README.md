
# FisheyeUnwarp

FisheyeUnwarp is an effect plug-in for Adobe After Effects CC and Adobe Premiere Pro CC that unwarps images shot with a fish-eye lens (for example, with a GoPro camera) into a planar perspective projection. 

## Why? 

After Effects and Premiere Pro already include an effect that can do such unwarp: Lens Distortion. But it is unbelievably slow. This plug-in is way faster - although it is not GPU accelerated.

## How to install

Like any other AE / PP plug-in: copy the FisheyeUnwarp.aex file into the Adobe plug-ins directory. On my system, it is located here: `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore`

Currently, the plug-in is for Windows x64 systems only.

## How to use

You'll find this effect under Video Effects / Distort category. After applying it to a track with default settings, you should see an image with heavily distorted boundaries. 

There are only two settings: Input FOV and Output FOV. These are horizontal FOVs measured in degrees. Input FOV should match your camera and setting; the default value of 96 seems to work well for GoPro Hero4 with Wide setting. Tune it until all the straight lines become straight on the screen. Output FOV is basically a scale factor for the output image; reduce it to make the output larger.

## How to build

1. Download the After Effects CC SDK for Windows here: http://www.adobe.com/devnet/aftereffects.html
2. Unpack the SDK into some local folder
3. Set the AE_PLUGIN_BUILD_DIR environment variable to point to the Adobe plug-ins folder (see above)
4. Copy the files of this plug-in into Examples\Effect sub-folder of the SDK
5. Open Win\FisheyeUnwarp.sln solution with Visual Studio 2015 (or downgrade it if necessary)
6. Build the solution for x64-Release or x64-Debug
7. Launch After Effects or Premiere Pro. You can launch them from Visual Studio with a debugger attached, for that go to Project properties / Debugging dialog and set Command to the AE / PP executable.
