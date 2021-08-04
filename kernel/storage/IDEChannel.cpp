/*
 * Copyright (c) 2021, NukeWilliams
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <base/ByteBuffer.h>
#include <base/Singleton.h>
#include <base/StringView.h>
#include <kernel/IO.h>
#include <kernel/Process.h>
#include <kernel/Sections.h>
#include <kernel/storage/ATA.h>
#include <kernel/storage/IDEChannel.h>
#include <kernel/storage/IDEController.h>
#include <kernel/storage/PATADiskDevice.h>
#include <kernel/vm/MemoryManager.h>
#include <kernel/WorkQueue.h>

namespace Kernel {

#define PATA_PRIMARY_IRQ 14
#define PATA_SECONDARY_IRQ 15

UNMAP_AFTER_INIT NonnullRefPtr<IDEChannel> IDEChannel::create(const IDEController& controller, IOAddressGroup io_group, ChannelType type)
{
    return adopt_ref(*new IDEChannel(controller, io_group, type));
}

UNMAP_AFTER_INIT NonnullRefPtr<IDEChannel> IDEChannel::create(const IDEController& controller, u8 irq, IOAddressGroup io_group, ChannelType type)
{
    return adopt_ref(*new IDEChannel(controller, irq, io_group, type));
}

RefPtr<StorageDevice> IDEChannel::master_device() const
{
    return m_master;
}

RefPtr<StorageDevice> IDEChannel::slave_device() const
{
    return m_slave;
}

UNMAP_AFTER_INIT void IDEChannel::initialize()
{
    disable_irq();
    dbgln_if(PATA_DEBUG, "IDEChannel: {} IO base: {}", channel_type_string(), m_io_group.io_base());
    dbgln_if(PATA_DEBUG, "IDEChannel: {} control base: {}", channel_type_string(), m_io_group.control_base());
    if (m_io_group.bus_master_base().has_value())
        dbgln_if(PATA_DEBUG, "IDEChannel: {} bus master base: {}", channel_type_string(), m_io_group.bus_master_base().value());
    else
        dbgln_if(PATA_DEBUG, "IDEChannel: {} bus master base disabled", channel_type_string());
    m_parent_controller->enable_pin_based_interrupts();

    u8 device_control = m_io_group.control_base().in<u8>();

    IO::delay(30000);
    m_io_group.control_base().out<u8>(device_control | (1 << 2));

    IO::delay(30000);
    m_io_group.control_base().out<u8>(device_control);
    if (!wait_until_not_busy(false, 30000)) {
        dbgln("IDEChannel: reset failed, busy flag on master stuck");
        return;
    }

    if (!wait_until_not_busy(true, 30000)) {
        dbgln("IDEChannel: reset failed, busy flag on slave stuck");
        return;
    }

    detect_disks();

    clear_pending_interrupts();
}

UNMAP_AFTER_INIT IDEChannel::IDEChannel(const IDEController& controller, u8 irq, IOAddressGroup io_group, ChannelType type)
    : IRQHandler(irq)
    , m_channel_type(type)
    , m_io_group(io_group)
    , m_parent_controller(controller)
{
    initialize();
}

UNMAP_AFTER_INIT IDEChannel::IDEChannel(const IDEController& controller, IOAddressGroup io_group, ChannelType type)
    : IRQHandler(type == ChannelType::Primary ? PATA_PRIMARY_IRQ : PATA_SECONDARY_IRQ)
    , m_channel_type(type)
    , m_io_group(io_group)
    , m_parent_controller(controller)
{
    initialize();
}

void IDEChannel::clear_pending_interrupts() const
{
    m_io_group.io_base().offset(ATA_REG_STATUS).in<u8>();
}

UNMAP_AFTER_INIT IDEChannel::~IDEChannel()
{
}

void IDEChannel::start_request(AsyncBlockDeviceRequest& request, bool is_slave, u16 capabilities)
{
    MutexLocker locker(m_lock);
    VERIFY(m_current_request.is_null());

    dbgln_if(PATA_DEBUG, "IDEChannel::start_request");

    m_current_request = request;
    m_current_request_block_index = 0;
    m_current_request_flushing_cache = false;

    if (request.request_type() == AsyncBlockDeviceRequest::Read)
        ata_read_sectors(is_slave, capabilities);
    else
        ata_write_sectors(is_slave, capabilities);
}

void IDEChannel::complete_current_request(AsyncDeviceRequest::RequestResult result)
{
    VERIFY(m_current_request);
    VERIFY(m_request_lock.is_locked());

    g_io_work->queue([this, result]() {
        dbgln_if(PATA_DEBUG, "IDEChannel::complete_current_request result: {}", (int)result);
        MutexLocker locker(m_lock);
        VERIFY(m_current_request);
        auto current_request = m_current_request;
        m_current_request.clear();
        current_request->complete(result);
    });
}

static void print_ide_status(u8 status)
{
    dbgln("IDEChannel: print_ide_status: DRQ={} BSY={}, DRDY={}, DSC={}, DF={}, CORR={}, IDX={}, ERR={}",
        (status & ATA_SR_DRQ) != 0,
        (status & ATA_SR_BSY) != 0,
        (status & ATA_SR_DRDY) != 0,
        (status & ATA_SR_DSC) != 0,
        (status & ATA_SR_DF) != 0,
        (status & ATA_SR_CORR) != 0,
        (status & ATA_SR_IDX) != 0,
        (status & ATA_SR_ERR) != 0);
}

void IDEChannel::try_disambiguate_error()
{
    VERIFY(m_lock.is_locked());
    dbgln("IDEChannel: Error cause:");

    switch (m_device_error) {
    case ATA_ER_BBK:
        dbgln("IDEChannel: - Bad block");
        break;
    case ATA_ER_UNC:
        dbgln("IDEChannel: - Uncorrectable data");
        break;
    case ATA_ER_MC:
        dbgln("IDEChannel: - Media changed");
        break;
    case ATA_ER_IDNF:
        dbgln("IDEChannel: - ID mark not found");
        break;
    case ATA_ER_MCR:
        dbgln("IDEChannel: - Media change request");
        break;
    case ATA_ER_ABRT:
        dbgln("IDEChannel: - Command aborted");
        break;
    case ATA_ER_TK0NF:
        dbgln("IDEChannel: - Track 0 not found");
        break;
    case ATA_ER_AMNF:
        dbgln("IDEChannel: - No address mark");
        break;
    default:
        dbgln("IDEChannel: - No one knows");
        break;
    }
}

bool IDEChannel::handle_irq(const RegisterState&)
{
    u8 status = m_io_group.io_base().offset(ATA_REG_STATUS).in<u8>();

    m_entropy_source.add_random_event(status);

    ScopedSpinLock lock(m_request_lock);
    dbgln_if(PATA_DEBUG, "IDEChannel: interrupt: DRQ={}, BSY={}, DRDY={}",
        (status & ATA_SR_DRQ) != 0,
        (status & ATA_SR_BSY) != 0,
        (status & ATA_SR_DRDY) != 0);

    if (!m_current_request) {
        dbgln("IDEChannel: IRQ but no pending request!");
        return false;
    }

    if (status & ATA_SR_ERR) {
        print_ide_status(status);
        m_device_error = m_io_group.io_base().offset(ATA_REG_ERROR).in<u8>();
        dbgln("IDEChannel: Error {:#02x}!", (u8)m_device_error);
        try_disambiguate_error();
        complete_current_request(AsyncDeviceRequest::Failure);
        return true;
    }
    m_device_error = 0;

    g_io_work->queue([this]() {
        MutexLocker locker(m_lock);
        ScopedSpinLock lock(m_request_lock);
        if (m_current_request->request_type() == AsyncBlockDeviceRequest::Read) {
            dbgln_if(PATA_DEBUG, "IDEChannel: Read block {}/{}", m_current_request_block_index, m_current_request->block_count());

            if (ata_do_read_sector()) {
                if (++m_current_request_block_index >= m_current_request->block_count()) {
                    complete_current_request(AsyncDeviceRequest::Success);
                    return;
                }

                enable_irq();
            }
        } else {
            if (!m_current_request_flushing_cache) {
                dbgln_if(PATA_DEBUG, "IDEChannel: Wrote block {}/{}", m_current_request_block_index, m_current_request->block_count());
                if (++m_current_request_block_index >= m_current_request->block_count()) {

                    VERIFY(!m_current_request_flushing_cache);
                    m_current_request_flushing_cache = true;
                    m_io_group.io_base().offset(ATA_REG_COMMAND).out<u8>(ATA_CMD_CACHE_FLUSH);
                } else {

                    ata_do_write_sector();
                }
            } else {
                complete_current_request(AsyncDeviceRequest::Success);
            }
        }
    });
    return true;
}

static void io_delay()
{
    for (int i = 0; i < 4; ++i)
        IO::in8(0x3f6);
}

bool IDEChannel::wait_until_not_busy(bool slave, size_t milliseconds_timeout)
{
    IO::delay(20);
    m_io_group.io_base().offset(ATA_REG_HDDEVSEL).out<u8>(0xA0 | (slave << 4)); 
    IO::delay(20);
    size_t time_elapsed = 0;
    while (m_io_group.control_base().in<u8>() & ATA_SR_BSY && time_elapsed <= milliseconds_timeout) {
        IO::delay(1000);
        time_elapsed++;
    }
    return time_elapsed <= milliseconds_timeout;
}

bool IDEChannel::wait_until_not_busy(size_t milliseconds_timeout)
{
    size_t time_elapsed = 0;
    while (m_io_group.control_base().in<u8>() & ATA_SR_BSY && time_elapsed <= milliseconds_timeout) {
        IO::delay(1000);
        time_elapsed++;
    }
    return time_elapsed <= milliseconds_timeout;
}

String IDEChannel::channel_type_string() const
{
    if (m_channel_type == ChannelType::Primary)
        return "Primary";

    return "Secondary";
}

UNMAP_AFTER_INIT void IDEChannel::detect_disks()
{
    auto channel_string = [](u8 i) -> const char* {
        if (i == 0)
            return "master";

        return "slave";
    };

    for (auto i = 0; i < 2; i++) {

        IO::delay(20);
        m_io_group.io_base().offset(ATA_REG_HDDEVSEL).out<u8>(0xA0 | (i << 4)); 
        IO::delay(20);

        auto status = m_io_group.control_base().in<u8>();
        if (status == 0x0) {
            dbgln_if(PATA_DEBUG, "IDEChannel: No {} {} disk detected!", channel_type_string().to_lowercase(), channel_string(i));
            continue;
        }

        m_io_group.io_base().offset(ATA_REG_SECCOUNT0).out<u8>(0);
        m_io_group.io_base().offset(ATA_REG_LBA0).out<u8>(0);
        m_io_group.io_base().offset(ATA_REG_LBA1).out<u8>(0);
        m_io_group.io_base().offset(ATA_REG_LBA2).out<u8>(0);
        m_io_group.io_base().offset(ATA_REG_COMMAND).out<u8>(ATA_CMD_IDENTIFY); 

        if (!wait_until_not_busy(2000)) {
            dbgln_if(PATA_DEBUG, "IDEChannel: No {} {} disk detected, BSY flag was not reset!", channel_type_string().to_lowercase(), channel_string(i));
            continue;
        }

        bool check_for_atapi = false;
        bool device_presence = true;
        PATADiskDevice::InterfaceType interface_type = PATADiskDevice::InterfaceType::ATA;

        size_t milliseconds_elapsed = 0;
        for (;;) {

            if (milliseconds_elapsed > 2000)
                break;
            u8 status = m_io_group.control_base().in<u8>();
            if (status & ATA_SR_ERR) {
                dbgln_if(PATA_DEBUG, "IDEChannel: {} {} device is not ATA. Will check for ATAPI.", channel_type_string(), channel_string(i));
                check_for_atapi = true;
                break;
            }

            if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                dbgln_if(PATA_DEBUG, "IDEChannel: {} {} device appears to be ATA.", channel_type_string(), channel_string(i));
                interface_type = PATADiskDevice::InterfaceType::ATA;
                break;
            }

            if (status == 0 || status == 0xFF) {
                dbgln_if(PATA_DEBUG, "IDEChannel: {} {} device presence - none.", channel_type_string(), channel_string(i));
                device_presence = false;
                break;
            }

            IO::delay(1000);
            milliseconds_elapsed++;
        }
        if (!device_presence) {
            continue;
        }
        if (milliseconds_elapsed > 10000) {
            dbgln_if(PATA_DEBUG, "IDEChannel: {} {} device state unknown. Timeout exceeded.", channel_type_string(), channel_string(i));
            continue;
        }

        if (check_for_atapi) {
            u8 cl = m_io_group.io_base().offset(ATA_REG_LBA1).in<u8>();
            u8 ch = m_io_group.io_base().offset(ATA_REG_LBA2).in<u8>();

            if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {
                interface_type = PATADiskDevice::InterfaceType::ATAPI;
                dbgln("IDEChannel: {} {} device appears to be ATAPI. We're going to ignore it for now as we don't support it.", channel_type_string(), channel_string(i));
                continue;
            } else {
                dbgln("IDEChannel: {} {} device doesn't appear to be ATA or ATAPI. Ignoring it.", channel_type_string(), channel_string(i));
                continue;
            }
        }

        ByteBuffer wbuf = ByteBuffer::create_uninitialized(512);
        ByteBuffer bbuf = ByteBuffer::create_uninitialized(512);
        u8* b = bbuf.data();
        u16* w = (u16*)wbuf.data();

        for (u32 i = 0; i < 256; ++i) {
            u16 data = m_io_group.io_base().offset(ATA_REG_DATA).in<u16>();
            *(w++) = data;
            *(b++) = MSB(data);
            *(b++) = LSB(data);
        }

        for (u32 i = 93; i > 54 && bbuf[i] == ' '; --i)
            bbuf[i] = 0;

        volatile ATAIdentifyBlock& identify_block = (volatile ATAIdentifyBlock&)(*wbuf.data());

        u16 capabilities = identify_block.capabilities[0];

        if (!(capabilities & ATA_CAP_LBA))
            continue;
        u64 max_addressable_block = identify_block.max_28_bit_addressable_logical_sector;

        if (identify_block.commands_and_feature_sets_supported[1] & (1 << 10))
            max_addressable_block = identify_block.user_addressable_logical_sectors_count;

        dbgln("IDEChannel: {} {} {} device found: Name={}, Capacity={}, Capabilities={:#04x}", channel_type_string(), channel_string(i), interface_type == PATADiskDevice::InterfaceType::ATA ? "ATA" : "ATAPI", ((char*)bbuf.data() + 54), max_addressable_block * 512, capabilities);
        if (i == 0) {
            m_master = PATADiskDevice::create(m_parent_controller, *this, PATADiskDevice::DriveType::Master, interface_type, capabilities, max_addressable_block);
        } else {
            m_slave = PATADiskDevice::create(m_parent_controller, *this, PATADiskDevice::DriveType::Slave, interface_type, capabilities, max_addressable_block);
        }
    }
}

void IDEChannel::ata_access(Direction direction, bool slave_request, u64 lba, u8 block_count, u16 capabilities)
{
    VERIFY(m_lock.is_locked());
    VERIFY(m_request_lock.is_locked());
    LBAMode lba_mode;
    u8 head = 0;

    VERIFY(capabilities & ATA_CAP_LBA);
    if (lba >= 0x10000000) {
        lba_mode = LBAMode::FortyEightBit;
        head = 0;
    } else {
        lba_mode = LBAMode::TwentyEightBit;
        head = (lba & 0xF000000) >> 24;
    }

    wait_until_not_busy(1000);

    m_io_group.io_base().offset(ATA_REG_HDDEVSEL).out<u8>(0xE0 | (static_cast<u8>(slave_request) << 4) | head);
    IO::delay(20);

    if (lba_mode == LBAMode::FortyEightBit) {
        m_io_group.io_base().offset(ATA_REG_SECCOUNT1).out<u8>(0);
        m_io_group.io_base().offset(ATA_REG_LBA3).out<u8>((lba & 0xFF000000) >> 24);
        m_io_group.io_base().offset(ATA_REG_LBA4).out<u8>((lba & 0xFF00000000ull) >> 32);
        m_io_group.io_base().offset(ATA_REG_LBA5).out<u8>((lba & 0xFF0000000000ull) >> 40);
    }

    m_io_group.io_base().offset(ATA_REG_SECCOUNT0).out<u8>(block_count);
    m_io_group.io_base().offset(ATA_REG_LBA0).out<u8>((lba & 0x000000FF) >> 0);
    m_io_group.io_base().offset(ATA_REG_LBA1).out<u8>((lba & 0x0000FF00) >> 8);
    m_io_group.io_base().offset(ATA_REG_LBA2).out<u8>((lba & 0x00FF0000) >> 16);

    for (;;) {
        auto status = m_io_group.control_base().in<u8>();
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY))
            break;
    }
    send_ata_io_command(lba_mode, direction);
    enable_irq();
}

void IDEChannel::send_ata_io_command(LBAMode lba_mode, Direction direction) const
{
    if (lba_mode != LBAMode::FortyEightBit) {
        m_io_group.io_base().offset(ATA_REG_COMMAND).out<u8>(direction == Direction::Read ? ATA_CMD_READ_PIO : ATA_CMD_WRITE_PIO);
    } else {
        m_io_group.io_base().offset(ATA_REG_COMMAND).out<u8>(direction == Direction::Read ? ATA_CMD_READ_PIO_EXT : ATA_CMD_WRITE_PIO_EXT);
    }
}

bool IDEChannel::ata_do_read_sector()
{
    VERIFY(m_lock.is_locked());
    VERIFY(m_request_lock.is_locked());
    VERIFY(!m_current_request.is_null());
    dbgln_if(PATA_DEBUG, "IDEChannel::ata_do_read_sector");
    auto& request = *m_current_request;
    auto out_buffer = request.buffer().offset(m_current_request_block_index * 512);
    auto result = request.write_to_buffer_buffered<512>(out_buffer, 512, [&](u8* buffer, size_t buffer_bytes) {
        for (size_t i = 0; i < buffer_bytes; i += sizeof(u16))
            *(u16*)&buffer[i] = IO::in16(m_io_group.io_base().offset(ATA_REG_DATA).get());
        return buffer_bytes;
    });
    if (result.is_error()) {
        complete_current_request(AsyncDeviceRequest::MemoryFault);
        return false;
    }
    return true;
}

void IDEChannel::ata_read_sectors(bool slave_request, u16 capabilities)
{
    VERIFY(m_lock.is_locked());
    VERIFY(!m_current_request.is_null());
    VERIFY(m_current_request->block_count() <= 256);

    ScopedSpinLock m_lock(m_request_lock);
    dbgln_if(PATA_DEBUG, "IDEChannel::ata_read_sectors");
    dbgln_if(PATA_DEBUG, "IDEChannel: Reading {} sector(s) @ LBA {}", m_current_request->block_count(), m_current_request->block_index());
    ata_access(Direction::Read, slave_request, m_current_request->block_index(), m_current_request->block_count(), capabilities);
}

void IDEChannel::ata_do_write_sector()
{
    VERIFY(m_lock.is_locked());
    VERIFY(m_request_lock.is_locked());
    VERIFY(!m_current_request.is_null());
    auto& request = *m_current_request;

    io_delay();
    while ((m_io_group.control_base().in<u8>() & ATA_SR_BSY) || !(m_io_group.control_base().in<u8>() & ATA_SR_DRQ))
        ;

    u8 status = m_io_group.control_base().in<u8>();
    VERIFY(status & ATA_SR_DRQ);

    auto in_buffer = request.buffer().offset(m_current_request_block_index * 512);
    dbgln_if(PATA_DEBUG, "IDEChannel: Writing 512 bytes (part {}) (status={:#02x})...", m_current_request_block_index, status);
    auto result = request.read_from_buffer_buffered<512>(in_buffer, 512, [&](u8 const* buffer, size_t buffer_bytes) {
        for (size_t i = 0; i < buffer_bytes; i += sizeof(u16))
            IO::out16(m_io_group.io_base().offset(ATA_REG_DATA).get(), *(const u16*)&buffer[i]);
        return buffer_bytes;
    });
    if (result.is_error())
        complete_current_request(AsyncDeviceRequest::MemoryFault);
}

void IDEChannel::ata_write_sectors(bool slave_request, u16 capabilities)
{
    VERIFY(m_lock.is_locked());
    VERIFY(!m_current_request.is_null());
    VERIFY(m_current_request->block_count() <= 256);

    ScopedSpinLock m_lock(m_request_lock);
    dbgln_if(PATA_DEBUG, "IDEChannel: Writing {} sector(s) @ LBA {}", m_current_request->block_count(), m_current_request->block_index());
    ata_access(Direction::Write, slave_request, m_current_request->block_index(), m_current_request->block_count(), capabilities);
    ata_do_write_sector();
}
}