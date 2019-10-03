// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "TechnosoftIpos.hpp"

#include <bitset>

#include <ColorDebug.h>

#include "CanUtils.hpp"

using namespace roboticslab;

// -----------------------------------------------------------------------------

namespace
{
    bool retrieveDrivePeakCurrent(std::uint32_t productCode, double *peakCurrent)
    {
        switch (productCode)
        {
            case 24300101: // iPOS2401 MX-CAN
            case 24200121: // iPOS2401 MX-CAT
                *peakCurrent = 0.9;
                break;
            case 28001001: // iPOS3602 VX-CAN
            case 28001021: // iPOS3602 VX-CAT
            case 28001101: // iPOS3602 MX-CAN
            case 28001201: // iPOS3602 BX-CAN
            case 28001501: // iPOS3602 HX-CAN
                *peakCurrent = 3.2;
                break;
            case 28002001: // iPOS3604 VX-CAN
            case 28002021: // iPOS3604 VX-CAT
            case 28002101: // iPOS3604 MX-CAN
            case 28002201: // iPOS3604 BX-CAN
            case 28002501: // iPOS3604 HX-CAN
                *peakCurrent = 10.0;
                break;
            case 27014001: // iPOS4808 VX-CAN
            case 27014101: // iPOS4808 MX-CAN
            case 27014121: // iPOS4808 MX-CAT
            case 27414101: // iPOS4808 MY-CAN (standard)
            case 27424101: // iPOS4808 MY-CAN (extended)
            case 27314111: // iPOS4808 MY-CAN-STO (standard)
            case 27324111: // iPOS4808 MY-CAN-STO (extended)
            case 27314121: // iPOS4808 MY-CAT-STO (standard)
            case 27324121: // iPOS4808 MY-CAT-STO (extended)
            case 27014201: // iPOS4808 BX-CAN
            case 27214201: // iPOS4808 BX-CAN (standard)
            case 27214701: // iPOS4808 BX-CAN (hall)
            case 27214221: // iPOS4808 BX-CAT (standard)
            case 27214721: // iPOS4808 BX-CAT (hall)
            case 27314221: // iPOS4808 BX-CAT-STO (standard)
            case 27314721: // iPOS4808 BX-CAT-STO (hall)
            case 29025201: // iPOS8010 BX-CAN
            case 29025221: // iPOS8010 BX-CAT
            case 29025202: // iPOS8010 BA-CAN
            case 29025222: // iPOS8010 BA-CAT
                *peakCurrent = 20.0;
                break;
            case 29026201: // iPOS8020 BX-CAN
            case 29026221: // iPOS8020 BX-CAT
            case 29026202: // iPOS8020 BA-CAN
            case 29026222: // iPOS8020 BA-CAT
                *peakCurrent = 40.0;
                break;
            default:
                return false;
        }

        return true;
    }

    void interpretSupportedDriveModes(std::uint32_t data)
    {
        std::bitset<32> bits(data);

        if (bits.test(0))
        {
            CD_INFO("\t*profiled position (pp)\n");
        }
        if (bits.test(1))
        {
            CD_INFO("\t*velocity (vl)\n");
        }
        if (bits.test(2))
        {
            CD_INFO("\t*profiled velocity (pv)\n");
        }
        if (bits.test(3))
        {
            CD_INFO("\t*profiled torque (tq)\n");
        }
        if (bits.test(5))
        {
            CD_INFO("\t*homing (hm)\n");
        }
        if (bits.test(6))
        {
            CD_INFO("\t*interpolated position (ip)\n");
        }
        if (bits.test(7))
        {
            CD_INFO("\t*cyclic synchronous position\n");
        }
        if (bits.test(8))
        {
            CD_INFO("\t*cyclic synchronous velocity\n");
        }
        if (bits.test(9))
        {
            CD_INFO("\t*cyclic synchronous torque\n");
        }
        if (bits.test(16))
        {
            CD_INFO("\t*electronic camming position (manufacturer specific)\n");
        }
        if (bits.test(17))
        {
            CD_INFO("\t*electronic gearing position (manufacturer specific)\n");
        }
        if (bits.test(18))
        {
            CD_INFO("\t*external reference position (manufacturer specific)\n");
        }
        if (bits.test(19))
        {
            CD_INFO("\t*external reference speed (manufacturer specific)\n");
        }
        if (bits.test(20))
        {
            CD_INFO("\t*external reference torque (manufacturer specific)\n");
        }
    }

    inline char getByte(std::uint32_t number, int n)
    {
        // https://stackoverflow.com/a/7787433
        return (number >> (8 * n)) & 0xFF;
    }
}

// -----------------------------------------------------------------------------

unsigned int TechnosoftIpos::getId()
{
    return can->getId();
}

// -----------------------------------------------------------------------------

std::vector<unsigned int> TechnosoftIpos::getAdditionalIds()
{
    if (iExternalEncoderCanBusSharer)
    {
        return {iExternalEncoderCanBusSharer->getId()};
    }

    return {};
}

// -----------------------------------------------------------------------------

