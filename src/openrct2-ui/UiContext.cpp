/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "UiContext.h"

#include "CursorRepository.h"
#include "SDLException.h"
#include "TextComposition.h"
#include "WindowManager.h"
#include "drawing/engines/DrawingEngineFactory.hpp"
#include "input/ShortcutIds.h"
#include "input/ShortcutManager.h"
#include "interface/Chat.h"
#include "interface/InGameConsole.h"
#include "interface/Theme.h"
#include "interface/ViewportInteraction.h"
#include "scripting/UiExtensions.h"
#include "title/TitleSequencePlayer.h"

#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <openrct2-ui/input/InputManager.h>
#include <openrct2-ui/input/MouseInput.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2/Context.h>
#include <openrct2/Diagnostic.h>
#include <openrct2/Input.h>
#include <openrct2/Version.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/audio/AudioMixer.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/String.hpp>
#include <openrct2/drawing/Drawing.Screen.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/IDrawingEngine.h>
#include <openrct2/drawing/NewDrawing.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/interface/ScreenCoords.hpp>
#include <openrct2/interface/Viewport.h>
#include <openrct2/scenes/title/TitleSequencePlayer.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <vector>

#if defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Ui;

#ifdef __MACOSX__
    // macOS uses COMMAND rather than CTRL for many keyboard shortcuts
    #define KB_PRIMARY_MODIFIER KMOD_GUI
#else
    #define KB_PRIMARY_MODIFIER KMOD_CTRL
#endif

class UiContext final : public IUiContext
{
private:
    constexpr static uint32_t kTouchDoubleTimeout = 390;

#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
    constexpr static int32_t kTouchDragThreshold = 8;
    constexpr static int32_t kTouchPanStartThreshold = 3;
    constexpr static int32_t kTouchConstructionPanStartThreshold = 12;
    constexpr static int32_t kTouchRemovalPanStartThreshold = 10;
    constexpr static int32_t kTouchRemovalDragThreshold = 6;
    constexpr static float kTouchPinchThreshold = 24.0f;
    constexpr static float kTouchPinchSpanRatio = 0.12f;
    constexpr static float kTouchPinchDominance = 1.5f;
    constexpr static float kTouchPanSensitivity = 0.5f;
    constexpr static float kTouchZoomStep = 20.0f;
    constexpr static uint32_t kTouchLongPressTimeout = 500;
    constexpr static int32_t kTouchToolTapDistance = 32;
    constexpr static int32_t kTrackpadViewportWheelStep = 4;
    constexpr static uint32_t kTouchPaintHoldTimeout = 350;
    constexpr static float kTouchRotationThreshold = 0.26179939f;
    constexpr static float kTouchRotationStep = 0.52359878f;
    constexpr static float kTouchRotationDominance = 1.5f;

    enum class TouchGestureMode
    {
        none,
        singlePending,
        singleDrag,
        longPressFired,
        multiPending,
        multiPan,
        multiPinch,
        multiRotate,
        multiRemove,
        suppressed,
    };

    struct TouchPoint
    {
        SDL_FingerID id = -1;
        ScreenCoordsXY position{};
        bool active = false;
    };
#endif

    const std::unique_ptr<IPlatformUiContext> _platformUiContext;
    const std::unique_ptr<IWindowManager> _windowManager;

    CursorRepository _cursorRepository;

    SDL_Window* _window = nullptr;
    int32_t _width = 0;
    int32_t _height = 0;
    ScaleQuality _scaleQuality = ScaleQuality::nearestNeighbour;

    std::vector<Resolution> _fsResolutions;

    bool _steamOverlayActive = false;

    // Input
    InputManager _inputManager;
    ShortcutManager _shortcutManager;
    TextComposition _textComposition;
    CursorState _cursorState = {};
    uint32_t _lastKeyPressed = 0;
    const uint8_t* _keysState = nullptr;
    uint8_t _keysPressed[256] = {};
    [[maybe_unused]] uint32_t _lastGestureTimestamp = 0;
    [[maybe_unused]] float _gestureRadius = 0;

#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
    TouchPoint _primaryTouch;
    TouchPoint _secondaryTouch;
    TouchGestureMode _touchGestureMode = TouchGestureMode::none;
    ScreenCoordsXY _touchGestureStart{};
    ScreenCoordsXY _touchLastCentroid{};
    float _touchPanCursorX = 0;
    float _touchPanCursorY = 0;
    float _touchGestureStartSpan = 0;
    float _touchLastSpan = 0;
    float _touchPinchAccumulator = 0;
    uint32_t _touchGestureStartTimestamp = 0;
    ScreenCoordsXY _lastTouchToolTap{};
    uint32_t _lastTouchToolTapTimestamp = 0;
    int32_t _trackpadViewportWheelAccumulator = 0;
    float _touchGestureStartAngle = 0;
    float _touchLastAngle = 0;
    float _touchRotationAccumulator = 0;
    ScreenCoordsXY _touchLastRemovePosition{};
#endif

    InGameConsole _inGameConsole;
    std::unique_ptr<ITitleSequencePlayer> _titleSequencePlayer;

#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
    ScreenCoordsXY GetTouchPosition(const SDL_TouchFingerEvent& event) const
    {
        return { static_cast<int32_t>(event.x * _width), static_cast<int32_t>(event.y * _height) };
    }

    static int64_t GetTouchDistanceSquared(const ScreenCoordsXY& first, const ScreenCoordsXY& second)
    {
        const int64_t deltaX = first.x - second.x;
        const int64_t deltaY = first.y - second.y;
        return deltaX * deltaX + deltaY * deltaY;
    }

    ScreenCoordsXY GetTouchCentroid() const
    {
        return { (_primaryTouch.position.x + _secondaryTouch.position.x) / 2,
                 (_primaryTouch.position.y + _secondaryTouch.position.y) / 2 };
    }

    float GetTouchSpan() const
    {
        const auto deltaX = static_cast<float>(_primaryTouch.position.x - _secondaryTouch.position.x);
        const auto deltaY = static_cast<float>(_primaryTouch.position.y - _secondaryTouch.position.y);
        return std::hypot(deltaX, deltaY);
    }

    float GetTouchAngle() const
    {
        const auto deltaX = static_cast<float>(_secondaryTouch.position.x - _primaryTouch.position.x);
        const auto deltaY = static_cast<float>(_secondaryTouch.position.y - _primaryTouch.position.y);
        return std::atan2(deltaY, deltaX);
    }

    static float NormaliseTouchAngle(float angle)
    {
        constexpr float pi = 3.14159265f;
        constexpr float twoPi = 2.0f * pi;
        while (angle > pi)
        {
            angle -= twoPi;
        }
        while (angle < -pi)
        {
            angle += twoPi;
        }
        return angle;
    }

    bool IsDeliberatePinch(float span, float centroidDistance) const
    {
        const auto spanDistance = std::abs(span - _touchGestureStartSpan);
        const auto pinchThreshold = std::max(kTouchPinchThreshold, _touchGestureStartSpan * kTouchPinchSpanRatio);
        return spanDistance >= pinchThreshold && spanDistance >= centroidDistance * kTouchPinchDominance;
    }

