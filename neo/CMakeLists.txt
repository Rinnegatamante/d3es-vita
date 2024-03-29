project(d3wasm)

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# don't add these as options, but document them?
# IDNET_HOST		-DIDNET_HOST=\\"%s\\"' % IDNET_HOST
# DEBUG_MEMORY		-DID_DEBUG_MEMORY', '-DID_REDIRECT_NEWDELETE
# LIBC_MALLOC		-DUSE_LIBC_MALLOC=0
# ID_NOLANADDRESS	-DID_NOLANADDRESS

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/sys/cmake")

set(D3WASMBINARY "d3wasm")

include(CheckCXXCompilerFlag)
include(GNUInstallDirs OPTIONAL RESULT_VARIABLE GNUINSTALLDIRS)

option(CORE "Build the core" ON)
option(BASE "Build the base game code" ON)
option(NODLL "Build core and base game code, embedded inside one binary" ON)
option(NOMT "Do not use multithreading" ON)
option(WEBGL "We are in a WebGL environment" OFF)
option(NOEFX "Do not use OpenAL EFX" OFF)

# Enforce some options for Emscripten
if (EMSCRIPTEN)
  set(NOEFX ON)
  set(NOMT  ON)
  set(NODLL ON)
  set(WEBGL ON)
  set(CMAKE_EXECUTABLE_SUFFIX ".html") # use the HTML suffix (will emit .html, .js and .wasm)
else ()
  set(NOEFX ON)
  set(NOMT  ON)
  set(NODLL ON)
  set(WEBGL OFF)
endif ()

# Enable C++ 11, required by raw string literals usage to inline shaders in C++ code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if (WEBGL)
  add_compile_options("-DWEBGL")
endif ()

if (NOEFX)
  add_compile_options("-DNOEFX")
endif ()

if (NOMT)
  add_compile_options("-DNOMT")
endif ()

if (NODLL)
  set(BASE OFF)   # base game will be linked with core directly
  add_compile_options("-DNODLL")
endif ()

if (EMSCRIPTEN)
  add_compile_options(-fno-exceptions)
endif()

if (NOT CMAKE_SYSTEM_PROCESSOR)
  message(FATAL_ERROR "No target CPU architecture set")
endif ()

if (NOT CMAKE_SYSTEM_NAME)
  message(FATAL_ERROR "No target OS set")
endif ()

# target cpu
set(cpu ${CMAKE_SYSTEM_PROCESSOR})

# target os
string(TOLOWER "${CMAKE_SYSTEM_NAME}" os)

if (EMSCRIPTEN)
  set(os "emscripten")
  set(cpu "wasm")
endif ()

# build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif ()

