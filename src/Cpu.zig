const wrapper = @import("moira_wrapper");
const std = @import("std");

pub const Cpu = struct {
    handle: ?*wrapper.cpu_t,
    ram: []u8,

    pub fn create(allocator: std.mem.Allocator, ram_size: usize) !*Cpu {
        const self = try allocator.create(Cpu);
        errdefer allocator.free(self);

        self.ram = allocator.alloc(u8, ram_size);
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

    fn fromUser(user: ?*anyopaque) *Cpu {
        return @ptrCast(@alignCast(user.?));
    }

    fn _read8(user: ?*anyopaque, c_addr: u32) callconv(.c) u8 {
        const self = fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        return self.ram[addr];
    }

    fn _read16(user: ?*anyopaque, c_addr: u32) callconv(.c) u16 {
        const self = fromUser(user);

        const addr = @as(usize, @intCast(c_addr));

        const hi = self.ram[addr +% 0];
        const lo = self.ram[addr +% 1];

        return (hi << 8) | lo;
    }

    fn _write8(user: ?*anyopaque, c_addr: u32, c_byte: u8) callconv(.c) void {
        const self = fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        const byte = @as(u8, @intCast(c_byte));

        self.ram[addr] = byte;
    }

    fn _write16(user: ?*anyopaque, c_addr: u32, c_word: u16) callconv(.c) void {
        const self = fromUser(user);

        const addr = @as(usize, @intCast(c_addr));
        const word = @as(u8, @intCast(c_word));

        self.ram[addr +% 0] = @truncate(word >> 8);
        self.ram[addr +% 1] = @truncate(word);
    }
};