    bool CanRotateTouchConstruction() const
    {
        if (!gInputFlags.has(InputFlag::toolActive))
        {
            return false;
        }

        switch (gCurrentToolWidget.windowClassification)
        {
            case WindowClass::scenery:
            case WindowClass::rideConstruction:
            case WindowClass::trackDesignPlace:
                return true;
            default:
                return false;
        }
    }

    bool IsDeliberateRotation(float angle, float span, float centroidDistance) const
    {
        if (!CanRotateTouchConstruction())
        {
            return false;
        }

        const auto angleDistance = std::abs(NormaliseTouchAngle(angle - _touchGestureStartAngle));
        const auto rotationDistance = angleDistance * std::max(1.0f, _touchGestureStartSpan * 0.5f);
        const auto spanDistance = std::abs(span - _touchGestureStartSpan);
        const auto pinchThreshold = std::max(kTouchPinchThreshold, _touchGestureStartSpan * kTouchPinchSpanRatio);
        return angleDistance >= kTouchRotationThreshold && rotationDistance >= centroidDistance * kTouchRotationDominance
            && spanDistance < pinchThreshold;
    }

    void RotateTouchConstruction(bool clockwise)
    {
        auto* shortcut = _shortcutManager.getShortcut(ShortcutId::kInterfaceRotateConstruction);
        if (shortcut == nullptr)
        {
            return;
        }

        const int32_t rotations = clockwise ? 1 : 3;
        for (int32_t i = 0; i < rotations; i++)
        {
            shortcut->action();
        }
    }

    void BeginTouchRotation(float angle)
    {
        const auto direction = NormaliseTouchAngle(angle - _touchGestureStartAngle);
        _touchGestureMode = TouchGestureMode::multiRotate;
        _touchLastAngle = angle;
        _touchRotationAccumulator = 0;
        RotateTouchConstruction(direction > 0);
    }

    void ContinueTouchRotation(float angle)
    {
        _touchRotationAccumulator += NormaliseTouchAngle(angle - _touchLastAngle);
        _touchLastAngle = angle;
        while (std::abs(_touchRotationAccumulator) >= kTouchRotationStep)
        {
            const bool clockwise = _touchRotationAccumulator > 0;
            RotateTouchConstruction(clockwise);
            _touchRotationAccumulator += clockwise ? -kTouchRotationStep : kTouchRotationStep;
        }
    }

    bool CanPaintWithTouch(const ScreenCoordsXY& position) const
    {
        if (!gInputFlags.has(InputFlag::toolActive) || ViewportFindFromPoint(position) == nullptr)
        {
            return false;
        }

        switch (gCurrentToolWidget.windowClassification)
        {
            case WindowClass::footpath:
            case WindowClass::land:
            case WindowClass::water:
            case WindowClass::clearScenery:
                return true;
            default:
                return false;
        }
    }

    bool CanUseTouchSecondaryAction(const ScreenCoordsXY& position) const
    {
        return gInputFlags.has(InputFlag::toolActive) && gInputFlags.has(InputFlag::allowRightMouseRemoval)
            && ViewportInteractionRightOver(position);
    }

    bool CanEraseFootpathWithTouch(const ScreenCoordsXY& position) const
    {
        return gCurrentToolWidget.windowClassification == WindowClass::footpath && CanUseTouchSecondaryAction(position);
    }

    void FireTouchSecondaryAction(const ScreenCoordsXY& position)
    {
        _cursorState.position = position;
        _cursorState.touch = true;
        ViewportInteractionRightClick(position);
    }

    void StoreTouchPress(MouseState state, const ScreenCoordsXY& position)
    {
        StoreMouseInput(state, position);
        _cursorState.position = position;
        _cursorState.touch = true;

        if (state == MouseState::leftPress)
        {
            _cursorState.left = CURSOR_PRESSED;
            _cursorState.old = 1;
        }
        else if (state == MouseState::rightPress)
        {
            _cursorState.right = CURSOR_PRESSED;
            _cursorState.old = 2;
        }
    }

    void StoreTouchRelease(MouseState state, const ScreenCoordsXY& position)
    {
        StoreMouseInput(state, position);
        _cursorState.position = position;
        _cursorState.touch = true;

        if (state == MouseState::leftRelease)
        {
            _cursorState.left = CURSOR_RELEASED;
            _cursorState.old = 3;
        }
        else if (state == MouseState::rightRelease)
        {
            _cursorState.right = CURSOR_RELEASED;
            _cursorState.old = 4;
        }
    }

    void FireTouchTap(const ScreenCoordsXY& position)
    {
        StoreTouchPress(MouseState::leftPress, position);
        StoreTouchRelease(MouseState::leftRelease, position);
    }

    void FireTouchLongPress(const ScreenCoordsXY& position)
    {
        _lastTouchToolTapTimestamp = 0;
        StoreTouchPress(MouseState::rightPress, position);
        StoreTouchRelease(MouseState::rightRelease, position);
        _touchGestureMode = TouchGestureMode::longPressFired;
    }

    void HandleTouchTap(const SDL_TouchFingerEvent& event, const ScreenCoordsXY& position)
    {
        const bool isActiveToolViewport = gInputFlags.has(InputFlag::toolActive) && ViewportFindFromPoint(position) != nullptr;
        if (!isActiveToolViewport)
        {
            _lastTouchToolTapTimestamp = 0;
            FireTouchTap(position);
            return;
        }

        const int64_t tapDistanceSquared = kTouchToolTapDistance * kTouchToolTapDistance;
        const bool isDoubleTap = _lastTouchToolTapTimestamp != 0
            && event.timestamp - _lastTouchToolTapTimestamp < kTouchDoubleTimeout
            && GetTouchDistanceSquared(position, _lastTouchToolTap) <= tapDistanceSquared;
        if (isDoubleTap)
        {
            _lastTouchToolTapTimestamp = 0;
            FireTouchTap(position);
        }
        else
        {
            _lastTouchToolTap = position;
            _lastTouchToolTapTimestamp = event.timestamp;
            _cursorState.position = position;
        }
    }

    void AddTrackpadViewportWheel(int32_t delta)
    {
        if ((_trackpadViewportWheelAccumulator < 0 && delta > 0) || (_trackpadViewportWheelAccumulator > 0 && delta < 0))
        {
            _trackpadViewportWheelAccumulator = 0;
        }

        _trackpadViewportWheelAccumulator = std::clamp(
            _trackpadViewportWheelAccumulator + delta, -kTrackpadViewportWheelStep, kTrackpadViewportWheelStep);
        if (std::abs(_trackpadViewportWheelAccumulator) == kTrackpadViewportWheelStep)
        {
            _cursorState.wheel += _trackpadViewportWheelAccumulator > 0 ? 1 : -1;
            _trackpadViewportWheelAccumulator = 0;
        }
    }

