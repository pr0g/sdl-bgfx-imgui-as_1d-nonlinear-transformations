@echo off

fd -0 -e h -e cpp -a -E "bgfx-imgui/" -E "sdl-imgui" | xargs -0 clang-format -i