# compiler specific flags
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang" OR EMSCRIPTEN)
  add_compile_options(-pipe)
  add_compile_options(-Wall)

  set(CMAKE_C_FLAGS_DEBUG "-g -D_DEBUG -O0")
  set(CMAKE_C_FLAGS_DEBUGALL "-g -D_DEBUG")
  set(CMAKE_C_FLAGS_PROFILE "-g --profiling-funcs -D_DEBUG -O1 -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fomit-frame-pointer")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O3 -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fomit-frame-pointer")

  string (REGEX MATCH "-O2" IS_O2_EXE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  if (IS_O2_EXE)
    string (REGEX REPLACE "-O2" "-O3" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  else()
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3")
  endif()

  set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
  set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})

  # LTO management (for Release and MinSizeRel only)
  set (ENABLE_LTO OFF CACHE BOOL "Enable LTO")
  if (ENABLE_LTO)
    if (EMSCRIPTEN)
      set(COMPILE_LTO_FLAGS "-s WASM_OBJECT_FILES=0 -flto")
      set(LINK_LTO_FLAGS    "-s WASM_OBJECT_FILES=0 --llvm-lto 1 -flto")
    else()
      set(COMPILE_LTO_FLAGS "-flto")
      set(LINK_LTO_FLAGS    "-flto")
    endif()
  else()
    unset(COMPILE_LTO_FLAGS)
    unset(LINK_LTO_FLAGS)
  endif()
  set(CMAKE_C_FLAGS_RELEASE             "${CMAKE_C_FLAGS_RELEASE} ${COMPILE_LTO_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE           "${CMAKE_CXX_FLAGS_RELEASE} ${COMPILE_LTO_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${LINK_LTO_FLAGS}")
  set(CMAKE_C_FLAGS_MINSIZEREL          "${CMAKE_C_FLAGS_MINSIZEREL} ${COMPILE_LTO_FLAGS}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL        "${CMAKE_CXX_FLAGS_MINSIZEREL} ${COMPILE_LTO_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} ${LINK_LTO_FLAGS}")

  add_compile_options(-fno-strict-aliasing)

  CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" cxx_has_fvisibility)
  if (NOT cxx_has_fvisibility)
    message(FATAL_ERROR "Compiler does not support -fvisibility")
  endif ()
  add_compile_options(-fvisibility=hidden)

  # TODO fix these warnings
  add_compile_options(-Wno-sign-compare)
  add_compile_options(-Wno-switch)
  add_compile_options(-Wno-format-security)
  if (EMSCRIPTEN)
    add_compile_options(-Wno-unused-variable)
    add_compile_options(-Wno-enum-compare)
    add_compile_options(-Wno-self-assign-field)
    add_compile_options(-Wno-unused-function)
    add_compile_options(-Wno-unused-const-variable)
    add_compile_options(-Wno-unused-private-field)
  else ()
    add_compile_options(-Wno-misleading-indentation)
  endif ()

  CHECK_CXX_COMPILER_FLAG("-Woverloaded-virtual" cxx_has_Woverload_virtual)
  if (cxx_has_Woverload_virtual)
    add_compile_options(-Woverloaded-virtual)
  endif ()
  if (os STREQUAL "linux")
    set(sys_libs ${sys_libs} dl)
  endif ()
else ()
  message(FATAL_ERROR "Unsupported compiler")
endif ()

# libs
if (NOT EMSCRIPTEN)
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIRS})

  find_package(OGG REQUIRED)
  include_directories(${OGG_INCLUDE_DIR})

  find_package(Vorbis REQUIRED)
  include_directories(${VORBIS_INCLUDE_DIR})

  find_package(VorbisFile REQUIRED)
  include_directories(${VORBISFILE_INCLUDE_DIR})

  set(SDL2_BUILDING_LIBRARY TRUE)
  find_package(SDL2 REQUIRED)
  include_directories(${SDL2_INCLUDE_DIR})
  set(SDLx_LIBRARY ${SDL2_LIBRARY})

  find_package(OpenAL REQUIRED)
  include_directories(${OPENAL_INCLUDE_DIR})

  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})

  find_package(JPEG REQUIRED)
  include_directories(${JPEG_INCLUDE_DIR})
  list(APPEND CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
  list(APPEND CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_VORBIS=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_OGG=1")      # Already done with USE_VORBIS=1
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_LIBJPEG=1")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -s USE_ZLIB=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1")

  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")    SIMD/Autovectorization: Not yet enabled in all browsers (only chrome for now)

  # fix for OpenAL include dir
  include_directories("${OPENAL_INCLUDE_DIR}/AL") # AL headers are not in a standard location
endif ()

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
# disable assertions in release builds
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG ${CMAKE_C_FLAGS_MINSIZEREL}")

if (EMSCRIPTEN)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_custom_target(package_demo_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_demo_data.bat data/demo)
    add_custom_target(package_full_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_full_data.bat data/base)
    add_custom_target(package_chunked_demo_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_chunked_demo_data.bat data/chunked)
  else()
    add_custom_target(package_demo_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_demo_data.sh data/demo)
    add_custom_target(package_full_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_full_data.sh data/base)
    add_custom_target(package_chunked_demo_data ${CMAKE_SOURCE_DIR}/sys/wasm/package_chunked_demo_data.sh data/chunked)
  endif()

  # demo data preload
  set(ldflags "${ldflags} --pre-js ../neo/sys/wasm/pre.js")
  # chunked demo data preload
  #set(ldflags "${ldflags} --pre-js ../neo/sys/wasm/pre-chunked.js")
  #set(ldflags "${ldflags} --post-js ../neo/sys/wasm/post-chunked.js")
  # full data preload
  #set(ldflags "${ldflags} --pre-js ../neo/sys/wasm/pre-full.js")

  set(ldflags "${ldflags} --shell-file ../neo/sys/wasm/shell.html")
  set(ldflags "${ldflags} -o d3wasm.html")
  set(ldflags "${ldflags} -s FORCE_FILESYSTEM=1")
  set(ldflags "${ldflags} -s TOTAL_MEMORY=402653184") # 384MB. Going below 320MB lead to crashes after a couple of levels played.
  set(ldflags "${ldflags} -s ASYNCIFY=1")
  set(ldflags "${ldflags} -s MINIFY_HTML=0")
  set(ldflags "${ldflags} -s ENVIRONMENT=web")

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    file(READ ../neo/sys/wasm/asyncify.win.json ASYNCIFY_WHITELIST)
    set(ldflags "${ldflags} -s \"ASYNCIFY_WHITELIST=${ASYNCIFY_WHITELIST}\"")
  else()
    file(READ ../neo/sys/wasm/asyncify.json ASYNCIFY_WHITELIST)
    set(ldflags "${ldflags} -s 'ASYNCIFY_WHITELIST=${ASYNCIFY_WHITELIST}'")
  endif()
  set(ldflags "${ldflags} -s EXIT_RUNTIME=0")   # Don't exit the runtime
  set(ldflags "${ldflags} -s ASSERTIONS=0")     # Disabling assertion slightly reduce binary size
  # D3 uses its own optimized memory allocator, build on top of malloc.
  # while dlmalloc might be a bit bigger in size than emmalloc, it is more efficient with the optimized memory allocator
  set(ldflags "${ldflags} -s MALLOC=dlmalloc")
  set(ldflags "${ldflags} -s ENVIRONMENT=web")
  set(ldflags "${ldflags} -s USE_SDL=2")
  set(ldflags "${ldflags} -s USE_ZLIB=1")
  set(ldflags "${ldflags} -s USE_VORBIS=1")
  set(ldflags "${ldflags} -s USE_OGG=1")      # Already done with USE_VORBIS=1
  set(ldflags "${ldflags} -s USE_LIBJPEG=1")
  set(ldflags "${ldflags} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0") # Because of SDL2 port still use the deprecated behavior
  #set(ldflags "${ldflags} -s STRICT=1") # can't enable this due to DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR that have to be 0
  #nevertheless be explicit on the libs to link to in strict mode
  set(ldflags "${ldflags} -lopenal.js") # OpenAL
  set(ldflags "${ldflags} -lwebgl.js")  # WebGL (NB: Already done with USE_SDL=2)
  set(ldflags "${ldflags} -legl.js")    # EGL (NB: Already done with USE_SDL=2)
  set(ldflags "${ldflags} -lfs.js")     # Filesystem core functionality
  set(ldflags "${ldflags} -lmemfs.js")  # in-memory filesystem
  set(ldflags "${ldflags} -lidbfs.js")  # in-memory filesystem + idb persistency (used in pre.js for "web_user" folder)
  set(ldflags "${ldflags} -lsockfs.js") # needed because of libc socket implementation (D3 use sockets)
endif ()

# fallback for cmake versions without GNUInstallDirs
if (GNUINSTALLDIRS MATCHES "NOTFOUND")
  set(CMAKE_INSTALL_BINDIR "bin"
      CACHE PATH "user executables (bin)")
  set(CMAKE_INSTALL_LIBDIR "lib${LIB_SUFFIX}"
      CACHE PATH "object code libraries (lib${LIB_SUFFIX})")
  set(CMAKE_INSTALL_DATAROOTDIR "share"
      CACHE PATH "read-only architecture-independent data root (share)")
  set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}"
      CACHE PATH "read-only architecture-independent data (DATAROOTDIR)")

  mark_as_advanced(CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATAROOTDIR CMAKE_INSTALL_DATADIR)
  foreach (dir BINDIR LIBDIR DATAROOTDIR DATADIR)
    if (NOT IS_ABSOLUTE ${CMAKE_INSTALL_${dir}})
      set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_${dir}}")
    else ()
      set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_${dir}}")
    endif ()
  endforeach ()
