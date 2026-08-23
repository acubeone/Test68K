const wrapper = @import("moira_wrapper");
const std = @import("std");

pub const Error = error{
    HandleNotInitialized,
    InvalidAddress,
};

pub const Model = enum(c_int) {
    M68000 = 0,
    M68010,
};

pub const Registers = enum(u16) {
    D0 = 0,
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

pub const MemOpKind = enum(u8) {
    Read = 0,
    Write,
};

pub const RamByte = struct {
    addr: u32,
    byte: u8,
};

pub const State = struct {
    regs: [REGISTERS_COUNT]u32 = undefined,
    ram: std.ArrayList(RamByte) = .empty,
};

pub const MemOp = struct {
    kind: MemOpKind,
    addr: u32, // Access address
    data: u16, // Data accessed
    fc: u8, // Function Code
    is_word: bool,
};

const _MAX_BUF_SIZE = 256;

pub const Cpu = struct {
    allocator: std.mem.Allocator,
    handle: ?*wrapper.cpu_t,
    ram: []u8,
    pre_ram: []u8,

    touched_list: std.ArrayList(u32) = .empty,
    mem_ops: std.ArrayList(MemOp) = .empty,
    pre_set: []bool,
    touched_set: []bool,
    is_trace_enabled: bool = false,

    pub fn create(allocator: std.mem.Allocator, ram_size: usize) !*Cpu {
        const self = try allocator.create(Cpu);
        errdefer allocator.destroy(self);

        self.touched_list = try .initCapacity(allocator, 1);
        self.mem_ops = try .initCapacity(allocator, 1);

        self.ram = try allocator.alloc(u8, ram_size);
        errdefer allocator.free(self.ram);
        @memset(self.ram, 0);

        self.pre_ram = try allocator.alloc(u8, ram_size);
        errdefer allocator.free(self.pre_ram);
        @memset(self.pre_ram, 0);

        self.pre_set = try allocator.alloc(bool, ram_size);
        errdefer allocator.free(self.pre_set);
        @memset(self.pre_set, false);

        self.touched_set = try allocator.alloc(bool, ram_size);
        errdefer allocator.free(self.touched_set);
        @memset(self.touched_set, false);

        self.handle = wrapper.cpu_create(read8, read16, write8, write16);
        if (self.handle == null)
            return error.OutOfMemory;
        wrapper.cpu_set_userdata(self.handle.?, self);

        self.allocator = allocator;
        return self;
    }

    pub fn destroy(self: *Cpu) void {
        if (self.handle) |h|
            wrapper.cpu_destroy(h);

        self.mem_ops.deinit(self.allocator);
        self.allocator.free(self.ram);
        self.allocator.free(self.pre_ram);
        self.allocator.free(self.pre_set);
        self.allocator.free(self.touched_set);
        self.allocator.destroy(self);
    }

    pub fn installVectorTable(self: *Cpu, table: []const u8) void {
        if (table.len == 0)
            return;

        @memcpy(self.ram[0..table.len], table);
    }

    pub fn capturePre(self: *Cpu) void {
        @memset(self.touched_set, false);
        self.touched_list.clearRetainingCapacity();

        self.is_trace_enabled = true;
    }

    pub fn capturePost(self: *Cpu, pre: *State, post: *State) !void {
        try self.buildRamDiff(&pre.ram, &post.ram);

        const post_regs = try self.getRegisters();
        @memcpy(&post.regs, post_regs[0..REGISTERS_COUNT]);
        self.is_trace_enabled = false;
    }

    pub fn reset(self: *Cpu, cpu_model: Model) !void {
        const h = self.handle orelse return Error.HandleNotInitialized;

        @memset(self.pre_set, false);
        wrapper.cpu_reset(h, @as(i32, @intFromEnum(cpu_model)));
    }

    pub fn execute(self: *Cpu) !void {
        const h = self.handle orelse return Error.HandleNotInitialized;

        wrapper.cpu_execute(h);
    }

    pub fn isInstructionValid(self: *const Cpu, op: u16, ext: u16) bool {
        const h = self.handle orelse return false;

        return wrapper.cpu_is_instruction_valid(h, op, ext);
    }

    pub fn disassemble(self: *const Cpu, addr: u32) ![]u8 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        var buf: [_MAX_BUF_SIZE]u8 = undefined;
        @memset(&buf, 0);

        _ = wrapper.cpu_disassemble(h, &buf, addr);
        return try self.allocator.dupe(u8, std.mem.sliceTo(&buf, 0));
    }

    pub fn getRegisters(self: *const Cpu) ![]u32 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        var regs: [REGISTERS_COUNT]u32 = undefined;
        @memset(&regs, 0);

        wrapper.cpu_get_registers(h, &regs);
        return try self.allocator.dupe(u32, &regs);
    }

    pub fn setRegisters(self: *const Cpu, regs: [REGISTERS_COUNT]u32) !void {
        const h = self.handle orelse return Error.HandleNotInitialized;

        wrapper.cpu_set_registers(h, @ptrCast(&regs));
    }

    pub fn getClock(self: *const Cpu) !i64 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_get_clock(h);
    }

    pub fn setClock(self: *Cpu, cycles: i64) !void {
        const h = self.handle orelse return Error.HandleNotInitialized;

        wrapper.cpu_set_clock(h, cycles);
    }

    pub fn getPC0(self: *const Cpu) !u32 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_get_pc0(h);
    }

    pub fn readFC(self: *const Cpu) !u8 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_read_fc(h);
    }

    pub fn getIPL(self: *const Cpu) !u8 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_get_ipl(h);
    }

    pub fn getExceptionVector(self: *const Cpu) !u16 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_get_exception_vector(h);
    }

    pub fn getExceptionKind(self: *const Cpu) !u16 {
        const h = self.handle orelse return Error.HandleNotInitialized;

        return wrapper.cpu_get_exception_kind(h);
    }

    pub fn readByte(self: *Cpu, addr: u32) !u8 {
        if (addr >= self.ram.len)
            return Error.InvalidAddress;

        const _addr = addr & 0x00ff_ffff;
        if (_addr >= self.ram.len)
            return 0;
        self.trackTouchedRam(_addr);

        const byte = self.ram[_addr];
        try self.registerMemOp(_addr, @intCast(byte), false, false);
        return byte;
    }

    pub fn readWord(self: *Cpu, addr: u32) !u16 {
        if (addr >= self.ram.len)
            return Error.InvalidAddress;
        const _addr = addr & 0x00ff_ffff;

        self.trackTouchedRam(_addr + 0);
        self.trackTouchedRam(_addr + 1);

        const hi: u16 = @intCast(self.ram[_addr + 0]);
        const lo: u16 = @intCast(self.ram[_addr + 1]);
        const word = hi << 8 | lo;

        try self.registerMemOp(_addr, word, true, false);
        return word;
    }

    pub fn writeByte(self: *Cpu, addr: u32, byte: u8) !void {
        if (addr >= self.ram.len)
            return Error.InvalidAddress;
        const _addr = addr & 0x00ff_ffff;

        if (!self.is_trace_enabled)
            self.trackPreRam(_addr, self.ram[_addr]);
        self.trackTouchedRam(_addr);

        self.ram[_addr] = byte;
        try self.registerMemOp(_addr, @intCast(byte), false, true);
    }

    pub fn writeWord(self: *Cpu, addr: u32, word: u16) !void {
        if (addr >= self.ram.len or addr + 1 >= self.ram.len)
            return Error.InvalidAddress;
        const _addr = addr & 0x00ff_ffff;

        if (!self.is_trace_enabled) {
            self.trackPreRam(_addr + 0, self.ram[_addr + 0]);
            self.trackPreRam(_addr + 1, self.ram[_addr + 1]);
        }
        self.trackTouchedRam(_addr + 0);
        self.trackTouchedRam(_addr + 1);

        self.ram[_addr + 0] = @truncate(word >> 8);
        self.ram[_addr + 1] = @truncate(word);
        try self.registerMemOp(_addr, word, true, true);
    }

    fn buildRamDiff(self: *const Cpu, pre: *std.ArrayList(RamByte), post: *std.ArrayList(RamByte)) !void {
        pre.clearRetainingCapacity();
        post.clearRetainingCapacity();

        for (self.touched_list.items) |addr| {
            const c_addr = addr & 0x00ff_ffff;

            const now = self.ram[c_addr];
            const prev = self.pre_ram[c_addr];

            if (now != prev) {
                try pre.append(self.allocator, .{ .addr = c_addr, .byte = prev });
                try post.append(self.allocator, .{ .addr = c_addr, .byte = now });
            }
        }
    }

    fn fromUser(user: ?*anyopaque) *Cpu {
        return @ptrCast(@alignCast(user.?));
    }

    fn registerMemOp(self: *Cpu, addr: u32, data: u16, is_word: bool, is_write: bool) !void {
        if (!self.is_trace_enabled)
            return;

        const current_fc = try self.readFC();
        try self.mem_ops.append(self.allocator, .{
            .kind = if (is_write) .Write else .Read,
            .addr = addr,
            .data = if (is_word) data else data & 0x00ff,
            .fc = current_fc,
            .is_word = is_word,
        });
    }

    fn trackTouchedRam(self: *Cpu, addr: u32) void {
        const c_addr = addr & 0x00ff_ffff;
        if (self.touched_set[c_addr])
            return;

        self.touched_set[c_addr] = true;
        self.touched_list.append(self.allocator, c_addr) catch |err| @panic(@errorName(err));
    }

    fn trackPreRam(self: *Cpu, addr: u32, byte: u8) void {
        if (self.pre_set[addr])
            return;

        self.pre_set[addr] = true;
        self.pre_ram[addr] = byte;
    }

    fn read8(user: ?*anyopaque, addr: u32) callconv(.c) u8 {
        const self = fromUser(user);
        return self.readByte(addr) catch @panic("ERR!");
    }

    fn read16(user: ?*anyopaque, addr: u32) callconv(.c) u16 {
        const self = fromUser(user);
        return self.readWord(addr) catch @panic("ERR!");
    }

    fn write8(user: ?*anyopaque, addr: u32, byte: u8) callconv(.c) void {
        const self = fromUser(user);
        self.writeByte(addr, byte) catch @panic("ERR!");
    }

    fn write16(user: ?*anyopaque, addr: u32, word: u16) callconv(.c) void {
        const self = fromUser(user);
        self.writeWord(addr, word) catch @panic("ERR!");
    }
};
