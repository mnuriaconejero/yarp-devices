// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "PdoProtocol.hpp"

#include <cstring>

#include <bitset>
#include <set>

#include <ColorDebug.h>

#include "nonstd/optional.hpp"

using namespace roboticslab;

struct PdoConfiguration::Private
{
    nonstd::optional<bool> valid;
    nonstd::optional<bool> rtr;
    nonstd::optional<std::uint8_t> transmissionType;
    nonstd::optional<std::uint16_t> inhibitTime;
    nonstd::optional<std::uint16_t> eventTimer;
    nonstd::optional<std::uint8_t> syncStartValue;
    std::set<std::uint32_t> mappings;
};

PdoConfiguration::PdoConfiguration()
    : priv(new Private)
{ }

PdoConfiguration::~PdoConfiguration()
{
    delete priv;
}

PdoConfiguration & PdoConfiguration::setValid(bool value)
{
    priv->valid = value;
    return *this;
}

PdoConfiguration & PdoConfiguration::setRtr(bool value)
{
    priv->rtr = value;
    return *this;
}

PdoConfiguration & PdoConfiguration::setTransmissionType(std::uint8_t value)
{
    priv->transmissionType = value;
    return *this;
}

PdoConfiguration & PdoConfiguration::setInhibitTime(std::uint16_t value)
{
    priv->inhibitTime = value;
    return *this;
}

PdoConfiguration & PdoConfiguration::setEventTimer(std::uint16_t value)
{
    priv->eventTimer = value;
    return *this;
}

PdoConfiguration & PdoConfiguration::setSyncStartValue(std::uint8_t value)
{
    priv->syncStartValue = value;
    return *this;
}

void PdoConfiguration::addMappingInternal(std::uint32_t value)
{
    priv->mappings.insert(value);
}

bool PdoProtocol::configure(const PdoConfiguration & conf)
{
    std::string pdoType;
    std::uint16_t commIdx;
    std::uint16_t mappingIdx;

    switch (getType())
    {
    case PdoType::RPDO:
        pdoType = "RPDO" + std::to_string(n);
        commIdx = 0x1400 + n - 1;
        mappingIdx = 0x1600 + n - 1;
        break;
    case PdoType::TPDO:
        pdoType = "TPDO" + std::to_string(n);
        commIdx = 0x1800 + n - 1;
        mappingIdx = 0x1A00 + n - 1;
        break;
    default:
        CD_ERROR("Unknown PDO type.\n");
        return false;
    }

    std::uint32_t cobId;

    if (!sdo->upload(std::string("COB-ID ") + pdoType, &cobId, commIdx, 0x01))
    {
        return false;
    }

    std::bitset<32>bits(cobId);
    bits.set(31);

    if (conf.priv->rtr)
    {
        if (getType() != PdoType::TPDO)
        {
            CD_ERROR("Illegal RTR usage on non-TPDO node.\n");
            return false;
        }

        bits.set(30, !*conf.priv->rtr);
    }

    if (!sdo->download(std::string("COB-ID ") + pdoType, bits.to_ulong(), commIdx, 0x01))
    {
        return false;
    }

    if (conf.priv->transmissionType && !sdo->download("Transmission type", *conf.priv->transmissionType, commIdx, 0x02))
    {
        return false;
    }

    if (conf.priv->inhibitTime && !sdo->download("Inhibit time", *conf.priv->inhibitTime, commIdx, 0x03))
    {
        return false;
    }

    if (conf.priv->eventTimer && !sdo->download("Event timer", *conf.priv->eventTimer, commIdx, 0x05))
    {
        return false;
    }

    if (conf.priv->syncStartValue)
    {
        if (getType() != PdoType::TPDO)
        {
            CD_ERROR("Illegal SYNC start value usage on non-TPDO node.\n");
            return false;
        }

        if (!sdo->download("SYNC start value", *conf.priv->syncStartValue, commIdx, 0x06))
        {
            return false;
        }
    }

    if (!conf.priv->mappings.empty())
    {
        if (!sdo->download<std::uint8_t>(pdoType + " mapping parameters", 0, mappingIdx))
        {
            return false;
        }

        unsigned int i = 0;

        for (auto mapping : conf.priv->mappings)
        {
            i++;
            std::string name = pdoType + ": mapped object " + std::to_string(i);

            if (!sdo->download(name, mapping, mappingIdx, i))
            {
                return false;
            }
        }

        if (!sdo->download<std::uint8_t>(pdoType + " mapping parameters", i, mappingIdx))
        {
            return false;
        }
    }

    if (!conf.priv->valid || *conf.priv->valid)
    {
        bits.reset(31);

        if (!sdo->download(std::string("COB-ID ") + pdoType, bits.to_ulong(), commIdx, 0x01))
        {
            return false;
        }
    }

    return true;
}

void ReceivePdo::packInternal(std::uint8_t * buff, const void * data, std::size_t size)
{
    std::memcpy(buff, data, size);
}

bool ReceivePdo::writeInternal(const std::uint8_t * data, std::size_t size)
{
    return sender->prepareMessage(message_builder(cob + id, size, data));
}

void TransmitPdo::unpackInternal(void * data, const std::uint8_t * buff, std::size_t size)
{
    std::memcpy(data, buff, size);;
}