endif ()

if (EMSCRIPTEN)
  set(bindir "/usr/local/bin")
  set(libdir "/usr/local/lib/d3wasm")
  set(datadir "/usr/local/share/d3wasm")
else()
  set(bindir "${CMAKE_INSTALL_FULL_BINDIR}")
  set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}/d3wasm")
  set(datadir "${CMAKE_INSTALL_FULL_DATADIR}/d3wasm")
endif()


configure_file(
    "${CMAKE_SOURCE_DIR}/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
)

message(STATUS "Building ${CMAKE_BUILD_TYPE} for ${os}-${cpu}")

if (NOT EMSCRIPTEN)
  message(STATUS "The install target will use the following directories:")
  message(STATUS "  Binary directory:  ${bindir}")
  message(STATUS "  Library directory: ${libdir}")
  message(STATUS "  Data directory:    ${datadir}")
endif ()

set(src_renderer_glsl
    renderer/glsl/glsl_shaders.h
    renderer/glsl/cubeMapShaderFP.cpp
    renderer/glsl/diffuseMapShaderVP.cpp
    renderer/glsl/diffuseCubeShaderVP.cpp
    renderer/glsl/diffuseMapShaderFP.cpp
    renderer/glsl/fogShaderFP.cpp
    renderer/glsl/fogShaderVP.cpp
    renderer/glsl/blendLightShaderVP.cpp
    renderer/glsl/interactionPhongShaderFP.cpp
    renderer/glsl/interactionPhongShaderVP.cpp
    renderer/glsl/interactionShaderFP.cpp
    renderer/glsl/interactionShaderVP.cpp
    renderer/glsl/reflectionCubeShaderVP.cpp
    renderer/glsl/skyboxCubeShaderVP.cpp
    renderer/glsl/stencilShadowShaderFP.cpp
    renderer/glsl/stencilShadowShaderVP.cpp
    renderer/glsl/zfillClipShaderFP.cpp
    renderer/glsl/zfillClipShaderVP.cpp
    renderer/glsl/zfillShaderFP.cpp
    renderer/glsl/zfillShaderVP.cpp
)

