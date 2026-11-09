/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <TargetConditionals.h>

#if TARGET_OS_IOS

    #include "../../src/openrct2/OpenRCT2.h"
    #include "../../src/openrct2/core/Path.hpp"
    #include "../../src/openrct2/drawing/Font.h"
    #include "../../src/openrct2/localisation/Language.h"
    #include "../../src/openrct2/platform/Platform.h"

    #include <CoreText/CoreText.h>
    #include <Foundation/Foundation.h>
    #include <mach-o/dyld.h>

namespace OpenRCT2::Platform
{
    static std::string GetSearchPath(NSSearchPathDirectory directory)
    {
        @autoreleasepool
        {
            NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES);
            NSString* path = paths.firstObject;
            return path == nil ? std::string() : std::string(path.fileSystemRepresentation);
        }
    }

    std::string GetFolderPath(SpecialFolder folder)
    {
        switch (folder)
        {
            case SpecialFolder::userCache:
                return GetSearchPath(NSCachesDirectory);
            case SpecialFolder::userConfig:
                return GetSearchPath(NSApplicationSupportDirectory);
            case SpecialFolder::userData:
                return GetSearchPath(NSDocumentDirectory);
            case SpecialFolder::userHome:
                return std::string(NSHomeDirectory().fileSystemRepresentation);
            default:
                return {};
        }
    }

    static std::string GetBundlePath()
    {
        @autoreleasepool
        {
            NSString* resourcePath = NSBundle.mainBundle.resourcePath;
            return resourcePath == nil ? std::string() : std::string(resourcePath.fileSystemRepresentation);
        }
    }

    std::string GetDocsPath()
    {
        return GetBundlePath();
    }

    std::string GetInstallPath()
    {
        if (!gCustomOpenRCT2DataPath.empty())
        {
            return Path::GetAbsolute(gCustomOpenRCT2DataPath);
        }
        return GetBundlePath();
    }

    std::string GetCurrentExecutablePath()
    {
        @autoreleasepool
        {
            NSString* executablePath = NSBundle.mainBundle.executablePath;
            if (executablePath != nil)
            {
                return executablePath.fileSystemRepresentation;
            }
        }

        char path[MAX_PATH];
        uint32_t size = sizeof(path);
        return _NSGetExecutablePath(path, &size) == 0 ? std::string(path) : std::string();
    }

    u8string StrDecompToPrecomp(u8string_view input)
    {
        @autoreleasepool
        {
            auto inputString = u8string(input);
            NSString* decomposed = [NSString stringWithUTF8String:inputString.c_str()];
            const char* precomposed = decomposed.precomposedStringWithCanonicalMapping.UTF8String;
            return precomposed == nullptr ? u8string() : u8string(precomposed);
        }
    }

    bool HandleSpecialCommandLineArgument([[maybe_unused]] const char* argument)
    {
        return false;
    }

    static bool HasMatchingLanguage(NSString* preferredLocale, uint16_t* languageIdentifier)
    {
        if ([preferredLocale isEqualToString:@"en"] || [preferredLocale isEqualToString:@"en-CA"])
        {
            *languageIdentifier = LANGUAGE_ENGLISH_US;
            return true;
        }

        for (int32_t i = 1; i < LANGUAGE_COUNT; i++)
        {
            if ([preferredLocale isEqualToString:[NSString stringWithUTF8String:LanguagesDescriptors[i].locale]])
            {
                *languageIdentifier = static_cast<uint16_t>(i);
                return true;
            }
        }

        NSString* languageCode = [preferredLocale componentsSeparatedByString:@"-"].firstObject;
        for (int32_t i = 1; i < LANGUAGE_COUNT; i++)
        {
            NSString* option = [NSString stringWithUTF8String:LanguagesDescriptors[i].locale];
            if ([languageCode isEqualToString:[option componentsSeparatedByString:@"-"].firstObject])
            {
                *languageIdentifier = static_cast<uint16_t>(i);
                return true;
            }
        }
        return false;
    }

    uint16_t GetLocaleLanguage()
    {
        @autoreleasepool
        {
            for (NSString* preferredLanguage in NSLocale.preferredLanguages)
            {
                uint16_t languageIdentifier;
                if (HasMatchingLanguage(preferredLanguage, &languageIdentifier))
                {
                    return languageIdentifier;
                }
            }
        }
        return LANGUAGE_ENGLISH_UK;
    }

    CurrencyType GetLocaleCurrency()
    {
        @autoreleasepool
        {
            NSString* currencyCode = [NSLocale.currentLocale objectForKey:NSLocaleCurrencyCode];
            return GetCurrencyValue(currencyCode.UTF8String);
        }
    }

    MeasurementFormat GetLocaleMeasurementFormat()
    {
        @autoreleasepool
        {
            NSNumber* usesMetricSystem = [NSLocale.currentLocale objectForKey:NSLocaleUsesMetricSystem];
            return usesMetricSystem.boolValue ? MeasurementFormat::metric : MeasurementFormat::imperial;
        }
    }

    SteamPaths GetSteamPaths()
    {
        return {};
    }

    std::string GetFontPath(const TTFFontDescriptor& font)
    {
        @autoreleasepool
        {
            CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithNameAndSize(
                static_cast<CFStringRef>([NSString stringWithUTF8String:font.font_name]), 0.0);
            CFURLRef url = static_cast<CFURLRef>(CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute));
            CFRelease(descriptor);
            if (url == nullptr)
            {
                return {};
            }

            NSString* path = [static_cast<NSURL*>(CFBridgingRelease(url)) path];
            return path == nil ? std::string() : std::string(path.fileSystemRepresentation);
        }
    }

    std::vector<std::string> GetSearchablePathsRCT1()
    {
        auto documentsPath = GetSearchPath(NSDocumentDirectory);
        if (documentsPath.empty())
        {
            return {};
        }
        return { Path::Combine(documentsPath, u8"rct1") };
    }

    std::vector<std::string> GetSearchablePathsRCT2()
    {
        auto documentsPath = GetSearchPath(NSDocumentDirectory);
        if (documentsPath.empty())
        {
            return {};
        }
        return { Path::Combine(documentsPath, u8"rct2") };
    }
} // namespace OpenRCT2::Platform

#endif // TARGET_OS_IOS