    void ResetTouchGesture()
    {
        _primaryTouch = {};
        _secondaryTouch = {};
        _touchGestureMode = TouchGestureMode::none;
        _touchGestureStart = {};
        _touchLastCentroid = {};
        _touchPanCursorX = 0;
        _touchPanCursorY = 0;
        _touchGestureStartSpan = 0;
        _touchLastSpan = 0;
        _touchPinchAccumulator = 0;
        _touchGestureStartTimestamp = 0;
        _touchGestureStartAngle = 0;
        _touchLastAngle = 0;
        _touchRotationAccumulator = 0;
        _touchLastRemovePosition = {};
    }

    void HandleTouchDown(const SDL_TouchFingerEvent& event)
    {
        const auto position = GetTouchPosition(event);
        _cursorState.position = position;
        _cursorState.touch = true;

        if (!_primaryTouch.active)
        {
            _primaryTouch = { event.fingerId, position, true };
            _touchGestureMode = TouchGestureMode::singlePending;
            _touchGestureStart = position;
            _touchGestureStartTimestamp = event.timestamp;
            return;
        }

        if (_secondaryTouch.active || event.fingerId == _primaryTouch.id)
        {
            return;
        }

        if (_touchGestureMode == TouchGestureMode::singleDrag)
        {
            StoreTouchRelease(MouseState::leftRelease, _primaryTouch.position);
        }

        _lastTouchToolTapTimestamp = 0;

        _secondaryTouch = { event.fingerId, position, true };
        _touchGestureMode = TouchGestureMode::multiPending;
        _touchGestureStart = GetTouchCentroid();
        _touchLastCentroid = _touchGestureStart;
        _touchPanCursorX = static_cast<float>(_touchGestureStart.x);
        _touchPanCursorY = static_cast<float>(_touchGestureStart.y);
        _touchGestureStartSpan = GetTouchSpan();
        _touchLastSpan = _touchGestureStartSpan;
        _touchGestureStartAngle = GetTouchAngle();
        _touchLastAngle = _touchGestureStartAngle;
        _touchPinchAccumulator = 0;
        _touchRotationAccumulator = 0;
        _touchLastRemovePosition = _touchGestureStart;
        _touchGestureStartTimestamp = event.timestamp;
        _cursorState.position = _touchGestureStart;
    }

    void HandleTouchMotion(const SDL_TouchFingerEvent& event)
    {
        const auto position = GetTouchPosition(event);
        if (_primaryTouch.active && event.fingerId == _primaryTouch.id)
        {
            _primaryTouch.position = position;
        }
        else if (_secondaryTouch.active && event.fingerId == _secondaryTouch.id)
        {
            _secondaryTouch.position = position;
        }
        else
        {
            return;
        }

        const int64_t dragThresholdSquared = kTouchDragThreshold * kTouchDragThreshold;
        if (_secondaryTouch.active)
        {
            const auto centroid = GetTouchCentroid();
            const auto span = GetTouchSpan();
            const auto angle = GetTouchAngle();
            const auto centroidDistance = std::sqrt(static_cast<float>(GetTouchDistanceSquared(centroid, _touchGestureStart)));

            if (_touchGestureMode == TouchGestureMode::multiRemove)
            {
                const int64_t removalThresholdSquared = kTouchRemovalDragThreshold * kTouchRemovalDragThreshold;
                if (GetTouchDistanceSquared(centroid, _touchLastRemovePosition) >= removalThresholdSquared)
                {
                    if (CanEraseFootpathWithTouch(centroid))
                    {
                        FireTouchSecondaryAction(centroid);
                    }
                    _touchLastRemovePosition = centroid;
                }
                _cursorState.position = centroid;
                return;
            }

            if (_touchGestureMode == TouchGestureMode::multiPending)
            {
                if (IsDeliberatePinch(span, centroidDistance))
                {
                    _touchGestureMode = TouchGestureMode::multiPinch;
                    _touchPinchAccumulator = span - _touchGestureStartSpan;
                    _touchLastSpan = span;
                }
                else if (IsDeliberateRotation(angle, span, centroidDistance))
                {
                    BeginTouchRotation(angle);
                }
                else
                {
                    int32_t panThreshold = kTouchPanStartThreshold;
                    if (CanRotateTouchConstruction())
                    {
                        panThreshold = kTouchConstructionPanStartThreshold;
                    }
                    else if (CanEraseFootpathWithTouch(_touchGestureStart))
                    {
                        panThreshold = kTouchRemovalPanStartThreshold;
                    }

                    if (centroidDistance >= panThreshold)
                    {
                        StoreTouchPress(MouseState::rightPress, _touchGestureStart);
                        _touchGestureMode = TouchGestureMode::multiPan;
                    }
                }
            }

            if (_touchGestureMode == TouchGestureMode::multiPan)
            {
                if (IsDeliberatePinch(span, centroidDistance))
                {
                    StoreTouchRelease(MouseState::rightRelease, _cursorState.position);
                    _touchGestureMode = TouchGestureMode::multiPinch;
                    _touchPinchAccumulator = span - _touchGestureStartSpan;
                    _touchLastSpan = span;
                    _cursorState.position = centroid;
                }
                else if (IsDeliberateRotation(angle, span, centroidDistance))
                {
                    StoreTouchRelease(MouseState::rightRelease, _cursorState.position);
                    BeginTouchRotation(angle);
                    _cursorState.position = centroid;
                }
                else
                {
                    const auto delta = centroid - _touchLastCentroid;
                    _touchLastCentroid = centroid;
                    _touchPanCursorX -= delta.x * kTouchPanSensitivity;
                    _touchPanCursorY -= delta.y * kTouchPanSensitivity;
                    _cursorState.position = { static_cast<int32_t>(std::lround(_touchPanCursorX)),
                                              static_cast<int32_t>(std::lround(_touchPanCursorY)) };
                }
            }

            if (_touchGestureMode == TouchGestureMode::multiPinch)
            {
                _touchPinchAccumulator += span - _touchLastSpan;
                _touchLastSpan = span;
                _cursorState.position = centroid;

                while (std::abs(_touchPinchAccumulator) >= kTouchZoomStep)
                {
                    const bool zoomIn = _touchPinchAccumulator > 0;
                    Windows::MainWindowZoom(zoomIn, true);
                    _touchPinchAccumulator += zoomIn ? -kTouchZoomStep : kTouchZoomStep;
                }
            }
            else if (_touchGestureMode == TouchGestureMode::multiRotate)
            {
                ContinueTouchRotation(angle);
                _cursorState.position = centroid;
            }
            else if (_touchGestureMode == TouchGestureMode::multiPending)
            {
                _cursorState.position = centroid;
            }
            return;
        }

        if (_touchGestureMode == TouchGestureMode::suppressed)
        {
            _cursorState.position = position;
            return;
        }

        if (_touchGestureMode == TouchGestureMode::singlePending
            && GetTouchDistanceSquared(position, _touchGestureStart) >= dragThresholdSquared)
        {
            if (ViewportFindFromPoint(_touchGestureStart) != nullptr)
            {
                _lastTouchToolTapTimestamp = 0;
                _touchGestureMode = TouchGestureMode::suppressed;
            }
            else
            {
                StoreTouchPress(MouseState::leftPress, _touchGestureStart);
                _touchGestureMode = TouchGestureMode::singleDrag;
            }
        }
        _cursorState.position = position;
    }

