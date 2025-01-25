#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "self_build/self_build.h"
#include "self_build/self_build.c"

#include "stdlib/win32_platform.c"
#include "stdlib/strings.c"
#include "stdlib/allocators.c"
#include "stdlib/arena.c"
#include "stdlib/thread_context.c"
#include "stdlib/managed_arena.c"
#include "stdlib/scratch_memory.c"
#include "stdlib/string_builder.c"

extern struct Build __declspec(dllexport) build(struct Build_Context *, enum Build_Kind);

struct Build build(struct Build_Context *context, enum Build_Kind kind) {

    static char *files[] = {
        "src/rcore.c",
        "src/utils.c",
        "src/rshapes.c",
        "src/rtextures.c",
        "src/rtext.c",
        "src/rmodels.c",
        "src/raudio.c",
        "src/rglfw.c",
    };

    // @TODO: It would be nice to be able to "install" individual headers by copying them to the build directory.
    // And maybe dependent modules can automatically include the build directory of their dependencies
    static char *includes[] = {
        "src",
        "src/external/glfw/include",
        "src/platforms",
    };

    static char *compile_flags[] = {
        "-DPLATFORM_DESKTOP_GLFW",
        "-DSUPPORT_MODULE_RSHAPES",
        "-DSUPPORT_MODULE_RTEXTURES",
        "-DSUPPORT_MODULE_RTEXT",
        "-DSUPPORT_MODULE_RMODELS",
        "-DSUPPORT_MODULE_RAUDIO",
        "-DGRAPHICS_API_OPENGL_33",
    };

    static char **link_flags       = { 0 };
    static size_t link_flags_count = 0;

    if (kind == Build_Kind_Shared_Library) {
        link_flags_count = 3;
        link_flags = calloc(3, sizeof(char *));
        link_flags[0] = strdup("-lwinmm");
        link_flags[1] = strdup("-lgdi32");
        link_flags[2] = strdup("-lopengl32");
    }

    static struct Build lib = {
        .name = "raylib",

        .sources        = files,
        .sources_count  = sizeof(files) / sizeof(char *),

        .compile_flags       = compile_flags,
        .compile_flags_count = sizeof(compile_flags) / sizeof(char *),

        .includes       = includes,
        .includes_count = sizeof(includes) / sizeof(char *),
    };

    lib.kind = kind;
    lib.link_flags = link_flags;
    lib.link_flags_count = link_flags_count;

    return lib;
}

int main(void) {
    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    struct Allocator scratch = scratch_begin();

    char *artifacts_directory = "bin";
    char *self_build_path     = "selfbuild";

    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);
    char *cwd = win32_get_current_directory(&scratch);
    
    bootstrap("build.c", "build.exe", "bin/build.old", self_build_path);

    struct Build_Context context = {
        .self_build_path     = self_build_path,
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,
    };

    struct Build module = build(&context, Build_Kind_Static_Library);
    module.root_dir = ".";
    build_module(&context, &module);

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
