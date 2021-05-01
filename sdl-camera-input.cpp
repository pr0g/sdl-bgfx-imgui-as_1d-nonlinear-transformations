#include "sdl-camera-input.h"

#include <SDL.h>

asci::InputEvent sdlToInput(const SDL_Event* event)
{
  switch (event->type) {
    case SDL_MOUSEMOTION: {
      const auto* mouse_motion_event = (SDL_MouseMotionEvent*)event;
      return asci::CursorMotionEvent{
        {mouse_motion_event->x, mouse_motion_event->y}};
    }
    case SDL_MOUSEWHEEL: {
      const auto* mouse_wheel_event = (SDL_MouseWheelEvent*)event;
      return asci::ScrollEvent{mouse_wheel_event->y};
    }
    case SDL_MOUSEBUTTONDOWN: {
      const auto* mouse_event = (SDL_MouseButtonEvent*)event;
      return asci::MouseButtonEvent{
        mouseFromSdl(mouse_event), asci::ButtonAction::Down};
    }
    case SDL_MOUSEBUTTONUP: {
      const auto* mouse_event = (SDL_MouseButtonEvent*)event;
      return asci::MouseButtonEvent{
        mouseFromSdl(mouse_event), asci::ButtonAction::Up};
    }
    case SDL_KEYDOWN: {
      const auto* keyboard_event = (SDL_KeyboardEvent*)event;
      return asci::KeyboardButtonEvent{
        keyboardFromSdl(keyboard_event->keysym.scancode),
        asci::ButtonAction::Down, event->key.repeat != 0u};
    }
    case SDL_KEYUP: {
      const auto* keyboard_event = (SDL_KeyboardEvent*)event;
      return asci::KeyboardButtonEvent{
        keyboardFromSdl(keyboard_event->keysym.scancode),
        asci::ButtonAction::Up, event->key.repeat != 0u};
    }
    default:
      return std::monostate{};
  }
}

asci::MouseButton mouseFromSdl(const SDL_MouseButtonEvent* event)
{
  switch (event->button) {
    case SDL_BUTTON_LEFT:
      return asci::MouseButton::Left;
    case SDL_BUTTON_RIGHT:
      return asci::MouseButton::Right;
    case SDL_BUTTON_MIDDLE:
      return asci::MouseButton::Middle;
    default:
      return asci::MouseButton::Nil;
  }
}

asci::KeyboardButton keyboardFromSdl(const int key)
{
  switch (key) {
    case SDL_SCANCODE_W:
      return asci::KeyboardButton::W;
    case SDL_SCANCODE_S:
      return asci::KeyboardButton::S;
    case SDL_SCANCODE_A:
      return asci::KeyboardButton::A;
    case SDL_SCANCODE_D:
      return asci::KeyboardButton::D;
    case SDL_SCANCODE_Q:
      return asci::KeyboardButton::Q;
    case SDL_SCANCODE_E:
      return asci::KeyboardButton::E;
    case SDL_SCANCODE_LALT:
      return asci::KeyboardButton::LAlt;
    case SDL_SCANCODE_LSHIFT:
      return asci::KeyboardButton::LShift;
    case SDL_SCANCODE_LCTRL:
      return asci::KeyboardButton::Ctrl;
    default:
      return asci::KeyboardButton::Nil;
  }
}