    void SuppressRemainingTouch(SDL_FingerID releasedFingerId)
    {
        if (_primaryTouch.active && releasedFingerId == _primaryTouch.id && _secondaryTouch.active)
        {
            _primaryTouch = _secondaryTouch;
            _secondaryTouch = {};
        }
        else if (_secondaryTouch.active && releasedFingerId == _secondaryTouch.id)
        {
            _secondaryTouch = {};
        }
        else
        {
            _primaryTouch = {};
        }

        if (_primaryTouch.active)
        {
            _touchGestureMode = TouchGestureMode::suppressed;
        }
        else
        {
            ResetTouchGesture();
        }
    }

    void HandleTouchUp(const SDL_TouchFingerEvent& event)
    {
        const auto position = GetTouchPosition(event);
        const bool isPrimary = _primaryTouch.active && event.fingerId == _primaryTouch.id;
        const bool isSecondary = _secondaryTouch.active && event.fingerId == _secondaryTouch.id;
        if (!isPrimary && !isSecondary)
        {
            return;
        }

        if (_secondaryTouch.active)
        {
            if (_touchGestureMode == TouchGestureMode::multiPan)
            {
                StoreTouchRelease(MouseState::rightRelease, _cursorState.position);
            }
            else if (_touchGestureMode == TouchGestureMode::multiPending)
            {
                const auto centroid = GetTouchCentroid();
                if (CanUseTouchSecondaryAction(centroid))
                {
                    FireTouchSecondaryAction(centroid);
                }
            }
            SuppressRemainingTouch(event.fingerId);
            return;
        }

        switch (_touchGestureMode)
        {
            case TouchGestureMode::singlePending:
                if (event.timestamp - _touchGestureStartTimestamp >= kTouchLongPressTimeout)
                {
                    FireTouchLongPress(position);
                }
                else
                {
                    HandleTouchTap(event, position);
                }
                break;
            case TouchGestureMode::singleDrag:
                StoreTouchRelease(MouseState::leftRelease, position);
                break;
            case TouchGestureMode::suppressed:
            case TouchGestureMode::longPressFired:
            case TouchGestureMode::none:
            case TouchGestureMode::multiPending:
            case TouchGestureMode::multiPan:
            case TouchGestureMode::multiPinch:
            case TouchGestureMode::multiRotate:
            case TouchGestureMode::multiRemove:
                break;
        }
        ResetTouchGesture();
    }

    void HandleTouchHold()
    {
        if (_touchGestureMode == TouchGestureMode::multiPending && _primaryTouch.active && _secondaryTouch.active)
        {
            const auto holdDuration = SDL_GetTicks() - _touchGestureStartTimestamp;
            const auto centroid = GetTouchCentroid();
            if (holdDuration >= kTouchPaintHoldTimeout && CanEraseFootpathWithTouch(centroid))
            {
                _touchGestureMode = TouchGestureMode::multiRemove;
                _touchLastRemovePosition = centroid;
                FireTouchSecondaryAction(centroid);
            }
            return;
        }

        if (_touchGestureMode != TouchGestureMode::singlePending || !_primaryTouch.active)
        {
            return;
        }

        const auto holdDuration = SDL_GetTicks() - _touchGestureStartTimestamp;
        if (holdDuration >= kTouchPaintHoldTimeout && CanPaintWithTouch(_primaryTouch.position))
        {
            _lastTouchToolTapTimestamp = 0;
            StoreTouchPress(MouseState::leftPress, _primaryTouch.position);
            _touchGestureMode = TouchGestureMode::singleDrag;
        }
        else if (holdDuration >= kTouchLongPressTimeout)
        {
            FireTouchLongPress(_primaryTouch.position);
        }
    }
#endif

public:
    InGameConsole& GetInGameConsole()
    {
        return _inGameConsole;
    }

    InputManager& GetInputManager()
    {
        return _inputManager;
    }

    ShortcutManager& GetShortcutManager()
    {
        return _shortcutManager;
    }

    explicit UiContext(IPlatformEnvironment& env)
        : _platformUiContext(CreatePlatformUiContext())
        , _windowManager(CreateWindowManager())
        , _shortcutManager(env)
    {
        LogSDLVersion();
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        {
            SDLException::Throw("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)");
        }
        _cursorRepository.LoadCursors();
        _shortcutManager.loadUserBindings();
    }

