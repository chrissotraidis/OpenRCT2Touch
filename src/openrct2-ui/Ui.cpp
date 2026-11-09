/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Ui.h"

#include "SDLException.h"
#include "UiContext.h"
#include "audio/AudioContext.h"
#include "drawing/BitmapReader.h"

#include <SDL.h>
#include <memory>
#include <openrct2/Context.h>
#include <openrct2/Diagnostic.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/command_line/ExitCode.h>
#include <openrct2/ui/UiContext.h>
#include <vector>

#if defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

using namespace OpenRCT2;
using namespace OpenRCT2::Audio;
using namespace OpenRCT2::Ui;

static int RunOpenRCT2(int argc, const char** argv)
{
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_SIMULATOR
    SDL_SetHint(SDL_HINT_AUDIODRIVER, "dummy");
#endif

#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({
        specialHTMLTargets["!canvas"] = Module.canvas;
        Module.canvas.addEventListener("contextmenu", function(e) { e.preventDefault(); });
    });
#endif
    int32_t rc = EXIT_SUCCESS;
    auto runGame = CommandLineRun(argv, argc);
    RegisterBitmapReader();
    if (runGame == OpenRCT2::CommandLine::ExitCode::launch)
    {
        std::unique_ptr<IContext> context;
        if (gOpenRCT2Headless)
        {
            // Run OpenRCT2 with a plain context
            context = CreateContext();
        }
        else
        {
            // Run OpenRCT2 with a UI context
            auto env = CreatePlatformEnvironment();
            std::unique_ptr<IAudioContext> audioContext;
            try
            {
                audioContext = CreateAudioContext();
            }
            catch (const SDLException& e)
            {
                LOG_WARNING("Failed to create audio context. Using dummy audio context. Error message was: %s", e.what());
                audioContext = CreateDummyAudioContext();
            }
            auto uiContext = CreateUiContext(*env);
            context = CreateContext(std::move(env), std::move(audioContext), std::move(uiContext));
        }
        rc = context->RunOpenRCT2(argc, argv);
    }
    else if (runGame == OpenRCT2::CommandLine::ExitCode::fail)
    {
        rc = EXIT_FAILURE;
    }
    return rc;
}

#if defined(_MSC_VER) && !defined(__DISABLE_DLL_PROXY__)

int NormalisedMain(int argc, const char** argv)
{
    return RunOpenRCT2(argc, argv);
}

#elif defined(__ANDROID__) || defined(__MINGW32__) || (defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS)

extern "C" {
int SDL_main(int argc, char* argv[]);

int SDL_main(int argc, char* argv[])
{
    std::vector<const char*> normalisedArgv(argv, argv + argc);
    return RunOpenRCT2(argc, normalisedArgv.data());
}
}

#else

int main(int argc, const char** argv)
{
    return RunOpenRCT2(argc, argv);
}

#endif