set(src_renderer
    renderer/Cinematic.cpp
    renderer/GuiModel.cpp
    renderer/Image_files.cpp
    renderer/Image_init.cpp
    renderer/Image_load.cpp
    renderer/Image_process.cpp
    renderer/Image_program.cpp
    renderer/Interaction.cpp
    renderer/Material.cpp
    renderer/MegaTexture.cpp
    renderer/Model.cpp
    renderer/ModelDecal.cpp
    renderer/ModelManager.cpp
    renderer/ModelOverlay.cpp
    renderer/Model_beam.cpp
    renderer/Model_ase.cpp
    renderer/Model_liquid.cpp
    renderer/Model_lwo.cpp
    renderer/Model_ma.cpp
    renderer/Model_md3.cpp
    renderer/Model_md5.cpp
    renderer/Model_prt.cpp
    renderer/Model_sprite.cpp
    renderer/RenderEntity.cpp
    renderer/RenderSystem.cpp
    renderer/RenderSystem_init.cpp
    renderer/RenderWorld.cpp
    renderer/RenderWorld_demo.cpp
    renderer/RenderWorld_load.cpp
    renderer/RenderWorld_portals.cpp
    renderer/VertexCache.cpp
    renderer/draw_gles2.cpp
    renderer/draw_common.cpp
    renderer/tr_backend.cpp
    renderer/tr_deform.cpp
    renderer/tr_font.cpp
    renderer/tr_guisurf.cpp
    renderer/tr_light.cpp
    renderer/tr_lightrun.cpp
    renderer/tr_main.cpp
    renderer/tr_orderIndexes.cpp
    renderer/tr_polytope.cpp
    renderer/tr_render.cpp
    renderer/tr_shadowbounds.cpp
    renderer/tr_stencilshadow.cpp
    renderer/tr_subview.cpp
    renderer/tr_trace.cpp
    renderer/tr_trisurf.cpp
    renderer/tr_turboshadow.cpp
    renderer/etc_android.cpp
    renderer/framebuffer.cpp
    ${src_renderer_glsl}
    )