    ~UiContext() override
    {
        UiContext::CloseWindow();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void InitialiseScriptExtensions() override
    {
#ifdef ENABLE_SCRIPTING
        auto& scriptEngine = GetContext()->GetScriptEngine();
        Scripting::UiScriptExtensions::Extend(scriptEngine);
#endif
    }

    void Tick() override
    {
        _inGameConsole.Update();

        _windowManager->UpdateMapTooltip();

        WindowDispatchUpdateAll();
    }

    void Draw(RenderTarget& rt) override
    {
        auto bgColour = ThemeGetColour(WindowClass::chat, 0);
        ChatDraw(rt, bgColour);
        _inGameConsole.Draw(rt);
    }

    // Window
    void* GetWindow() override
    {
        return _window;
    }

    int32_t GetWidth() override
    {
        return _width;
    }

    int32_t GetHeight() override
    {
        return _height;
    }

    ScaleQuality GetScaleQuality() override
    {
        return _scaleQuality;
    }

    void SetFullscreenMode(FullscreenMode mode) override
    {
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        // UIKit owns the main window geometry. Applying saved desktop window sizes here
        // produces a logical canvas whose aspect ratio differs from the Metal drawable.
        static_cast<void>(mode);
#else
    #ifndef __EMSCRIPTEN__
        static constexpr int32_t kSDLFullscreenFlags[] = {
            0,
            SDL_WINDOW_FULLSCREEN,
            SDL_WINDOW_FULLSCREEN_DESKTOP,
        };
        uint32_t windowFlags = kSDLFullscreenFlags[EnumValue(mode)];

        // HACK Changing window size when in fullscreen usually has no effect
        if (mode == FullscreenMode::fullscreen)
        {
            SDL_SetWindowFullscreen(_window, 0);

            // Set window size
            UpdateFullscreenResolutions();
            Resolution resolution = GetClosestResolution(
                Config::Get().general.fullscreenWidth, Config::Get().general.fullscreenHeight);
            SDL_SetWindowSize(_window, resolution.Width, resolution.Height);
        }
        else if (mode == FullscreenMode::windowed)
        {
            SDL_SetWindowSize(_window, Config::Get().general.windowWidth, Config::Get().general.windowHeight);
        }

        if (SDL_SetWindowFullscreen(_window, windowFlags))
        {
            LOG_FATAL("SDL_SetWindowFullscreen %s", SDL_GetError());
            exit(1);

            // TODO try another display mode rather than just exiting the game
        }
    #else
        if (mode == FullscreenMode::fullscreen)
        {
            emscripten_request_fullscreen("!canvas", false);
        }
        else if (mode == FullscreenMode::windowed)
        {
            emscripten_exit_fullscreen();
        }
    #endif // __EMSCRIPTEN__
#endif
    }

    const std::vector<Resolution>& GetFullscreenResolutions() override
    {
        UpdateFullscreenResolutions();
        return _fsResolutions;
    }

    bool HasFocus() override
    {
        uint32_t windowFlags = GetWindowFlags();
        return (windowFlags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    void requestUserAttention() override
    {
        SDL_FlashWindow(_window, SDL_FLASH_UNTIL_FOCUSED);
    }

    bool IsMinimised() override
    {
        uint32_t windowFlags = GetWindowFlags();
        return (windowFlags & SDL_WINDOW_MINIMIZED) || (windowFlags & SDL_WINDOW_HIDDEN);
    }

    bool IsSteamOverlayActive() override
    {
        return _steamOverlayActive;
    }

    // Input
    const CursorState* GetCursorState() override
    {
        return &_cursorState;
    }

    const uint8_t* GetKeysState() override
    {
        return _keysState;
    }

    const uint8_t* GetKeysPressed() override
    {
        return _keysPressed;
    }

    CursorID GetCursor() override
    {
        return _cursorRepository.GetCurrentCursor();
    }

    void SetCursor(CursorID cursor) override
    {
        _cursorRepository.SetCurrentCursor(cursor);
    }

    void SetCursorScale(uint8_t scale) override
    {
        _cursorRepository.SetCursorScale(scale);
    }

    void SetCursorVisible(bool value) override
    {
        SDL_ShowCursor(value ? SDL_ENABLE : SDL_DISABLE);
    }

    ScreenCoordsXY GetCursorPosition() override
    {
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        if (_touchGestureMode == TouchGestureMode::multiPan)
        {
            return _cursorState.position;
        }
#endif
        ScreenCoordsXY cursorPosition;
        SDL_GetMouseState(&cursorPosition.x, &cursorPosition.y);
        return cursorPosition;
    }

    void SetCursorPosition(const ScreenCoordsXY& cursorPosition) override
    {
        SDL_WarpMouseInWindow(nullptr, cursorPosition.x, cursorPosition.y);
    }

    void SetCursorTrap(bool value) override
    {
        SDL_SetWindowGrab(_window, value ? SDL_TRUE : SDL_FALSE);
    }

    void SetKeysPressed(uint32_t keysym, uint8_t scancode) override
    {
        _lastKeyPressed = keysym;
        _keysPressed[scancode] = 1;
    }

    // Drawing
    std::shared_ptr<IDrawingEngineFactory> GetDrawingEngineFactory() override
    {
        return std::make_shared<DrawingEngineFactory>();
    }

    void DrawWeatherAnimation(IWeatherDrawer* weatherDrawer, RenderTarget& rt, DrawWeatherFunc drawFunc) override
    {
        int32_t left = rt.x;
        int32_t right = left + rt.width;
        int32_t top = rt.y;
        int32_t bottom = top + rt.height;

        for (auto& w : gWindowList)
        {
            DrawWeatherWindow(rt, weatherDrawer, w.get(), left, right, top, bottom, drawFunc);
        }
    }

    // Text input
    bool IsTextInputActive() override
    {
        return _textComposition.IsActive();
    }

    TextInputSession* StartTextInput(u8string& buffer, size_t maxLength) override
    {
        auto* session = _textComposition.Start(buffer, maxLength);
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        _platformUiContext->BeginTextInput();
        LOG_INFO(
            "[OpenRCT2Touch] text-input: active=%d screen_supported=%d screen_requested=%d", SDL_IsTextInputActive(),
            SDL_HasScreenKeyboardSupport(), SDL_IsScreenKeyboardShown(_window));
#endif
        return session;
    }

    void StopTextInput() override
    {
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        _platformUiContext->EndTextInput();
#endif
        _textComposition.Stop();
    }

    void ProcessMessages() override
    {
        _lastKeyPressed = 0;
        _cursorState.left &= ~CURSOR_CHANGED;
        _cursorState.middle &= ~CURSOR_CHANGED;
        _cursorState.right &= ~CURSOR_CHANGED;
        _cursorState.old = 0;

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    ContextQuit();
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        LOG_VERBOSE("New Window size: %ux%u\n", e.window.data1, e.window.data2);
                        OnResize(e.window.data1, e.window.data2);
                    }

                    switch (e.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_MOVED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        case SDL_WINDOWEVENT_RESTORED:
                        {
                            // Update default display index
                            int32_t displayIndex = SDL_GetWindowDisplayIndex(_window);
                            if (displayIndex != Config::Get().general.defaultDisplay)
                            {
                                Config::Get().general.defaultDisplay = displayIndex;
                                Config::Save();
                            }
                            break;
                        }
                    }

                    if (Config::Get().sound.audioFocus)
                    {
                        if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                        {
                            SetAudioVolume(1);
                        }
                        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                        {
                            SetAudioVolume(0);
                        }
                    }
                    break;
                case SDL_MOUSEMOTION:
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    if (e.motion.which == SDL_TOUCH_MOUSEID)
                    {
                        // SDL mirrors finger motion as mouse motion on iOS. The touch gesture layer has already
                        // applied pan direction and sensitivity, so accepting this event would overwrite both.
                        break;
                    }
#endif
                    _cursorState.position = { static_cast<int32_t>(e.motion.x / Config::Get().general.windowScale),
                                              static_cast<int32_t>(e.motion.y / Config::Get().general.windowScale) };
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    _cursorState.touch = false;
#endif
                    break;
                case SDL_MOUSEWHEEL:
                    if (_inGameConsole.IsOpen())
                    {
                        _inGameConsole.Scroll(e.wheel.y * 3); // Scroll 3 lines at a time
                        break;
                    }
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    if (ViewportFindFromPoint(_cursorState.position) != nullptr)
                    {
                        AddTrackpadViewportWheel(-e.wheel.y);
                    }
                    else
                    {
                        _trackpadViewportWheelAccumulator = 0;
                        _cursorState.wheel -= e.wheel.y;
                    }
#else
                    _cursorState.wheel -= e.wheel.y;
#endif
                    break;
                case SDL_MOUSEBUTTONDOWN:
                {
                    if (e.button.which == SDL_TOUCH_MOUSEID)
                    {
                        break;
                    }
                    ScreenCoordsXY mousePos = { static_cast<int32_t>(e.button.x / Config::Get().general.windowScale),
                                                static_cast<int32_t>(e.button.y / Config::Get().general.windowScale) };
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            StoreMouseInput(MouseState::leftPress, mousePos);
                            _cursorState.left = CURSOR_PRESSED;
                            _cursorState.old = 1;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            _cursorState.middle = CURSOR_PRESSED;
                            break;
                        case SDL_BUTTON_RIGHT:
                            StoreMouseInput(MouseState::rightPress, mousePos);
                            _cursorState.right = CURSOR_PRESSED;
                            _cursorState.old = 2;
                            break;
                    }
                    _cursorState.touch = false;

                    {
                        InputEvent ie;
                        ie.deviceKind = InputDeviceKind::mouse;
                        ie.modifiers = SDL_GetModState();
                        ie.button = e.button.button;
                        ie.state = InputEventState::down;
                        _inputManager.queueInputEvent(std::move(ie));
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    if (e.button.which == SDL_TOUCH_MOUSEID)
                    {
                        break;
                    }
                    ScreenCoordsXY mousePos = { static_cast<int32_t>(e.button.x / Config::Get().general.windowScale),
                                                static_cast<int32_t>(e.button.y / Config::Get().general.windowScale) };
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            StoreMouseInput(MouseState::leftRelease, mousePos);
                            _cursorState.left = CURSOR_RELEASED;
                            _cursorState.old = 3;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            _cursorState.middle = CURSOR_RELEASED;
                            break;
                        case SDL_BUTTON_RIGHT:
                            StoreMouseInput(MouseState::rightRelease, mousePos);
                            _cursorState.right = CURSOR_RELEASED;
                            _cursorState.old = 4;
                            break;
                    }
                    _cursorState.touch = false;

                    {
                        InputEvent ie;
                        ie.deviceKind = InputDeviceKind::mouse;
                        ie.modifiers = SDL_GetModState();
                        ie.button = e.button.button;
                        ie.state = InputEventState::release;
                        _inputManager.queueInputEvent(std::move(ie));
                    }
                    break;
                }
                // Apple sends touchscreen events for trackpads, so ignore these events on macOS
#ifndef __MACOSX__
                case SDL_FINGERMOTION:
    #if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    HandleTouchMotion(e.tfinger);
    #else
                    _cursorState.position = { static_cast<int32_t>(e.tfinger.x * _width),
                                              static_cast<int32_t>(e.tfinger.y * _height) };
    #endif
                    break;
                case SDL_FINGERDOWN:
                {
    #if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    HandleTouchDown(e.tfinger);
    #else
                    ScreenCoordsXY fingerPos = { static_cast<int32_t>(e.tfinger.x * _width),
                                                 static_cast<int32_t>(e.tfinger.y * _height) };

                    _cursorState.touchIsDouble
                        = (!_cursorState.touchIsDouble
                           && e.tfinger.timestamp - _cursorState.touchDownTimestamp < kTouchDoubleTimeout);

                    if (_cursorState.touchIsDouble)
                    {
                        StoreMouseInput(MouseState::rightPress, fingerPos);
                        _cursorState.right = CURSOR_PRESSED;
                        _cursorState.old = 2;
                    }
                    else
                    {
                        StoreMouseInput(MouseState::leftPress, fingerPos);
                        _cursorState.left = CURSOR_PRESSED;
                        _cursorState.old = 1;
                    }
                    _cursorState.touch = true;
                    _cursorState.touchDownTimestamp = e.tfinger.timestamp;
    #endif
                    break;
                }
                case SDL_FINGERUP:
                {
    #if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    HandleTouchUp(e.tfinger);
    #else
                    ScreenCoordsXY fingerPos = { static_cast<int32_t>(e.tfinger.x * _width),
                                                 static_cast<int32_t>(e.tfinger.y * _height) };

                    if (_cursorState.touchIsDouble)
                    {
                        StoreMouseInput(MouseState::rightRelease, fingerPos);
                        _cursorState.right = CURSOR_RELEASED;
                        _cursorState.old = 4;
                    }
                    else
                    {
                        StoreMouseInput(MouseState::leftRelease, fingerPos);
                        _cursorState.left = CURSOR_RELEASED;
                        _cursorState.old = 3;
                    }
                    _cursorState.touch = true;
    #endif
                    break;
                }
#endif
                case SDL_KEYDOWN:
                {
#ifndef __MACOSX__
                    // Ignore winkey keydowns. Handles edge case where tiling
                    // window managers don't eat the keypresses when changing
                    // workspaces.
                    if (SDL_GetModState() & KMOD_GUI)
                    {
                        break;
                    }
#endif
                    _textComposition.HandleMessage(&e);
                    auto ie = GetInputEventFromSDLEvent(e);
                    ie.state = InputEventState::down;
                    _inputManager.queueInputEvent(std::move(ie));
                    break;
                }
                case SDL_KEYUP:
                {
                    auto ie = GetInputEventFromSDLEvent(e);
                    ie.state = InputEventState::release;
                    _inputManager.queueInputEvent(std::move(ie));
                    break;
                }
                case SDL_MULTIGESTURE:
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
                    // iPadOS touch handling classifies two-finger motion as pan or pinch above.
#else
                    if (e.mgesture.numFingers == 2)
                    {
                        if (e.mgesture.timestamp > _lastGestureTimestamp + 1000)
                        {
                            _gestureRadius = 0;
                        }
                        _lastGestureTimestamp = e.mgesture.timestamp;
                        _gestureRadius += e.mgesture.dDist;

                        // Zoom gesture
                        constexpr int32_t tolerance = 128;
                        int32_t gesturePixels = static_cast<int32_t>(_gestureRadius * _width);
                        if (abs(gesturePixels) > tolerance)
                        {
                            _gestureRadius = 0;
                            Windows::MainWindowZoom(gesturePixels > 0, true);
                        }
                    }
#endif
                    break;
                case SDL_TEXTEDITING:
                    _textComposition.HandleMessage(&e);
                    break;
                case SDL_TEXTINPUT:
                    _textComposition.HandleMessage(&e);
                    break;
                default:
                {
                    _inputManager.queueInputEvent(e);
                    break;
                }
            }
        }

#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        HandleTouchHold();
#endif

