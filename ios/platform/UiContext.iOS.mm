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

    #include "RCT2Importer.iOS.h"

    #include <openrct2-ui/UiContext.h>

    #include <SDL.h>
    #include <UIKit/UIKit.h>
    #include <algorithm>
    #include <cstring>
    #include <openrct2/Diagnostic.h>
    #include <openrct2/ui/UiContext.h>

namespace OpenRCT2::Ui
{
    static void PushTextInput(NSString* text)
    {
        const char* utf8 = text.UTF8String;
        while (utf8 != nullptr && utf8[0] != '\0')
        {
            SDL_Event event{};
            event.type = SDL_TEXTINPUT;
            const size_t chunkLength = std::min(strlen(utf8), sizeof(event.text.text) - 1);
            memcpy(event.text.text, utf8, chunkLength);
            event.text.text[chunkLength] = '\0';
            SDL_PushEvent(&event);
            utf8 += chunkLength;
        }
    }

    static void PushKey(SDL_Keycode key, SDL_Scancode scancode)
    {
        SDL_Event event{};
        event.type = SDL_KEYDOWN;
        event.key.state = SDL_PRESSED;
        event.key.keysym.sym = key;
        event.key.keysym.scancode = scancode;
        SDL_PushEvent(&event);

        event.type = SDL_KEYUP;
        event.key.state = SDL_RELEASED;
        SDL_PushEvent(&event);
    }

    static UIViewController* GetHostViewController()
    {
        UIViewController* fallback = nil;
        for (UIScene* scene in UIApplication.sharedApplication.connectedScenes)
        {
            if (![scene isKindOfClass:UIWindowScene.class])
            {
                continue;
            }

            for (UIWindow* window in static_cast<UIWindowScene*>(scene).windows)
            {
                if (window.rootViewController == nil)
                {
                    continue;
                }

                fallback = window.rootViewController;
                if (window.isKeyWindow)
                {
                    return window.rootViewController;
                }
            }
        }
        return fallback;
    }

} // namespace OpenRCT2::Ui

@interface OpenRCT2TouchTextField : UITextField <UITextFieldDelegate>
@end

@implementation OpenRCT2TouchTextField

    - (instancetype)init
    {
        self = [super initWithFrame:CGRectMake(0, 0, 1, 1)];
        if (self != nil)
        {
            self.delegate = self;
            self.alpha = 0.01;
            self.textColor = UIColor.clearColor;
            self.tintColor = UIColor.clearColor;
            self.backgroundColor = UIColor.clearColor;
            self.keyboardAppearance = UIKeyboardAppearanceLight;
            self.autocorrectionType = UITextAutocorrectionTypeNo;
            self.spellCheckingType = UITextSpellCheckingTypeNo;
            self.smartDashesType = UITextSmartDashesTypeNo;
            self.smartQuotesType = UITextSmartQuotesTypeNo;
            self.accessibilityElementsHidden = YES;
            self.text = @" ";
        }
        return self;
    }

    - (BOOL)textField:(UITextField*)textField
        shouldChangeCharactersInRange:(NSRange)range
                    replacementString:(NSString*)replacement
    {
        if (replacement.length == 0)
        {
            OpenRCT2::Ui::PushKey(SDLK_BACKSPACE, SDL_SCANCODE_BACKSPACE);
        }
        else
        {
            OpenRCT2::Ui::PushTextInput(replacement);
        }
        return NO;
    }

    - (BOOL)textFieldShouldReturn:(UITextField*)textField
    {
        OpenRCT2::Ui::PushKey(SDLK_RETURN, SDL_SCANCODE_RETURN);
        return NO;
    }

@end

namespace OpenRCT2::Ui
{
    class iOSContext final : public IPlatformUiContext
    {
    private:
        OpenRCT2TouchTextField* _textField = nil;

        void EnsureTextField()
        {
            if (_textField != nil)
            {
                return;
            }

            UIViewController* host = GetHostViewController();
            if (host == nil)
            {
                return;
            }

            _textField = [[OpenRCT2TouchTextField alloc] init];
            [host.view addSubview:_textField];
        }

    public:
        iOSContext()
        {
            SDL_SetHint(SDL_HINT_ORIENTATIONS,
                UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad
                    ? "Portrait PortraitUpsideDown LandscapeLeft LandscapeRight"
                    : "LandscapeLeft LandscapeRight");
            SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2");
            SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
        }

        ~iOSContext() override
        {
            EndTextInput();
            [_textField removeFromSuperview];
            [_textField release];
            _textField = nil;
        }

        void SetWindowIcon([[maybe_unused]] SDL_Window* window) override
        {
        }

        bool IsSteamOverlayAttached() override
        {
            return false;
        }

        void ShowMessageBox(SDL_Window* window, const std::string& message) override
        {
            LOG_INFO("%s", message.c_str());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "OpenRCT2 Touch", message.c_str(), window);
        }

        bool HasMenuSupport() override
        {
            return false;
        }

        int32_t ShowMenuDialog(
            [[maybe_unused]] const std::vector<std::string>& options, [[maybe_unused]] const std::string& title,
            [[maybe_unused]] const std::string& text) override
        {
            return -1;
        }

        void OpenFolder([[maybe_unused]] const std::string& path) override
        {
            LOG_WARNING("Opening folders is not supported on iOS yet.");
        }

        void OpenURL(const std::string& url) override
        {
            @autoreleasepool
            {
                NSURL* target = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
                if (target != nil)
                {
                    [UIApplication.sharedApplication openURL:target options:@{} completionHandler:nil];
                }
            }
        }

        std::string ShowFileDialog(
            [[maybe_unused]] SDL_Window* window, [[maybe_unused]] const FileDialogDesc& desc) override
        {
            LOG_WARNING("The iOS document picker is not implemented yet.");
            return {};
        }

        std::string ShowDirectoryDialog(
            SDL_Window* window, const std::string& title) override
        {
            return ShowRCT2DirectoryImporter(window, title);
        }

        bool HasFilePicker() const override
        {
            return true;
        }

        void BeginTextInput() override
        {
            @autoreleasepool
            {
                EnsureTextField();
                _textField.text = @" ";
                const bool becameFirstResponder = _textField != nil && [_textField becomeFirstResponder];
                LOG_INFO("[OpenRCT2Touch] native text input first_responder=%d", becameFirstResponder);
            }
        }

        void EndTextInput() override
        {
            @autoreleasepool
            {
                [_textField resignFirstResponder];
            }
        }
    };

    std::unique_ptr<IPlatformUiContext> CreatePlatformUiContext()
    {
        return std::make_unique<iOSContext>();
    }
} // namespace OpenRCT2::Ui

#endif // TARGET_OS_IOS
