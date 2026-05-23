const std = @import("std");
const Cpu = @import("Cpu.zig").Cpu;

const MAX_RAM_SIZE = 1 << 22;

pub fn main() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();

    var cpu = Cpu.create(arena, MAX_RAM_SIZE);
    defer cpu.destroy(arena);
}