# globs all the headers from ${PATHPREFIX}/ and adds them to ${SRCLIST}
function(add_globbed_headers SRCLIST PATHPREFIX)
  file(GLOB_RECURSE tmp_hdrs RELATIVE "${CMAKE_SOURCE_DIR}" "${PATHPREFIX}/*.h")
  set(${SRCLIST} ${tmp_hdrs} ${${SRCLIST}} PARENT_SCOPE)
endfunction()

add_globbed_headers(src_renderer "renderer")

set(src_framework
    framework/CVarSystem.cpp
    framework/CmdSystem.cpp
    framework/Common.cpp
    framework/Compressor.cpp
    framework/Console.cpp
    framework/DemoFile.cpp
    framework/DeclAF.cpp
    framework/DeclEntityDef.cpp
    framework/DeclFX.cpp
    framework/DeclManager.cpp
    framework/DeclParticle.cpp
    framework/DeclPDA.cpp
    framework/DeclSkin.cpp
    framework/DeclTable.cpp
    framework/EditField.cpp
    framework/EventLoop.cpp
    framework/File.cpp
    framework/FileSystem.cpp
    framework/KeyInput.cpp
    framework/UsercmdGen.cpp
    framework/Session_menu.cpp
    framework/Session.cpp
    framework/async/AsyncClient.cpp
    framework/async/AsyncNetwork.cpp
    framework/async/AsyncServer.cpp
    framework/async/MsgChannel.cpp
    framework/async/NetworkSystem.cpp
    framework/async/ServerScan.cpp
    framework/minizip/ioapi.c
    framework/minizip/unzip.cpp
    )

add_globbed_headers(src_framework "framework")

set(src_cm
    cm/CollisionModel_contacts.cpp
    cm/CollisionModel_contents.cpp
    cm/CollisionModel_debug.cpp
    cm/CollisionModel_files.cpp
    cm/CollisionModel_load.cpp
    cm/CollisionModel_rotate.cpp
    cm/CollisionModel_trace.cpp
    cm/CollisionModel_translate.cpp
    )

add_globbed_headers(src_cm "cm")

set(src_aas
    tools/compilers/aas/AASBuild.cpp
    tools/compilers/aas/AASBuild_file.cpp
    tools/compilers/aas/AASBuild_gravity.cpp
    tools/compilers/aas/AASBuild_ledge.cpp
    tools/compilers/aas/AASBuild_merge.cpp
    tools/compilers/aas/AASCluster.cpp
    tools/compilers/aas/AASFile.cpp
    tools/compilers/aas/AASFile_optimize.cpp
    tools/compilers/aas/AASFile_sample.cpp
    tools/compilers/aas/AASReach.cpp
    tools/compilers/aas/AASFileManager.cpp
    tools/compilers/aas/Brush.cpp
    tools/compilers/aas/BrushBSP.cpp
    )

add_globbed_headers(src_aas "tools/compilers/aas")

set(src_snd
    sound/snd_cache.cpp
    sound/snd_decoder.cpp
    sound/snd_efxfile.cpp
    sound/snd_emitter.cpp
    sound/snd_shader.cpp
    sound/snd_system.cpp
    sound/snd_wavefile.cpp
    sound/snd_world.cpp
    )

add_globbed_headers(src_snd "sound")

