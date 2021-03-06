// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "DextraRawControlboard.hpp"

#include <ColorDebug.h>

using namespace roboticslab;

// -----------------------------------------------------------------------------

bool DextraRawControlboard::getAxisNameRaw(int axis, std::string & name)
{
    CD_DEBUG("(%d)\n", axis);
    CHECK_JOINT(axis);
    name = Synapse::LABELS[axis];
    return true;
}

// -----------------------------------------------------------------------------

bool DextraRawControlboard::getJointTypeRaw(int axis, yarp::dev::JointTypeEnum & type)
{
    CD_DEBUG("(%d)\n", axis);
    CHECK_JOINT(axis);
    type = yarp::dev::JointTypeEnum::VOCAB_JOINTTYPE_REVOLUTE;
    return true;
}

// -----------------------------------------------------------------------------
