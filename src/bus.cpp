#include "rvsim/bus.hpp"

namespace rvsim {


void
Bus::map(Addr base, std::unique_ptr<Device> _device) {

    if (_device == nullptr)
        throw std::invalid_argument(std::format("Bus::map: null device at 0x{:08x}", base));

    //lookup to see if the area is already mapped
    std::size_t size = _device->size();
    if (size == 0)
        throw std::invalid_argument(std::format("Bus::map: empty device at 0x{:08x}", base));
    
    // The device's last byte, base + size - 1, must be at most 0xFFFFFFFF: 16 bytes at
    // 0xFFFFFFF0 end exactly at the top and are fine, 16 bytes at 0xFFFFFFF8 are not.
    // Computed in 64 bits, because base + size can wrap around to a small number in 32 bits.
    std::uint64_t addressSpaceSize = std::uint64_t{1} << 32;
    if (std::uint64_t{base} + size > addressSpaceSize)
        throw std::invalid_argument(std::format(
            "Bus::map: {} bytes at 0x{:08x} run past 0xffffffff", size, base));

    if (overlaps(base, size))
        throw std::invalid_argument(std::format(
            "Bus::map: {} bytes at 0x{:08x} overlap a mapped device", size, base));

    MappedDevices deviceToMap;
    deviceToMap.baseAddress = base;
    deviceToMap.device = std::move(_device);
    listOfMappedDevices.push_back(std::move(deviceToMap));
}

Word
Bus::fetch(Addr address) {

    std::size_t byte = 4;
    FoundDevice foundDevice = lookup(address, byte, AccessType::Fetch);
    return foundDevice.device.read(foundDevice.deviceOffset, byte);
}

Word
Bus::read(Addr address, std::size_t width) {
    FoundDevice foundDevice = lookup(address, width, AccessType::Read);
    return foundDevice.device.read(foundDevice.deviceOffset, width);
}

void
Bus::write(Addr address, std::size_t width, Word data) {
    FoundDevice foundDevice = lookup(address, width, AccessType::Write);
    foundDevice.device.write(foundDevice.deviceOffset, width, data);
}

Bus::FoundDevice
Bus::lookup(Addr address, std::size_t width, AccessType access) {

    //make sure we are only reading the allowed number of bytes
    assert(width == 1 || width == 2 || width == 4);

    for(const MappedDevices& mapping : listOfMappedDevices) {

        /** check whether the address is in bounds
            in one of the devices */
        if(address < mapping.baseAddress)
            continue;
        const Addr offset = address - mapping.baseAddress;
        if (offset >= mapping.device->size())
            continue;

        //make sure that the full width of the fetch fits
        if (offset + width > mapping.device->size())
            throw MemoryFault(address, width, access);

        return FoundDevice{offset, *mapping.device};
    }

    throw MemoryFault(address, width, access);
}

bool
Bus::overlaps(Addr base, std::size_t size) const{
    const std::uint64_t newStart = base;
    const std::uint64_t newEnd = newStart + size;

    for (const MappedDevices& mapping : listOfMappedDevices) {
        const std::uint64_t oldStart = mapping.baseAddress;
        const std::uint64_t oldEnd = oldStart + mapping.device->size();

        if (newStart < oldEnd && oldStart < newEnd)
            return true;
    }
    return false;
}
}