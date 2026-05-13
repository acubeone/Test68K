const std = @import("std");
const Translator = @import("translate_c").Translator;

const ResolvedTarget = std.Build.ResolvedTarget;
const OptimizeMode = std.builtin.OptimizeMode;
const Compile = std.Build.Step.Compile;

fn _link_moira(b: *std.Build, target: ResolvedTarget, optimize: OptimizeMode) *Compile {
    const moira = b.addLibrary(.{
        .name = "moirawrapper",
        .root_module = b.createModule(.{
            .link_libc = true,
            .link_libcpp = true,
            .target = target,
            .optimize = optimize,
        }),
        .linkage = .static,
        .use_lld = true,
        .use_llvm = true,
    });
    moira.root_module.addIncludePath(b.path("vendor/Moira"));
    moira.root_module.addIncludePath(b.path("src/moira"));
    moira.root_module.addCSourceFiles(.{
        .files = &.{
            "vendor/Moira/Moira.cpp",
            "vendor/Moira/MoiraDebugger.cpp",
            "src/moira/wrapper.cpp",
        },
        .flags = &.{ "-std=c++20", "-O2", "-gen-cdb-fragment-path", ".cache/cdb" },
        .language = .cpp,
    });
    moira.installHeadersDirectory(b.path("vendor/Moira"), "", .{});
    b.installArtifact(moira);

    // Generate compile_commands.json since Zig can't do it for some reason
    const cdb = b.addSystemCommand(&.{
        "sh", "-c",
        \\set -eu
        \\mkdir -p .cache/cdb
        \\{
        \\  printf '['
        \\  first=1
        \\  find .cache/cdb -type f | sort | while IFS= read -r f; do
        \\    [ -f "$f" ] || continue
        \\    if [ $first -eq 0 ]; then printf ','; fi
        \\    first=0
        \\    cat "$f"
        \\  done
        \\  printf ']'
        \\} > compile_commands.json
    });
    cdb.step.dependOn(&moira.step);

    return moira;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const moira = _link_moira(b, target, optimize);

    const wrapper = b.addTranslateC(.{
        .root_source_file = b.path("src/moira/wrapper.h"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });

    const exe = b.addExecutable(.{
        .name = "test68k",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .link_libc = true,
            .target = target,
            .optimize = optimize,
            .imports = &.{.{
                .name = "moira_wrapper",
                .module = wrapper.createModule(),
            }},
        }),
        .use_lld = true,
        .use_llvm = true,
    });
    exe.root_module.linkLibrary(moira);
    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);
    if (b.args) |args|
        run_exe.addArgs(args);

    _ = b.step("run", "Run generator").dependOn(&run_exe.step);
}
