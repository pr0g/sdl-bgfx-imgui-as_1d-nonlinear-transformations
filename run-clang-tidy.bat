@echo off

fd -0 -e cpp -a -E "bgfx-imgui/" -E "sdl-imgui" | xargs -0 clang-tidy -p build\debug -header-filter="smooth*|debug*|curve*|file*|fps*|noise" --fix