set(src_ui
    ui/BindWindow.cpp
    ui/ChoiceWindow.cpp
    ui/DeviceContext.cpp
    ui/EditWindow.cpp
    ui/FieldWindow.cpp
    ui/GameBearShootWindow.cpp
    ui/GameBustOutWindow.cpp
    ui/GameSSDWindow.cpp
    ui/GuiScript.cpp
    ui/ListGUI.cpp
    ui/ListWindow.cpp
    ui/MarkerWindow.cpp
    ui/RegExp.cpp
    ui/RenderWindow.cpp
    ui/SimpleWindow.cpp
    ui/SliderWindow.cpp
    ui/UserInterface.cpp
    ui/Window.cpp
    ui/Winvar.cpp
    )

add_globbed_headers(src_ui "ui")

set(src_idlib
    idlib/bv/Bounds.cpp
    idlib/bv/Frustum.cpp
    idlib/bv/Sphere.cpp
    idlib/bv/Box.cpp
    idlib/geometry/DrawVert.cpp
    idlib/geometry/Winding2D.cpp
    idlib/geometry/Surface_SweptSpline.cpp
    idlib/geometry/Winding.cpp
    idlib/geometry/Surface.cpp
    idlib/geometry/Surface_Patch.cpp
    idlib/geometry/TraceModel.cpp
    idlib/geometry/JointTransform.cpp
    idlib/hashing/CRC32.cpp
    idlib/hashing/MD4.cpp
    idlib/hashing/MD5.cpp
    idlib/math/Angles.cpp
    idlib/math/Lcp.cpp
    idlib/math/Math.cpp
    idlib/math/Matrix.cpp
    idlib/math/Ode.cpp
    idlib/math/Plane.cpp
    idlib/math/Pluecker.cpp
    idlib/math/Polynomial.cpp
    idlib/math/Quat.cpp
    idlib/math/Rotation.cpp
    idlib/math/Simd.cpp
    idlib/math/Simd_Generic.cpp
    idlib/math/Simd_MMX.cpp
    idlib/math/Simd_SSE.cpp
    idlib/math/Simd_SSE2.cpp
    idlib/math/Simd_SSE3.cpp
    idlib/math/Vector.cpp
    idlib/BitMsg.cpp
    idlib/LangDict.cpp
    idlib/Lexer.cpp
    idlib/Lib.cpp
    idlib/containers/HashIndex.cpp
    idlib/Dict.cpp
    idlib/Str.cpp
    idlib/Parser.cpp
    idlib/MapFile.cpp
    idlib/CmdArgs.cpp
    idlib/Token.cpp
    idlib/Base64.cpp
    idlib/Timer.cpp
    idlib/Heap.cpp
    )

add_globbed_headers(src_idlib "idlib")

set(src_game
    game/AF.cpp
    game/AFEntity.cpp
    game/Actor.cpp
    game/Camera.cpp
    game/Entity.cpp
    game/BrittleFracture.cpp
    game/Fx.cpp
    game/GameEdit.cpp
    game/Game_local.cpp
    game/Game_network.cpp
    game/Item.cpp
    game/IK.cpp
    game/Light.cpp
    game/Misc.cpp
    game/Mover.cpp
    game/Moveable.cpp
    game/MultiplayerGame.cpp
    game/Player.cpp
    game/PlayerIcon.cpp
    game/PlayerView.cpp
    game/Projectile.cpp
    game/Pvs.cpp
    game/SecurityCamera.cpp
    game/SmokeParticles.cpp
    game/Sound.cpp
    game/Target.cpp
    game/Trigger.cpp
    game/Weapon.cpp
    game/WorldSpawn.cpp
    game/ai/AAS.cpp
    game/ai/AAS_debug.cpp
    game/ai/AAS_pathing.cpp
    game/ai/AAS_routing.cpp
    game/ai/AI.cpp
    game/ai/AI_events.cpp
    game/ai/AI_pathing.cpp
    game/ai/AI_Vagary.cpp
    game/gamesys/DebugGraph.cpp
    game/gamesys/Class.cpp
    game/gamesys/Event.cpp
    game/gamesys/SaveGame.cpp
    game/gamesys/SysCmds.cpp
    game/gamesys/SysCvar.cpp
    game/gamesys/TypeInfo.cpp
    game/anim/Anim.cpp
    game/anim/Anim_Blend.cpp
    game/anim/Anim_Import.cpp
    game/anim/Anim_Testmodel.cpp
    game/script/Script_Compiler.cpp
    game/script/Script_Interpreter.cpp
    game/script/Script_Program.cpp
    game/script/Script_Thread.cpp
    game/physics/Clip.cpp
    game/physics/Force.cpp
    game/physics/Force_Constant.cpp
    game/physics/Force_Drag.cpp
    game/physics/Force_Field.cpp
    game/physics/Force_Spring.cpp
    game/physics/Physics.cpp
    game/physics/Physics_AF.cpp
    game/physics/Physics_Actor.cpp
    game/physics/Physics_Base.cpp
    game/physics/Physics_Monster.cpp
    game/physics/Physics_Parametric.cpp
    game/physics/Physics_Player.cpp
    game/physics/Physics_RigidBody.cpp
    game/physics/Physics_Static.cpp
    game/physics/Physics_StaticMulti.cpp
    game/physics/Push.cpp
    )

