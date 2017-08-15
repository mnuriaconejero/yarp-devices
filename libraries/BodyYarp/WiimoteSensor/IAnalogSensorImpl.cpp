// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "WiimoteSensor.hpp"

#include <cerrno>

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::read(yarp::sig::Vector &out)
{
    if (poll(fds, fds_num, -1) < 0 && errno != EINTR)
    {
        CD_ERROR("Cannot poll fds: %d.\n", -errno);
        return yarp::dev::IAnalogSensor::AS_ERROR;
    }

    int ret = xwii_iface_dispatch(iface, &event, sizeof(event));

    if (ret != 0)
    {
        if (ret != -EAGAIN)
        {
            CD_ERROR("Read failed with err: %d.\n", ret);
            return yarp::dev::IAnalogSensor::AS_ERROR;
        }
    }

    out.resize(5);  //-- [roll, pitch, A, B, yawActive]

    out[0] = roll;
    out[1] = pitch;
    out[2] = buttonA_pressed ? 1.0 : 0.0;
    out[3] = buttonB_pressed ? 1.0 : 0.0;
    out[4] = yawActive ? 1.0 : 0.0;

    switch (event.type)
    {
    case XWII_EVENT_KEY:
        CD_INFO("Keypress event: code %d, state %d\n", event.v.key.code, event.v.key.state);

        switch (event.v.key.code)
        {
        case XWII_KEY_A:
            if (event.v.key.state == 1)  //-- up
            {
                out[2] = 1.0;
                buttonA_pressed = true;
            }
            else if (event.v.key.state == 0)  //-- down
            {
                out[2] = 0.0;
                buttonA_pressed = false;
            }

            break;
        case XWII_KEY_B:
            if (event.v.key.state == 1)  //-- up
            {
                out[3] = 1.0;
                buttonB_pressed = true;
            }
            else if (event.v.key.state == 0)  //-- down
            {
                out[3] = 0.0;
                buttonB_pressed = false;
            }

            break;
        case XWII_KEY_ONE:
            CD_INFO("Roll active.\n");
            yawActive = false;
            out[4] = 0.0;
            break;
        case XWII_KEY_TWO:
            CD_INFO("Yaw active.\n");
            yawActive = true;
            out[4] = 1.0;
            break;
        }

        break;
    case XWII_EVENT_ACCEL:
        {
            int x = event.v.abs[0].x;
            int y = event.v.abs[0].y;
            int z = event.v.abs[0].z;

            CD_INFO("Accel event: [x] %d, [y] %d, [z] %d\n", x, y, z);

            double normX = ((double) x - calibZeroX) / (calibOneX - calibZeroX);
            double normY = ((double) y - calibZeroY) / (calibOneY - calibZeroY);
            double normZ = ((double) z - calibZeroZ) / (calibOneZ - calibZeroZ);

            out[0] = roll = normX;
            out[1] = pitch = normY;
        }
        break;
    }

    CD_INFO("[roll, pitch, A, B, yawActive] = [%f, %f, %.0f, %.0f, %.0f]\n", out[0], out[1], out[2], out[3], out[4]);

    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::getState(int ch)
{
    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::getChannels()
{
    return 5;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::calibrateSensor()
{
    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::calibrateSensor(const yarp::sig::Vector& value)
{
    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::calibrateChannel(int ch)
{
    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------

int roboticslab::WiimoteSensor::calibrateChannel(int ch, double value)
{
    return yarp::dev::IAnalogSensor::AS_OK;
}

// -----------------------------------------------------------------------------