        _cursorState.any = _cursorState.left | _cursorState.middle | _cursorState.right;

        // Updates the state of the keys
        int32_t numKeys = 256;
        _keysState = SDL_GetKeyboardState(&numKeys);
    }

    /**
     * Helper function to set various render target features.
     * Does not get triggered on resize, but rather manually on config changes.
     */
    void TriggerResize() override
    {
        char scaleQualityBuffer[4];
        _scaleQuality = ScaleQuality::smoothNearestNeighbour;
        if (Config::Get().general.windowScale == std::floor(Config::Get().general.windowScale))
        {
            _scaleQuality = ScaleQuality::nearestNeighbour;
        }

        ScaleQuality scaleQuality = _scaleQuality;
        if (_scaleQuality == ScaleQuality::smoothNearestNeighbour)
        {
            scaleQuality = ScaleQuality::linear;
        }
        snprintf(scaleQualityBuffer, sizeof(scaleQualityBuffer), "%d", static_cast<int32_t>(scaleQuality));
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scaleQualityBuffer);

        int32_t width, height;
        SDL_GetWindowSize(_window, &width, &height);
        OnResize(width, height);
    }

    void CreateWindow() override
    {
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, Config::Get().general.minimizeFullscreenFocusLoss ? "1" : "0");

        // Set window position to default display
        int32_t defaultDisplay = std::clamp(Config::Get().general.defaultDisplay, 0, 0xFFFF);
        auto windowPos = ScreenCoordsXY{ static_cast<int32_t>(SDL_WINDOWPOS_UNDEFINED_DISPLAY(defaultDisplay)),
                                         static_cast<int32_t>(SDL_WINDOWPOS_UNDEFINED_DISPLAY(defaultDisplay)) };

        CreateWindow(windowPos);

        // Check if steam overlay renderer is loaded into the process
        _steamOverlayActive = _platformUiContext->IsSteamOverlayAttached();
    }

    void CloseWindow() override
    {
        DrawingEngineDispose();
        if (_window != nullptr)
        {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }
    }

    void RecreateWindow() override
    {
        // Use the position of the current window for the new window
        ScreenCoordsXY windowPos;
        SDL_SetWindowFullscreen(_window, 0);
        SDL_GetWindowPosition(_window, &windowPos.x, &windowPos.y);

        CloseWindow();
        CreateWindow(windowPos);
    }

    void ShowMessageBox(const std::string& message) override
    {
        _platformUiContext->ShowMessageBox(_window, message);
    }

    int32_t ShowMessageBox(
        const std::string& title, const std::string& message, const std::vector<std::string>& options) override
    {
        auto message_box_button_data = std::make_unique<SDL_MessageBoxButtonData[]>(options.size());
        for (size_t i = 0; i < options.size(); i++)
        {
            message_box_button_data[i].buttonid = static_cast<int>(i);
            message_box_button_data[i].text = options[i].c_str();
        }

        SDL_MessageBoxData message_box_data{};
        message_box_data.window = _window;
        message_box_data.title = title.c_str();
        message_box_data.message = message.c_str();
        message_box_data.numbuttons = static_cast<int>(options.size());
        message_box_data.buttons = message_box_button_data.get();

        int buttonid{};

        SDL_ShowMessageBox(&message_box_data, &buttonid);

        return buttonid;
    }

    bool HasMenuSupport() override
    {
        return _platformUiContext->HasMenuSupport();
    }

    int32_t ShowMenuDialog(const std::vector<std::string>& options, const std::string& title, const std::string& text) override
    {
        return _platformUiContext->ShowMenuDialog(options, title, text);
    }

    void OpenFolder(const std::string& path) override
    {
        _platformUiContext->OpenFolder(path);
    }

    void OpenURL(const std::string& url) override
    {
        _platformUiContext->OpenURL(url);
    }

    std::string ShowFileDialog(const FileDialogDesc& desc) override
    {
        return _platformUiContext->ShowFileDialog(_window, desc);
    }

    std::string ShowDirectoryDialog(const std::string& title) override
    {
        return _platformUiContext->ShowDirectoryDialog(_window, title);
    }

    bool HasFilePicker() const override
    {
        return _platformUiContext->HasFilePicker();
    }

    IWindowManager* GetWindowManager() override
    {
        return _windowManager.get();
    }

    bool SetClipboardText(const utf8* target) override
    {
#ifndef __EMSCRIPTEN__
        return (SDL_SetClipboardText(target) == 0);
#else
        return (
            MAIN_THREAD_EM_ASM_INT(
                {
                    try
                    {
                        navigator.clipboard.writeText(UTF8ToString($0));
                        return 0;
                    }
                    catch (e)
                    {
                        return -1;
                    };
                },
                target)
            == 0);
#endif
    }

    ITitleSequencePlayer* GetTitleSequencePlayer() override
    {
        if (_titleSequencePlayer == nullptr)
        {
            _titleSequencePlayer = Title::CreateTitleSequencePlayer();
        }
        return _titleSequencePlayer.get();
    }

