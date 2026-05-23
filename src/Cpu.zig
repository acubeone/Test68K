const wrapper = @import("moira_wrapper");
const std = @import("std");

pub const GenericError = error{
    Unknown,
    HandleNotInitialized,
    InvalidArgument,
};

pub const Registers = enum(u16) {
    D0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    A0,
    A1,
    A2,
    A3,
    A4,
    A5,
    A6,
    A7,

    PC,
    SR,
    SP,
    USP,
    ISP,
    VBR,
    SFC,
    DFC,
};
pub const REGISTERS_COUNT = std.meta.fields(Registers).len;

pub const Model = enum(c_int) {
    M68000 = 0,
    M68010,
};

const _MAX_BUF_SIZE = 256;

pub const Cpu = struct {
    handle: ?*wrapper.cpu_t,
    ram: []u8,

    pub fn create(allocator: std.mem.Allocator, ram_size: usize) !*Cpu {
        const self = try allocator.create(Cpu);
        errdefer allocator.destroy(self);

        self.ram = try allocator.alloc(u8, ram_size);
        errdefer allocator.free(self.ram);

        self.handle = wrapper.cpu_create(_read8, _read16, _write8, _write16);
        if (self.handle == null)
            return error.OutOfMemory;
        wrapper.cpu_set_userdata(self.handle.?, self);

        return self;
    }

    pub fn destroy(self: *Cpu, allocator: std.mem.Allocator) void {
        if (self.handle) |h|
            wrapper.cpu_destroy(h);

        allocator.free(self.ram);
        allocator.destroy(self);
    }

    pub fn reset(self: *Cpu, cpu_model: Model) !void {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        wrapper.cpu_reset(h, @as(i32, @intFromEnum(cpu_model)));
    }

    pub fn execute(self: *Cpu, cycles: i64) !void {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        wrapper.cpu_execute(h, cycles);
    }

    pub fn isInstructionValid(self: *const Cpu, op: u16, ext: u16) !bool {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_is_instruction_valid(h, op, ext);
    }

    pub fn disassemble(self: *const Cpu, allocator: std.mem.Allocator, addr: u32) ![]u8 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        var buf: [_MAX_BUF_SIZE]u8 = undefined;
        @memset(&buf, 0);

        _ = wrapper.cpu_disassemble(h, &buf, addr);
        return try allocator.dupe(u8, std.mem.sliceTo(&buf, 0));
    }

    pub fn getRegisters(self: *const Cpu, allocator: std.mem.Allocator) ![]u32 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        var regs: [REGISTERS_COUNT]u32 = undefined;
        @memset(&regs, 0);

        wrapper.cpu_get_registers(h, &regs);
        return try allocator.dupe(u32, &regs);
    }

    pub fn setRegisters(self: *const Cpu, regs: [REGISTERS_COUNT]u32) !void {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        wrapper.cpu_set_registers(h, &regs);
    }

    pub fn getClock(self: *const Cpu) !i64 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_get_clock(h);
    }

    pub fn setClock(self: *Cpu, cycles: i64) !void {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        wrapper.cpu_set_clock(h, cycles);
    }

    pub fn getPC0(self: *const Cpu) !u32 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_get_pc0(h);
    }

    pub fn readFC(self: *const Cpu) !u8 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_read_fc(h);
    }

    pub fn getIPL(self: *const Cpu) !u8 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_get_ipl(h);
    }

    pub fn getExceptionVector(self: *const Cpu) !u16 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_get_exception_vector(h);
    }

    pub fn getExceptionKind(self: *const Cpu) !u16 {
        const h = self.handle orelse return GenericError.HandleNotInitialized;

        return wrapper.cpu_get_exception_kind(h);
    }

    fn _fromUser(user: ?*anyopaque) *Cpu {
        return @ptrCast(@alignCast(user.?));
    }

    fn _read8(user: ?*anyopaque, c_addr: u32) callconv(.c) u8 {
        const self = _fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        if (addr >= self.ram.len)
            return 0;

        return self.ram[addr];
    }

    fn _read16(user: ?*anyopaque, c_addr: u32) callconv(.c) u16 {
        const self = _fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        if (addr >= self.ram.len or addr + 1 >= self.ram.len)
            return 0;

        const hi = self.ram[addr + 0];
        const lo = self.ram[addr + 1];
        return (hi << 8) | lo;
    }

    fn _write8(user: ?*anyopaque, c_addr: u32, byte: u8) callconv(.c) void {
        const self = _fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        if (addr >= self.ram.len)
            return;

        self.ram[addr] = byte;
    }

    fn _write16(user: ?*anyopaque, c_addr: u32, word: u16) callconv(.c) void {
        const self = _fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        if (addr >= self.ram.len or addr + 1 >= self.ram.len)
            return;

        self.ram[addr + 0] = @truncate(word >> 8);
        self.ram[addr + 1] = @truncate(word);
    }
};
