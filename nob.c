#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};
    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-Wall", "-Wextra", "-std=c23", "-o", "main", "main.c");

    cmd_append(&cmd, "-I/opt/homebrew/include", "-L/opt/homebrew/lib");

    cmd_append(&cmd, "-lraylib", "-framework", "CoreVideo", "-framework",
               "IOKit", "-framework", "Cocoa", "-framework", "GLUT",
               "-framework", "OpenGL");
    if (!cmd_run_sync(cmd))
        return 1;
    return 0;
}