private:
    void LogSDLVersion()
    {
        SDL_version version{};
        SDL_GetVersion(&version);
        LOG_VERBOSE("SDL2 version: %d.%d.%d", version.major, version.minor, version.patch);
    }

    void InferDisplayDPI()
    {
        auto& config = Config::Get().general;
        if (!config.inferDisplayDPI)
            return;

        int wWidth, wHeight;
        SDL_GetWindowSize(_window, &wWidth, &wHeight);

        auto renderer = SDL_GetRenderer(_window);
        int rWidth, rHeight;
        if (SDL_GetRendererOutputSize(renderer, &rWidth, &rHeight) == 0)
            config.windowScale = rWidth / wWidth;

        config.inferDisplayDPI = false;
        Config::Save();
    }

    void CreateWindow(const ScreenCoordsXY& windowPos)
    {
#ifdef __EMSCRIPTEN__
        MAIN_THREAD_EM_ASM({
            Module.canvas.width = window.innerWidth;
            Module.canvas.height = window.innerHeight;
        });
        int32_t width = 0;
        int32_t height = 0;
        emscripten_get_canvas_element_size("!canvas", &width, &height);
#else
        // Get saved window size
        int32_t width = Config::Get().general.windowWidth;
        int32_t height = Config::Get().general.windowHeight;
#endif

        // Set defaults if size is invalid
        if (width <= 0)
            width = 1280;
        if (height <= 0)
            height = 720;

        // Create window in window first rather than fullscreen so we have the display the window is on first
        uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        flags |= SDL_WINDOW_BORDERLESS;
#endif
        if (Config::Get().general.drawingEngine == DrawingEngine::openGL)
        {
            flags |= SDL_WINDOW_OPENGL;
        }

        _window = SDL_CreateWindow(OPENRCT2_NAME, windowPos.x, windowPos.y, width, height, flags);
        if (_window == nullptr)
        {
            const char* error = SDL_GetError();
            std::string errorMessage = String::stdFormat(
                "SDL_CreateWindow(" OPENRCT2_NAME ", %d, %d, %d, %d, %d) failed: %s", windowPos.x, windowPos.y, width, height,
                flags, error);
            SDLException::Throw(errorMessage.c_str());
        }

#if defined(__APPLE__) && defined(__MACH__) && TARGET_OS_IOS
        SDL_GetWindowSize(_window, &width, &height);
#endif

        ApplyScreenSaverLockSetting();

        SDL_SetWindowMinimumSize(_window, 720, 480);
        SetCursorTrap(Config::Get().general.trapCursor);
        _platformUiContext->SetWindowIcon(_window);

        // Initialise the surface, palette and draw buffer
        DrawingEngineInit();
        InferDisplayDPI();
        OnResize(width, height);

        UpdateFullscreenResolutions();

        SetFullscreenMode(static_cast<FullscreenMode>(Config::Get().general.fullscreenMode));
        TriggerResize();
    }

    void OnResize(int32_t width, int32_t height)
    {
        // Scale the native window size to the game's canvas size
        _width = static_cast<int32_t>(width / Config::Get().general.windowScale);
        _height = static_cast<int32_t>(height / Config::Get().general.windowScale);

        DrawingEngineResize();

        uint32_t flags = SDL_GetWindowFlags(_window);
        if ((flags & SDL_WINDOW_MINIMIZED) == 0)
        {
            WindowResizeGui(_width, _height);
            Windows::WindowRelocateWindows(_width, _height);
        }

        GfxInvalidateScreen();

        // Check if the window has been resized in windowed mode and update the config file accordingly
        int32_t nonWindowFlags =
#ifndef __MACOSX__
            SDL_WINDOW_MAXIMIZED |
#endif
            SDL_WINDOW_MINIMIZED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;

        if (!(flags & nonWindowFlags))
        {
            if (width != Config::Get().general.windowWidth || height != Config::Get().general.windowHeight)
            {
                Config::Get().general.windowWidth = width;
                Config::Get().general.windowHeight = height;
                Config::Save();
            }
        }
    }

    void UpdateFullscreenResolutions()
    {
        // Query number of display modes
        int32_t displayIndex = SDL_GetWindowDisplayIndex(_window);
        int32_t numDisplayModes = SDL_GetNumDisplayModes(displayIndex);

        // Get desktop aspect ratio
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(displayIndex, &mode);

        // Get resolutions
        auto resolutions = std::vector<Resolution>();
        float desktopAspectRatio = static_cast<float>(mode.w) / mode.h;
        for (int32_t i = 0; i < numDisplayModes; i++)
        {
            SDL_GetDisplayMode(displayIndex, i, &mode);
            if (mode.w > 0 && mode.h > 0)
            {
                float aspectRatio = static_cast<float>(mode.w) / mode.h;
                if (std::fabs(desktopAspectRatio - aspectRatio) < 0.1f)
                {
                    resolutions.push_back({ mode.w, mode.h });
                }
            }
        }

        // Sort by area
        std::sort(resolutions.begin(), resolutions.end(), [](const Resolution& a, const Resolution& b) -> bool {
            int32_t areaA = a.Width * a.Height;
            int32_t areaB = b.Width * b.Height;
            return areaA < areaB;
        });

        // Remove duplicates
        auto last = std::unique(resolutions.begin(), resolutions.end(), [](const Resolution& a, const Resolution& b) -> bool {
            return (a.Width == b.Width && a.Height == b.Height);
        });
        resolutions.erase(last, resolutions.end());

        // Update config fullscreen resolution if not set
        if (!resolutions.empty()
            && (Config::Get().general.fullscreenWidth == -1 || Config::Get().general.fullscreenHeight == -1))
        {
            Config::Get().general.fullscreenWidth = resolutions.back().Width;
            Config::Get().general.fullscreenHeight = resolutions.back().Height;
        }

        _fsResolutions = resolutions;
    }

    Resolution GetClosestResolution(int32_t inWidth, int32_t inHeight)
    {
        Resolution result = { 640, 480 };
        int32_t closestAreaDiff = -1;
        int32_t destinationArea = inWidth * inHeight;
        for (const Resolution& resolution : _fsResolutions)
        {
            // Check if exact match
            if (resolution.Width == inWidth && resolution.Height == inHeight)
            {
                result = resolution;
                break;
            }

            // Check if area is closer to best match
            int32_t areaDiff = std::abs((resolution.Width * resolution.Height) - destinationArea);
            if (closestAreaDiff == -1 || areaDiff < closestAreaDiff)
            {
                closestAreaDiff = areaDiff;
                result = resolution;
            }
        }
        return result;
    }

    uint32_t GetWindowFlags()
    {
        return SDL_GetWindowFlags(_window);
    }

    static void DrawWeatherWindow(
        RenderTarget& rt, IWeatherDrawer* weatherDrawer, WindowBase* original_w, int16_t left, int16_t right, int16_t top,
        int16_t bottom, DrawWeatherFunc drawFunc)
    {
        WindowBase* w{};
        auto itStart = WindowGetIterator(original_w);
        for (auto it = std::next(itStart);; it++)
        {
            if (it == gWindowList.end())
            {
                // Loop ended, draw weather for original_w
                auto vp = original_w->viewport;
                if (vp != nullptr)
                {
                    left = std::max<int16_t>(left, vp->pos.x);
                    right = std::min<int16_t>(right, vp->pos.x + vp->width);
                    top = std::max<int16_t>(top, vp->pos.y);
                    bottom = std::min<int16_t>(bottom, vp->pos.y + vp->height);
                    if (left < right && top < bottom)
                    {
                        auto width = right - left;
                        auto height = bottom - top;
                        drawFunc(rt, weatherDrawer, left, top, width, height);
                    }
                }
                return;
            }

            w = it->get();

            if (w->flags.has(WindowFlag::dead))
            {
                continue;
            }

            if (right <= w->windowPos.x || bottom <= w->windowPos.y)
            {
                continue;
            }

            if (w->right() <= left || w->bottom() <= top)
            {
                continue;
            }

            if (left >= w->windowPos.x)
            {
                break;
            }

            DrawWeatherWindow(rt, weatherDrawer, original_w, left, w->windowPos.x, top, bottom, drawFunc);

            left = w->windowPos.x;
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, bottom, drawFunc);
            return;
        }

        auto wRight = w->right();
        if (right > wRight)
        {
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, wRight, top, bottom, drawFunc);

            left = wRight;
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, bottom, drawFunc);
            return;
        }

        if (top < w->windowPos.y)
        {
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, w->windowPos.y, drawFunc);

            top = w->windowPos.y;
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, bottom, drawFunc);
            return;
        }

        auto wBottom = w->bottom();
        if (bottom > wBottom)
        {
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, wBottom, drawFunc);

            top = wBottom;
            DrawWeatherWindow(rt, weatherDrawer, original_w, left, right, top, bottom, drawFunc);
            return;
        }
    }

    InputEvent GetInputEventFromSDLEvent(const SDL_Event& e)
    {
        InputEvent ie;
        ie.deviceKind = InputDeviceKind::keyboard;
        ie.modifiers = e.key.keysym.mod;
        ie.button = e.key.keysym.sym;

        // Handle dead keys
        if (ie.button == (SDLK_SCANCODE_MASK | 0))
        {
            switch (e.key.keysym.scancode)
            {
                case SDL_SCANCODE_APOSTROPHE:
                    ie.button = '\'';
                    break;
                case SDL_SCANCODE_GRAVE:
                    ie.button = '`';
                    break;
                default:
                    break;
            }
        }

        return ie;
    }

    void SetAudioVolume(float value)
    {
        auto& audioContext = GetContext()->GetAudioContext();
        auto* mixer = audioContext.GetMixer();
        if (mixer != nullptr)
        {
            mixer->SetVolume(value);
        }
    }
};

std::unique_ptr<IUiContext> Ui::CreateUiContext(IPlatformEnvironment& env)
{
    return std::make_unique<UiContext>(env);
}

InGameConsole& Ui::GetInGameConsole()
{
    auto& uiContext = static_cast<UiContext&>(GetContext()->GetUiContext());
    return uiContext.GetInGameConsole();
}

InputManager& Ui::GetInputManager()
{
    auto& uiContext = static_cast<UiContext&>(GetContext()->GetUiContext());
    return uiContext.GetInputManager();
}

ShortcutManager& Ui::GetShortcutManager()
{
    auto& uiContext = static_cast<UiContext&>(GetContext()->GetUiContext());
    return uiContext.GetShortcutManager();
}
