// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "AmorControlboard.hpp"

#include <limits>

// ------------------- IControlLimits related ------------------------------------

bool roboticslab::AmorControlboard::setLimits(int axis, double min, double max)
{
    CD_ERROR("Not available (%d, %f, %f).\n", axis, min, max);
    return false;
}

// -----------------------------------------------------------------------------

bool roboticslab::AmorControlboard::getLimits(int axis, double *min, double *max)
{
    CD_DEBUG("(%d)\n", axis);

    if (!indexWithinRange(axis))
    {
        return false;
    }

    AMOR_JOINT_INFO parameters;

    if (amor_get_joint_info(handle, axis, &parameters) != AMOR_SUCCESS)
    {
        CD_ERROR("%s\n", amor_error());
        return false;
    }

    if (axis == 0 || parameters.lowerJointLimit == 0)
    {
        *min = -180;
        *max = 180;
    }
    else
    {
        *min = toDeg(parameters.lowerJointLimit);
        *max = toDeg(parameters.upperJointLimit);
    }

    return true;
}

// -----------------------------------------------------------------------------
