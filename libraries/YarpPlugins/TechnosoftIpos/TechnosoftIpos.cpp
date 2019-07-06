// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "TechnosoftIpos.hpp"

#include <cmath>
#include <cstring>

// -----------------------------------------------------------------------------

std::string roboticslab::TechnosoftIpos::msgToStr(const yarp::dev::CanMessage & message)
{
    std::stringstream tmp;
    for(int i=0; i < message.getLen()-1; i++)
    {
        tmp << std::hex << static_cast<int>(message.getData()[i]) << " ";
    }
    tmp << std::hex << static_cast<int>(message.getData()[message.getLen()-1]);
    tmp << ". canId(";
    tmp << std::dec << (message.getId() & 0x7F);
    tmp << ") via(";
    tmp << std::hex << (message.getId() & 0xFF80);
    tmp << ").";
    return tmp.str();
}

// -----------------------------------------------------------------------------

std::string roboticslab::TechnosoftIpos::msgToStr(uint32_t cob, uint16_t len, uint8_t * msgData)
{
    std::stringstream tmp;
    for(int i=0; i < len-1; i++)
    {
        tmp << std::hex << static_cast<int>(*(msgData+i)) << " ";
    }
    tmp << std::hex << static_cast<int>(*(msgData+len-1));
    tmp << ". canId(";
    tmp << std::dec << canId;
    tmp << ") via(";
    tmp << std::hex << cob;
    tmp << ").";
    return tmp.str();
}

// -----------------------------------------------------------------------------

bool roboticslab::TechnosoftIpos::send(uint32_t cob, uint16_t len, uint8_t * msgData)
{
    canBufferSemaphore.wait();

    if ( (lastUsage - yarp::os::Time::now()) < DELAY )
        yarp::os::Time::delay( lastUsage + DELAY - yarp::os::Time::now() );

    yarp::dev::CanMessage &msg = canOutputBuffer[0];
    msg.setId(cob + canId);
    msg.setLen(len);
    std::memcpy(msg.getData(), msgData, len * sizeof(uint8_t));

    unsigned int sent;

    if( ! canDevicePtr->canWrite(canOutputBuffer, 1, &sent, true) || sent == 0 )
        return false;

    lastUsage = yarp::os::Time::now();
    canBufferSemaphore.post();
    return true;
}

// -----------------------------------------------------------------------------

template <typename T_int, typename T_frac>
void roboticslab::TechnosoftIpos::encodeFixedPoint(double value, T_int * integer, T_frac * fractional)
{
    *integer = (T_int)value;
    *fractional = std::abs(value - *integer) * (1 << 4 * sizeof(T_frac));
}

// -----------------------------------------------------------------------------

template <typename T_int, typename T_frac>
double roboticslab::TechnosoftIpos::decodeFixedPoint(T_int integer, T_frac fractional)
{
    double frac = (double)fractional / (1 << 4 * sizeof(fractional));

    if (integer >= 0)
    {
        return integer + frac;
    }
    else
    {
        return integer - frac;
    }
}

// -----------------------------------------------------------------------------
