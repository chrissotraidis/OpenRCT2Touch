/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../Context.h"
#include "../GameState.h"
#include "../OpenRCT2.h"
#include "../core/Console.hpp"
#include "../core/Path.hpp"
#include "../entity/EntityRegistry.h"
#include "../network/NetworkTypes.h"
#include "CommandLine.hpp"

#include <memory>

using namespace OpenRCT2::CommandLine;

namespace OpenRCT2
{
    static u8string _userDataPath = {};
    static u8string _openrct2DataPath = {};
    static u8string _rct1DataPath = {};
    static u8string _rct2DataPath = {};

    // clang-format off
    static constexpr CommandLineOptionDefinition kSimulateOptions[]
    {
        { CMDLINE_TYPE_STRING, &_userDataPath,     kNAC, "user-data-path",     "path to the user data directory (containing config.ini)" },
        { CMDLINE_TYPE_STRING, &_openrct2DataPath, kNAC, "openrct2-data-path", "path to the OpenRCT2 data directory (containing languages)" },
        { CMDLINE_TYPE_STRING, &_rct1DataPath,     kNAC, "rct1-data-path",     "path to the RollerCoaster Tycoon 1 data directory (containing data/csg1.dat)" },
        { CMDLINE_TYPE_STRING, &_rct2DataPath,     kNAC, "rct2-data-path",     "path to the RollerCoaster Tycoon 2 data directory (containing data/g1.dat)" },
        kOptionTableEnd
    };

    static ExitCode HandleSimulate(CommandLineArgEnumerator* argEnumerator);

    const CommandLineCommand CommandLine::kSimulateCommands[]{
        // Main commands
        DefineCommand("", "<park file> <ticks>", kSimulateOptions, HandleSimulate),
        kCommandTableEnd
    };
    // clang-format on

    static ExitCode HandleSimulate(CommandLineArgEnumerator* argEnumerator)
    {
        const utf8* inputPath;
        if (!argEnumerator->TryPopString(&inputPath))
        {
            Console::Error::WriteLine("Expected a save file path");
            return ExitCode::fail;
        }

        int32_t ticks;
        if (!argEnumerator->TryPopInteger(&ticks))
        {
            Console::Error::WriteLine("Expected a number of ticks to simulate");
            return ExitCode::fail;
        }

        if (!_userDataPath.empty())
        {
            gCustomUserDataPath = Path::GetAbsolute(_userDataPath);
        }
        if (!_openrct2DataPath.empty())
        {
            gCustomOpenRCT2DataPath = Path::GetAbsolute(_openrct2DataPath);
        }
        if (!_rct1DataPath.empty())
        {
            gCustomRCT1DataPath = Path::GetAbsolute(_rct1DataPath);
        }
        if (!_rct2DataPath.empty())
        {
            gCustomRCT2DataPath = Path::GetAbsolute(_rct2DataPath);
        }

        gOpenRCT2Headless = true;

#ifndef DISABLE_NETWORK
        gNetworkStart = Network::Mode::server;
#endif

        std::unique_ptr<IContext> context(CreateContext());
        if (context->Initialise())
        {
            if (!context->LoadParkFromFile(inputPath))
            {
                return ExitCode::fail;
            }

            Console::WriteLine("Running %d ticks...", ticks);
            for (int32_t i = 0; i < ticks; i++)
            {
                gameStateUpdateLogic();
            }
            Console::WriteLine("Completed: %s", getGameState().entities.getAllEntitiesChecksum().toString().c_str());
        }
        else
        {
            Console::Error::WriteLine("Context initialization failed.");
            return ExitCode::fail;
        }

        return ExitCode::ok;
    }
} // namespace OpenRCT2
