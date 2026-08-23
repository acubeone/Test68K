const std = @import("std");
const moira = @import("moira.zig");

const MAX_RAM_SIZE = 1 << 22;

const STACK_ADDR = 0x0003_fff0;
const PROG_ADDR = 0x0001_0000;

const VECTOR_TABLE: [1024]u8 align(4) = blk: {
    var buf: [1024]u8 = undefined;
    std.mem.writeInt(u32, buf[0..4], STACK_ADDR, .big);
    std.mem.writeInt(u32, buf[4..8], PROG_ADDR, .big);

    for (2..256) |vec| {
        const addr = 0x0000_0800 + (@as(u32, @intCast(vec)) * 0x10);
        const s = vec * 4;
        const e = (vec * 4 + 4);
        std.mem.writeInt(u32, buf[s..e], addr, .big);
    }

    break :blk buf;
};

fn dumpState(allocator: std.mem.Allocator, msg: *std.ArrayList(u8), state: *const moira.State) !void {
    try msg.print(allocator, "\t.regs=[\n", .{});
    for (state.regs) |reg| {
        try msg.print(allocator, "\t\t0x{x:08},\n", .{reg});
    }
    try msg.print(allocator, "\t]\n", .{});

    try msg.print(allocator, "\t.ram=[\n", .{});
    for (state.ram.items) |ram| {
        try msg.print(allocator, "\t\t{{ .addr=0x{x:08}, .byte=0x{x:02} }},\n", .{ ram.addr, ram.byte });
    }
    try msg.print(allocator, "\t]\n", .{});
}

pub fn main() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    var cpu = try moira.Cpu.create(allocator, MAX_RAM_SIZE);
    defer cpu.destroy();

    const prog = [_]u16{
        0x23c0,
        0x0000,
        0xf000,
    };
    var pre_state: moira.State = .{};
    var post_state: moira.State = .{};

    pre_state.regs = blk: {
        var arr: [moira.REGISTERS_COUNT]u32 = undefined;
        arr[0] = 0x1234_5678;

        for (1..moira.REGISTERS_COUNT) |i| {
            arr[i] = 0;
        }
        break :blk arr;
    };

    cpu.installVectorTable(&VECTOR_TABLE);
    try cpu.reset(moira.Model.M68000);
    try cpu.setRegisters(pre_state.regs);

    for (prog, 0..) |word, i| {
        try cpu.writeWord(@intCast(PROG_ADDR + (i * 2)), word);
    }

    cpu.capturePre();
    try cpu.execute();
    try cpu.capturePost(&pre_state, &post_state);

    var msg: std.ArrayList(u8) = try .initCapacity(allocator, 0);

    const disasm = try cpu.disassemble(PROG_ADDR);
    try msg.print(allocator, ".name=\"{s}\"\n", .{disasm});

    try msg.print(allocator, ".op_words=[ ", .{});
    for (prog) |word| {
        try msg.print(allocator, "0x{x:04}, ", .{word});
    }
    try msg.print(allocator, "]\n", .{});

    try msg.print(allocator, ".pre_state={{\n", .{});
    try dumpState(allocator, &msg, &pre_state);
    try msg.print(allocator, "}}", .{});

    try msg.print(allocator, ".post_state={{\n", .{});
    try dumpState(allocator, &msg, &post_state);
    try msg.print(allocator, "}}", .{});

    std.log.debug("\n{s}", .{msg.items});
}
