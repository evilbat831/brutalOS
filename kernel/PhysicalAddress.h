/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <base/Format.h>
#include <base/Types.h>

typedef u64 PhysicalPtr;
typedef u64 PhysicalSize;

class PhysicalAddress {
public:
    ALWAYS_INLINE static PhysicalPtr physical_page_base(PhysicalPtr page_address) { return page_address & ~(PhysicalPtr)0xfff; }
    ALWAYS_INLINE static size_t physical_page_index(PhysicalPtr page_address)
    {
        auto page_index = page_address >> 12;
        if constexpr (sizeof(size_t) < sizeof(PhysicalPtr))
            VERIFY(!(page_index & ~(PhysicalPtr)((size_t)-1)));
        return (size_t)(page_index);
    }

    PhysicalAddress() = default;
    explicit PhysicalAddress(PhysicalPtr address)
        : m_address(address)
    {
    }

    [[nodiscard]] PhysicalAddress offset(PhysicalPtr o) const { return PhysicalAddress(m_address + o); }
    [[nodiscard]] PhysicalPtr get() const { return m_address; }
    void set(PhysicalPtr address) { m_address = address; }
    void mask(PhysicalPtr m) { m_address &= m; }

    [[nodiscard]] bool is_null() const { return m_address == 0; }

    [[nodiscard]] u8* as_ptr() { return reinterpret_cast<u8*>(m_address); }
    [[nodiscard]] const u8* as_ptr() const { return reinterpret_cast<const u8*>(m_address); }

    [[nodiscard]] PhysicalAddress page_base() const { return PhysicalAddress(physical_page_base(m_address)); }
    [[nodiscard]] PhysicalPtr offset_in_page() const { return PhysicalAddress(m_address & 0xfff).get(); }

    bool operator==(const PhysicalAddress& other) const { return m_address == other.m_address; }
    bool operator!=(const PhysicalAddress& other) const { return m_address != other.m_address; }
    bool operator>(const PhysicalAddress& other) const { return m_address > other.m_address; }
    bool operator>=(const PhysicalAddress& other) const { return m_address >= other.m_address; }
    bool operator<(const PhysicalAddress& other) const { return m_address < other.m_address; }
    bool operator<=(const PhysicalAddress& other) const { return m_address <= other.m_address; }

private:
    PhysicalPtr m_address { 0 };
};

template<>
struct Base::Formatter<PhysicalAddress> : Base::Formatter<FormatString> {
    void format(FormatBuilder& builder, PhysicalAddress value)
    {
        if constexpr (sizeof(PhysicalPtr) == sizeof(u64))
            return Base::Formatter<FormatString>::format(builder, "P{:016x}", value.get());
        else
            return Base::Formatter<FormatString>::format(builder, "P{}", value.as_ptr());
    }
};