add_globbed_headers(src_game "game")

set(src_core
    ${src_renderer}
    ${src_framework}
    ${src_cm}
    ${src_aas}
    ${src_snd}
    ${src_ui}
    )

set(src_sys_base
    sys/cpu.cpp
    sys/threads.cpp
    sys/events.cpp
    sys/sys_local.cpp
    sys/posix/posix_net.cpp
    sys/posix/posix_main.cpp
    sys/linux/main.cpp
    )

set(src_sys_core
    sys/glimp.cpp
    )

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

add_library(idlib STATIC ${src_idlib})

# fPIC have issues with latest Emscripten LLVM backend
if (NOT EMSCRIPTEN)
  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_target_properties(idlib PROPERTIES COMPILE_FLAGS "-fPIC")
  endif ()
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib})

if (CORE)
  if (NODLL)
    add_executable(${D3WASMBINARY}
        ${src_core}
        ${src_sys_base}
        ${src_sys_core}
        ${src_game}         # base game is added to core
        )

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_sys_core} ${src_game})

    set_target_properties(${D3WASMBINARY} PROPERTIES COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/game")
  else ()
    add_executable(${D3WASMBINARY}
        ${src_core}
        ${src_sys_base}
        ${src_sys_core}
        )

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_sys_core})

    set_target_properties(${D3WASMBINARY} PROPERTIES COMPILE_DEFINITIONS "__DOOM_DLL__")
  endif ()

  set_target_properties(${D3WASMBINARY} PROPERTIES LINK_FLAGS "${ldflags}")
  target_link_libraries(${D3WASMBINARY}
      idlib
      ${OPENAL_LIBRARY}
      ${VORBISFILE_LIBRARIES}
      ${VORBIS_LIBRARIES}
      ${OGG_LIBRARIES}
      ${JPEG_LIBRARY}
      ${ZLIB_LIBRARY}
      ${SDLx_LIBRARY}
      ${sys_libs})

  if (NOT EMSCRIPTEN)
    install(TARGETS ${D3WASMBINARY}
        RUNTIME DESTINATION "${bindir}"
        LIBRARY DESTINATION "${libdir}"
        ARCHIVE DESTINATION "${libdir}"
        )
  endif ()
endif ()

if (BASE)
  add_library(base SHARED ${src_game})

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_game})

  set_target_properties(base PROPERTIES PREFIX "")
  set_target_properties(base PROPERTIES COMPILE_DEFINITIONS "GAME_DLL")
  set_target_properties(base PROPERTIES COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/game")
  set_target_properties(base PROPERTIES LINK_FLAGS "${ldflags}")
  set_target_properties(base PROPERTIES INSTALL_NAME_DIR "@executable_path")
  target_link_libraries(base idlib)

  install(TARGETS base
      RUNTIME DESTINATION "${bindir}"
      LIBRARY DESTINATION "${libdir}"
      ARCHIVE DESTINATION "${libdir}"
      )

endif ()