bool TechnosoftIpos::registerSender(CanSenderDelegate * sender)
{
    can->configureSender(sender);
    return iExternalEncoderCanBusSharer && iExternalEncoderCanBusSharer->registerSender(sender);
}

// -----------------------------------------------------------------------------

bool TechnosoftIpos::initialize()
{
    if (iExternalEncoderCanBusSharer && !iExternalEncoderCanBusSharer->initialize())
    {
        CD_ERROR("Unable to initialize external encoder device.\n");
        return false;
    }

    std::uint32_t data;

    if (!can->sdo()->upload("Device type", &data, 0x1000))
    {
        return false;
    }

    CD_INFO("CiA standard: %d.\n", data & 0xFFFF);

    if (!can->sdo()->upload("Supported drive modes", &data, 0x6502))
    {
        return false;
    }

    interpretSupportedDriveModes(data);

    std::string firmware;

    if (!can->sdo()->upload("Manufacturer software version", &firmware, 0x100A))
    {
        return false;
    }

    CD_INFO("Firmware version: %s.\n", firmware.c_str());

    can->sdo()->upload("Identity Object: Vendor ID", &data, 0x1018, 0x01);

    if (!can->sdo()->upload("Identity Object: Product Code", &data, 0x1018, 0x02))
    {
        return false;
    }

    CD_INFO("Retrieved product code: P%03d.%03d.E%03d.\n", data / 1000000, (data / 1000) % 1000, data % 1000);

    if (!retrieveDrivePeakCurrent(data, &vars.drivePeakCurrent))
    {
        CD_ERROR("Unhandled iPOS model %d, unable to retrieve drive peak current.\n", data);
        return false;
    }

    CD_SUCCESS("Retrieved drive peak current: %f A.\n", vars.drivePeakCurrent);

    if (!can->sdo()->upload("Identity Object: Revision number", &data, 0x1018, 0x03))
    {
        return false;
    }

    CD_INFO("Revision number: %c%c%c%c.\n", getByte(data, 3), getByte(data, 2), getByte(data, 1), getByte(data, 0));

    if (!can->sdo()->upload("Identity Object: Serial number", &data, 0x1018, 0x04))
    {
        return false;
    }

    CD_INFO("Serial number: %c%c%02x%02x.\n", getByte(data, 3), getByte(data, 2), getByte(data, 1), getByte(data, 0));

    if (!can->sdo()->download<std::int16_t>("Quick stop option code", 6, 0x605A))
    {
        return false;
    }

    if (!setLimitsRaw(0, vars.min, vars.max))
    {
        CD_ERROR("Unable to set software limits.\n");
        return false;
    }

    if (!setRefSpeedRaw(0, vars.refSpeed))
    {
        CD_ERROR("Unable to set reference speed.\n");
        return false;
    }

    if (!setRefAccelerationRaw(0, vars.refAcceleration))
    {
        CD_ERROR("Unable to set reference acceleration.\n");
        return false;
    }

    if (iEncodersTimedRawExternal)
    {
        // synchronize absolute (master) and relative (slave) encoders
        double extEnc;

        if (!iEncodersTimedRawExternal->getEncodersRaw(&extEnc))
        {
            return false;
        }

        if (!setEncoderRaw(0, extEnc))
        {
            return false;
        }
    }

    if (!can->tpdo1()->configure(vars.tpdo1Conf))
    {
        CD_ERROR("Unable to configure TPDO1.\n");
        return false;
    }

    if (!can->tpdo2()->configure(vars.tpdo2Conf))
    {
        CD_ERROR("Unable to configure TPDO2.\n");
        return false;
    }

    if (!can->tpdo3()->configure(vars.tpdo3Conf))
    {
        CD_ERROR("Unable to configure TPDO3.\n");
        return false;
    }

    vars.actualControlMode = VOCAB_CM_CONFIGURED;

    if (!can->nmt()->issueServiceCommand(NmtService::START_REMOTE_NODE))
    {
        return false;
    }

    if (!can->driveStatus()->requestState(DriveState::SWITCHED_ON))
    {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------

bool TechnosoftIpos::finalize()
{
    bool ok = true;

    if (!can->driveStatus()->requestState(DriveState::SWITCH_ON_DISABLED))
    {
        CD_WARNING("SWITCH_ON_DISABLED transition failed.\n");
        ok = false;
    }
    else
    {
        vars.actualControlMode = VOCAB_CM_CONFIGURED;
    }

    if (!can->nmt()->issueServiceCommand(NmtService::RESET_NODE))
    {
        CD_WARNING("Reset node NMT service failed.\n");
        ok = false;
    }
    else
    {
        vars.actualControlMode = VOCAB_CM_NOT_CONFIGURED;
    }

    return ok;
}

// -----------------------------------------------------------------------------

bool TechnosoftIpos::interpretMessage(const yarp::dev::CanMessage & message)
{
    if (iExternalEncoderCanBusSharer && message.getId() == iExternalEncoderCanBusSharer->getId())
    {
        return iExternalEncoderCanBusSharer->interpretMessage(message);
    }

    if (!can->consumeMessage(message.getId(), message.getData(), message.getLen()))
    {
        CD_ERROR("Unknown message: %s\n", CanUtils::msgToStr(message).c_str());
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
