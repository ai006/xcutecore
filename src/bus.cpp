#include "rvsim/bus.hpp"

namespace rvsim {


void
Bus::map(Addr base, std::unique_ptr<Device> _device) {

    //lookup to see if the area is already mapped
    std::size_t size = _device->size();
    if (size == 0)
        throw std::runtime_error("Device size cannot be 0");
    if (overlaps(base, size))
        throw std::runtime_error("Address range already mapped");

    MappedDevices deviceToMap;
    deviceToMap.baseAddress = base;
    deviceToMap.device = std::move(_device);
    listOfMappedDevices.push_back(std::move(deviceToMap));
}

Word
Bus::fetch(Addr address) {

    std::size_t byte = 4;
    FoundDevice foundDevice = lookup(address, byte, AccessType::Fetch);
    return foundDevice.device.read(address, byte);
}

Word
Bus::read(Addr address, std::size_t width) {
    FoundDevice foundDevice = lookup(address, width, AccessType::Read);
    return foundDevice.device.read(address, width);
}

void
Bus::write(Addr address, std::size_t width, Word data) {
    FoundDevice foundDevice = lookup(address, width, AccessType::Write);
    foundDevice.device.write(address, width, data);
}

Bus::FoundDevice
Bus::lookup(Addr address, std::size_t width, AccessType access) {

    //make sure we are only reading the allowed number of bytes
    assert(width == 1 || width == 2 || width == 4);

    for(const auto& mapping : listOfMappedDevices) {

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

        return {mapping.baseAddress, *mapping.device};
    }

    throw MemoryFault(address, width, access);
}

bool
Bus::overlaps(Addr base, std::size_t size) const{
    std::uint64_t newStart = base;
    std::uint64_t newEnd   = static_cast<std::uint64_t>(base) + size - 1;

    for (const auto& mapping : listOfMappedDevices) {
        std::uint64_t oldStart = mapping.baseAddress;
        std::uint64_t oldEnd   = oldStart + mapping.device->size() - 1;

        // overlap unless one range ends before the other starts
        if (newStart <= oldEnd && oldStart <= newEnd)
            return true;
    }
    return false;
}
}