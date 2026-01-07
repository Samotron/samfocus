const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // ============================================================
    // Fetch cli dependency
    // ============================================================

    const cli_dep = b.dependency("cli", .{
        .target = target,
        .optimize = optimize,
    });

    // ============================================================
    // cimgui Static Library (C++)
    // ============================================================

    const cimgui = b.addStaticLibrary(.{
        .name = "cimgui",
        .target = target,
        .optimize = optimize,
    });

    cimgui.addCSourceFiles(.{
        .files = &.{
            "external/cimgui/cimgui.cpp",
            "external/cimgui/imgui/imgui.cpp",
            "external/cimgui/imgui/imgui_draw.cpp",
            "external/cimgui/imgui/imgui_tables.cpp",
            "external/cimgui/imgui/imgui_widgets.cpp",
            "external/cimgui/imgui/imgui_demo.cpp",
            "external/cimgui/imgui/backends/imgui_impl_glfw.cpp",
            "external/cimgui/imgui/backends/imgui_impl_opengl3.cpp",
        },
        .flags = &.{"-DIMGUI_IMPL_API=extern \"C\""},
    });

    cimgui.addIncludePath(b.path("external/cimgui"));
    cimgui.addIncludePath(b.path("external/cimgui/imgui"));
    cimgui.linkLibC();
    cimgui.linkLibCpp();
    cimgui.linkSystemLibrary("glfw3");

    if (target.result.os.tag == .linux) {
        cimgui.linkSystemLibrary("GL");
        cimgui.linkSystemLibrary("X11");
    } else if (target.result.os.tag == .windows) {
        cimgui.linkSystemLibrary("opengl32");
    }

    // ============================================================
    // Main GUI Application
    // ============================================================

    const samfocus = b.addExecutable(.{
        .name = "samfocus",
        .target = target,
        .optimize = optimize,
    });

    samfocus.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/core/platform.c",
            "src/core/task.c",
            "src/core/project.c",
            "src/core/context.c",
            "src/core/undo.c",
            "src/core/export.c",
            "src/core/preferences.c",
            "src/db/database.c",
            "src/ui/inbox_view.c",
            "src/ui/sidebar.c",
            "src/ui/help_overlay.c",
            "src/ui/command_palette.c",
            "src/ui/markdown.c",
            "src/ui/launcher.c",
        },
        .flags = &.{ "-std=c11", "-Wall", "-Wextra" },
    });

    samfocus.addIncludePath(b.path("src"));
    samfocus.addIncludePath(b.path("external/cimgui"));
    samfocus.addIncludePath(b.path("external/cimgui/imgui"));
    samfocus.linkLibrary(cimgui);
    samfocus.linkLibC();
    samfocus.linkSystemLibrary("sqlite3");

    // Platform-specific configuration
    if (target.result.os.tag == .linux) {
        samfocus.root_module.addCMacro("PLATFORM_LINUX", "1");
        samfocus.linkSystemLibrary("dl");
        samfocus.linkSystemLibrary("pthread");
        samfocus.linkSystemLibrary("X11");
    } else if (target.result.os.tag == .windows) {
        samfocus.root_module.addCMacro("PLATFORM_WINDOWS", "1");
        samfocus.root_module.addCMacro("_CRT_SECURE_NO_WARNINGS", "1");
        samfocus.linkSystemLibrary("shell32");
        samfocus.linkSystemLibrary("user32");
    }

    b.installArtifact(samfocus);

    // ============================================================
    // CLI Tool (with cli.h library)
    // ============================================================

    const samfocus_cli = b.addExecutable(.{
        .name = "samfocus-cli",
        .target = target,
        .optimize = optimize,
    });

    samfocus_cli.addCSourceFiles(.{
        .files = &.{
            "src/cli/samfocus-cli.c",
            "src/db/database.c",
            "src/core/task.c",
            "src/core/project.c",
            "src/core/context.c",
            "src/core/platform.c",
        },
        .flags = &.{ "-std=gnu11", "-Wall", "-Wextra", "-D_POSIX_C_SOURCE=200809L" },
    });

    samfocus_cli.addIncludePath(b.path("src"));
    samfocus_cli.addIncludePath(cli_dep.path(".")); // cli.h include path
    samfocus_cli.linkLibC();
    samfocus_cli.linkSystemLibrary("sqlite3");

    // Platform-specific configuration
    if (target.result.os.tag == .linux) {
        samfocus_cli.root_module.addCMacro("PLATFORM_LINUX", "1");
    } else if (target.result.os.tag == .windows) {
        samfocus_cli.root_module.addCMacro("PLATFORM_WINDOWS", "1");
        samfocus_cli.root_module.addCMacro("_CRT_SECURE_NO_WARNINGS", "1");
        samfocus_cli.linkSystemLibrary("shell32");
        samfocus_cli.linkSystemLibrary("user32");
    }

    b.installArtifact(samfocus_cli);

    // ============================================================
    // Tests
    // ============================================================

    // Unit tests
    const test_database = b.addExecutable(.{
        .name = "test_database",
        .target = target,
        .optimize = optimize,
    });

    test_database.addCSourceFiles(.{
        .files = &.{
            "tests/unit/test_database.c",
            "src/db/database.c",
            "src/core/task.c",
            "src/core/project.c",
            "src/core/context.c",
        },
        .flags = &.{"-std=c11"},
    });

    test_database.addIncludePath(b.path("src"));
    test_database.addIncludePath(b.path("tests"));
    test_database.linkLibC();
    test_database.linkSystemLibrary("sqlite3");

    if (target.result.os.tag == .linux) {
        test_database.root_module.addCMacro("PLATFORM_LINUX", "1");
    } else if (target.result.os.tag == .windows) {
        test_database.root_module.addCMacro("PLATFORM_WINDOWS", "1");
        test_database.root_module.addCMacro("_CRT_SECURE_NO_WARNINGS", "1");
    }

    // Integration tests
    const test_workflows = b.addExecutable(.{
        .name = "test_workflows",
        .target = target,
        .optimize = optimize,
    });

    test_workflows.addCSourceFiles(.{
        .files = &.{
            "tests/integration/test_workflows.c",
            "src/db/database.c",
            "src/core/task.c",
            "src/core/project.c",
            "src/core/context.c",
        },
        .flags = &.{"-std=c11"},
    });

    test_workflows.addIncludePath(b.path("src"));
    test_workflows.addIncludePath(b.path("tests"));
    test_workflows.linkLibC();
    test_workflows.linkSystemLibrary("sqlite3");

    if (target.result.os.tag == .linux) {
        test_workflows.root_module.addCMacro("PLATFORM_LINUX", "1");
    } else if (target.result.os.tag == .windows) {
        test_workflows.root_module.addCMacro("PLATFORM_WINDOWS", "1");
        test_workflows.root_module.addCMacro("_CRT_SECURE_NO_WARNINGS", "1");
    }

    // Register test steps
    const run_test_database = b.addRunArtifact(test_database);
    const run_test_workflows = b.addRunArtifact(test_workflows);

    const test_step = b.step("test", "Run all tests");
    test_step.dependOn(&run_test_database.step);
    test_step.dependOn(&run_test_workflows.step);

    // Individual test steps
    const test_db_step = b.step("test-db", "Run database unit tests");
    test_db_step.dependOn(&run_test_database.step);

    const test_wf_step = b.step("test-workflows", "Run integration tests");
    test_wf_step.dependOn(&run_test_workflows.step);

    // ============================================================
    // Run steps
    // ============================================================

    const run_samfocus = b.addRunArtifact(samfocus);
    if (b.args) |args| {
        run_samfocus.addArgs(args);
    }

    const run_cli = b.addRunArtifact(samfocus_cli);
    if (b.args) |args| {
        run_cli.addArgs(args);
    }

    const run_step = b.step("run", "Run samfocus GUI");
    run_step.dependOn(&run_samfocus.step);

    const run_cli_step = b.step("run-cli", "Run samfocus CLI");
    run_cli_step.dependOn(&run_cli.step);
}
