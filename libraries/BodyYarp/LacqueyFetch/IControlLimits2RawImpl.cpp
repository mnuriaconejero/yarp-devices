// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "LacqueyFetch.hpp"

// ------------------- IControlLimitsRaw Related ------------------------------------

bool roboticslab::LacqueyFetch::setLimitsRaw(int axis, double min, double max)
{
    CD_INFO("(%d,%f,%f)\n",axis,min,max);

    //-- Check index within range
    if ( axis != 0 ) return false;

    //-- Store the new limits locally.
    this->max;
    this->min;

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::LacqueyFetch::getLimitsRaw(int axis, double *min, double *max)
{
    CD_INFO("(%d)\n",axis);

    //-- Check index within range
    if( axis != 0 ) return false;

    //-- Get the limits that have been locally stored.
    *min = this->min;
    *max = this->max;

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::LacqueyFetch::setVelLimitsRaw(int axis, double min, double max)
{
    CD_WARNING("Not implemented.\n");
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::LacqueyFetch::getVelLimitsRaw(int axis, double *min, double *max)
{
    CD_WARNING("Not implemented.\n");
    return true;
}

// -----------------------------------------------------------------------------
