cmake_minimum_required(VERSION 3.20)

project(nqwcl C)

include(CheckIPOSupported)
include(CheckCCompilerFlag)

check_ipo_supported(RESULT USE_LTO)
if(USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
add_compile_options(
        /nologo
        /W2
        /WX
        /diagnostics:column
)
else()
    add_compile_options(
            -std=gnu89

            -Wall
            -Wno-pointer-to-int-cast
            -Wno-int-to-pointer-cast
            -Wno-strict-aliasing
            -Wno-deprecated-declarations

            -fvisibility=hidden

            -Werror=format
            -Werror=strict-prototypes
            -Werror=old-style-definition
            -Werror=incompatible-pointer-types

            $<$<CONFIG:Release>:-Werror=unused-function>
            $<$<CONFIG:Release>:-Werror=unused-variable>
            $<$<CONFIG:Release>:-Werror=unused-but-set-variable>
    )

    check_c_compiler_flag("-Wstrlcpy-strlcat-size" HAS_STRLCPY_STRLCAT_SIZE)
    if(HAS_STRLCPY_STRLCAT_SIZE)
        add_compile_options("-Werror=strlcpy-strlcat-size")
    endif()
    check_c_compiler_flag("-Wimplicit-function-declaration" HAS_IMPLICIT_FUNCTION_DECLARATION)
    if(HAS_IMPLICIT_FUNCTION_DECLARATION)
        add_compile_options("-Werror=implicit-function-declaration")
    endif()

endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT $<$<CONFIG:Debug>:ProgramDatabase>)

set(CMAKE_VS_GLOBALS
        "UseMultiToolTask=true"
        "EnforceProcessCountAcrossBuilds=true"
)

set(SOURCES
        "src/cd_null.c"
        "src/cl_cam.c"
        "src/cl_demo.c"
        "src/cl_ents.c"
        "src/cl_input.c"
        "src/cl_main.c"
        "src/cl_parse.c"
        "src/cl_pred.c"
        "src/cl_tent.c"
        "src/cmd.c"
        "src/common.c"
        "src/console.c"
        "src/crc.c"
        "src/cvar.c"
        "src/gl_draw.c"
        "src/gl_mesh.c"
        "src/gl_model.c"
        "src/gl_ngraph.c"
        "src/gl_refrag.c"
        "src/gl_rlight.c"
        "src/gl_rmain.c"
        "src/gl_rmisc.c"
        "src/gl_rsurf.c"
        "src/gl_screen.c"
        "src/gl_vidsdl.c"
        "src/gl_warp.c"
        "src/in_sdl.c"
        "src/keys.c"
        "src/mathlib.c"
        "src/md4.c"
        "src/menu.c"
        "src/net_chan.c"
        "src/net_udp.c"
        "src/pmove.c"
        "src/pmovetst.c"
        "src/r_part.c"
        "src/sbar.c"
        "src/skin.c"
        "src/snd_dma.c"
        "src/snd_mem.c"
        "src/snd_mix.c"
        "src/snd_sdl.c"
        "src/sys_sdl.c"
        "src/view.c"
        "src/wad.c"
        "src/zone.c"
)

set(HEADERS
        "src/anorms.h"
        "src/anorm_dots.h"
        "src/bothdefs.h"
        "src/bspfile.h"
        "src/client.h"
        "src/cmd.h"
        "src/common.h"
        "src/console.h"
        "src/crc.h"
        "src/cvar.h"
        "src/draw.h"
        "src/glquake.h"
        "src/glquake2.h"
        "src/gl_model.h"
        "src/gl_warp_sin.h"
        "src/input.h"
        "src/keys.h"
        "src/mathlib.h"
        "src/menu.h"
        "src/modelgen.h"
        "src/net.h"
        "src/pmove.h"
        "src/protocol.h"
        "src/quakedef.h"
        "src/render.h"
        "src/resource.h"
        "src/r_local.h"
        "src/r_shared.h"
        "src/sbar.h"
        "src/screen.h"
        "src/sound.h"
        "src/spritegn.h"
        "src/sys.h"
        "src/vid.h"
        "src/view.h"
        "src/wad.h"
        "src/winquake.h"
        "src/zone.h"
)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

find_package(SDL3 REQUIRED)

include_directories("src")
add_definitions(-DGLQUAKE)

add_executable(nqwcl WIN32 ${SOURCES} ${HEADERS})
target_link_libraries(nqwcl
        $<$<PLATFORM_ID:Windows>:wsock32>
        $<$<PLATFORM_ID:Windows>:ws2_32>

        OpenGL::GL
        Threads::Threads
        SDL3::SDL3-static
)
