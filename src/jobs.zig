const moira = @import("moira.zig");

pub const TestResult = struct {
    name: []u8,
    model: moira.Model,
    seed: u64,
    opwords: []u16,
    pre: State,
    post: State,
};

const Job = struct {
    regs: [moira.REGISTERS_COUNT]u32,
    rom: []u32,
    ram_len: u32,
};
