// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "CanBusControlboard.hpp"

#include <ColorDebug.h>

#include "ICanBusSharer.hpp"

using namespace roboticslab;

// -----------------------------------------------------------------------------

namespace
{
    bool setSingleKeyValuePair(const std::string & key, const yarp::os::Bottle & val, const DeviceMapper & mapper)
    {
        if (val.size() != 2 || !val.get(0).isString() || !val.get(1).isList())
        {
            CD_ERROR("Illegal bottle format, expected string key and list value.\n");
            return false;
        }

        bool setAll = key == "all";
        bool allOk = true;

        for (const auto & t : mapper.getDevicesWithOffsets())
        {
            auto * iCanBusSharer = std::get<0>(t)->castToType<ICanBusSharer>();

            if (setAll || key == "id" + std::to_string(iCanBusSharer->getId()))
            {
                auto * p = std::get<0>(t)->getHandle<yarp::dev::IRemoteVariablesRaw>();

                if (!p)
                {
                    if (!setAll)
                    {
                        CD_ERROR("Unsupported interface: \"%s\".\n", key.c_str());
                        return false;
                    }

                    CD_WARNING("Unsupported interface: \"id%d\".\n", iCanBusSharer->getId());
                }
                else if (!p->setRemoteVariableRaw(val.get(0).asString(), *val.get(1).asList()))
                {
                    if (!setAll)
                    {
                        return false;
                    }

                    CD_WARNING("Request failed: \"id%d\".\n", iCanBusSharer->getId());
                    allOk = false;
                }
                else if (!setAll)
                {
                    return true;
                }
            }
        }

        if (!setAll)
        {
            CD_ERROR("Node \"%s\" not found, type e.g. \"id19\" or \"all\".\n", key.c_str());
            return false;
        }

        return allOk;
    }
}

// -----------------------------------------------------------------------------

bool CanBusControlboard::getRemoteVariable(std::string key, yarp::os::Bottle & val)
{
    CD_DEBUG("%s\n", key.c_str());

    bool queryAll = key == "all";
    val.clear();

    for (const auto & t : deviceMapper.getDevicesWithOffsets())
    {
        auto * iCanBusSharer = std::get<0>(t)->castToType<ICanBusSharer>();
        yarp::os::Bottle & nodeVal = queryAll ? val.addList() : val;

        if (queryAll || key == "id" + std::to_string(iCanBusSharer->getId()))
        {
            auto * p = std::get<0>(t)->getHandle<yarp::dev::IRemoteVariablesRaw>();
            yarp::os::Bottle b;

            if (p && p->getRemoteVariablesListRaw(&b))
            {
                nodeVal.addString("id" + std::to_string(iCanBusSharer->getId()));
                bool ok = true;

                for (int j = 0; j < b.size(); j++)
                {
                    ok &= p->getRemoteVariableRaw(b.get(j).asString(), nodeVal.addList());
                }

                if (!queryAll)
                {
                    return ok;
                }
            }

            if (!queryAll)
            {
                CD_ERROR("Unsupported interface: \"%s\".\n", key.c_str());
                return false;
            }
            else if (!p)
            {
                CD_WARNING("Unsupported interface: \"id%d\".\n", iCanBusSharer->getId());
            }
        }
    }

    if (!queryAll)
    {
        CD_ERROR("Node \"%s\" not found, type e.g. \"id19\" or \"all\".\n", key.c_str());
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------

bool CanBusControlboard::setRemoteVariable(std::string key, const yarp::os::Bottle & val)
{
    CD_DEBUG("%s\n", key.c_str());

    if (key == "multi")
    {
        bool ok = true;

        for (int i = 0; i < val.size(); i++)
        {
            if (!val.get(i).isList())
            {
                CD_ERROR("Not a list: %s\n.\n", val.get(i).toString().c_str());
                return false;
            }

            yarp::os::Bottle * b = val.get(i).asList();

            if (b->size() != 2 || !b->get(0).isString() || !b->get(1).isList())
            {
                CD_ERROR("Illegal bottle format, expected string key and list value: %s.\n", b->toString().c_str());
                return false;
            }

            if (b->get(0).asString() == "all")
            {
                CD_ERROR("Cannot set all node vars in multi mode.\n");
                return false;
            }

            ok &= setRemoteVariable(b->get(0).asString(), *b->get(1).asList());
        }

        return ok;
    }

    if (val.size() == 0)
    {
        CD_ERROR("Empty value list.\n");
        return false;
    }

    if (val.get(0).isList())
    {
        bool ok = true;

        for (int i = 0; i < val.size(); i++)
        {
            if (!val.get(i).isList())
            {
                CD_ERROR("Not a list: %s.\n", val.get(i).toString().c_str());
                return false;
            }

            ok &= setSingleKeyValuePair(key, *val.get(i).asList(), deviceMapper);
        }

        return ok;
    }

    return setSingleKeyValuePair(key, val, deviceMapper);
}

// -----------------------------------------------------------------------------

bool CanBusControlboard::getRemoteVariablesList(yarp::os::Bottle * listOfKeys)
{
    CD_DEBUG("\n");

    listOfKeys->clear();

    // Place each key in its own list so that clients can just call check('<key>') or !find('<key>').isNull().
    for (const auto & t : deviceMapper.getDevicesWithOffsets())
    {
        auto * iCanBusSharer = std::get<0>(t)->castToType<ICanBusSharer>();
        listOfKeys->addString("id" + std::to_string(iCanBusSharer->getId()));
    }

    return true;
}

// -----------------------------------------------------------------------------
