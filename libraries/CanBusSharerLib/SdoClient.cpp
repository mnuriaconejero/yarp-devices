// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "SdoClient.hpp"

#include <cmath>
#include <cstring>

#include <bitset>
#include <string>

#include <ColorDebug.h>

#include "CanUtils.hpp"

using namespace roboticslab;

namespace
{
    std::string parseAbortCode(uint32_t code)
    {
        // CiA 301 v4.2.0
        switch (code)
        {
        case 0x05030000:
            return "Toggle bit not alternated";
        case 0x05040000:
            return "SDO protocol timed out";
        case 0x05040001:
            return "Client/server command specifier not valid or unknown";
        case 0x05040002:
            return "Invalid block size (block mode only)";
        case 0x05040003:
            return "Invalid sequence number (block mode only)";
        case 0x05040004:
            return "CRC error (block mode only)";
        case 0x05040005:
            return "Out of memory";
        case 0x06010000:
            return "Unsupported access to an object";
        case 0x06010001:
            return "Attempt to read a write only object";
        case 0x06010002:
            return "Attempt to write a read only object";
        case 0x06020000:
            return "Object does not exist in the object dictionary";
        case 0x06040041:
            return "Object cannot be mapped to the PDO";
        case 0x06040042:
            return "The number and length of the objects to be mapped would exceed PDO length";
        case 0x06040043:
            return "General parameter incompatibility reason";
        case 0x06040047:
            return "General internal incompatibility in the device";
        case 0x06060000:
            return "Access failed due to an hardware error";
        case 0x06070010:
            return "Data type does not match, length of service parameter does not match";
        case 0x06070012:
            return "Data type does not match, length of service parameter too high";
        case 0x06070013:
            return "Data type does not match, length of service parameter too low";
        case 0x06090011:
            return "Sub-index does not exist";
        case 0x06090030:
            return "Invalid value for parameter (download only)";
        case 0x06090031:
            return "Value of parameter written too high (download only)";
        case 0x06090032:
            return "Value of parameter written too low (download only)";
        case 0x06090036:
            return "Maximum value is less than minimum value";
        case 0x060A0023:
            return "Resource not available: SDO connection";
        case 0x08000000:
            return "General error";
        case 0x08000020:
            return "Data cannot be transferred or stored to the application";
        case 0x08000021:
            return "Data cannot be transferred or stored to the application because of local control";
        case 0x08000022:
            return "Data cannot be transferred or stored to the application because of the present device state";
        case 0x08000023:
            return "Object dictionary dynamic generation fails or no object dictionary is present (e.g. object "
                   "dictionary is generated from file and generation fails because of an file error)";
        case 0x08000024:
            return "No data available";
        default:
            return "unknown";
        }
    }
}

bool SdoClient::send(const uint8_t * msg)
{
    return sender->prepareMessage(message_builder(COB_D + id, 8, msg));
}

std::string SdoClient::msgToStr(uint16_t cob, const uint8_t * msgData)
{
    return CanUtils::msgToStr(id, cob, 8, msgData);
}

bool SdoClient::uploadInternal(const std::string & name, void * data, size_t size, uint16_t index, uint8_t subindex)
{
    uint8_t requestMsg[8] = {0};

    requestMsg[0] = 0x40; // client command specifier
    std::memcpy(requestMsg + 1, &index, 2);
    requestMsg[3] = subindex;

    uint8_t responseMsg[8];

    if (!performTransfer(name, requestMsg, responseMsg))
    {
        return false;
    }

    std::bitset<8> bitsReceived(responseMsg[0]);

    if (bitsReceived.test(1)) // expedited trasfer
    {
        if (bitsReceived.test(0)) // data size is indicated in 'n'
        {
            const size_t n = ((bitsReceived << 4) >> 6).to_ulong();
            const size_t actualSize = 4 - n;

            if (size != actualSize)
            {
                CD_ERROR("SDO response size mismatch: expected %zu, got %zu.\n", size, actualSize);
                return false;
            }
        }

        std::memcpy(data, responseMsg + 4, size);
    }
    else
    {
        uint32_t len;
        std::memcpy(&len, responseMsg + 4, sizeof(len));

        if (size < len)
        {
            CD_ERROR("Unsufficient memory allocated for segmented SDO transfer: expected %zu, got %zu.\n", len, size);
            return false;
        }

        CD_INFO("SDO segmented transfer begin: id %d.\n", id);

        std::bitset<8> bitsSent(0x60);
        uint8_t segmentedMsg[8] = {0};
        size_t sent = 0;

        do
        {
            segmentedMsg[0] = bitsSent.to_ulong();

            if (!performTransfer(name, segmentedMsg, responseMsg))
            {
                return false;
            }

            bitsReceived = std::bitset<8>(responseMsg[0]);

            if (!bitsReceived.test(4) != bitsSent.test(4))
            {
                CD_ERROR("SDO segmented transfer: toggle bit mismatch.\n");
                return false;
            }

            const size_t n = ((bitsReceived << 4) >> 5).to_ulong();
            const size_t actualSize = 7 - n;

            std::memcpy(static_cast<uint8_t *>(data) + sent, responseMsg + 1, actualSize);

            sent += actualSize;
            bitsSent.flip(4);
        }
        while (!bitsReceived.test(0)); // continuation bit

        CD_INFO("SDO segmented transfer finish: id %d.\n", id);
    }

    return true;
}

bool SdoClient::downloadInternal(const std::string & name, const void * data, size_t size, uint16_t index, uint8_t subindex)
{
    uint8_t indicationMsg[8] = {0};

    indicationMsg[0] = 0x23 + ((4 - size) << 2); // client command specifier
    std::memcpy(indicationMsg + 1, &index, 2);
    indicationMsg[3] = subindex;
    std::memcpy(indicationMsg + 4, &data, size);

    uint8_t confirmMsg[8];
    return performTransfer(name, indicationMsg, confirmMsg);
}

bool SdoClient::performTransfer(const std::string & name, const uint8_t * req, uint8_t * resp)
{
    const std::string & reqStr = msgToStr(COB_D, req);

    if (!send(req))
    {
        CD_ERROR("SDO client request/indication (\"%s\"). %s\n", name.c_str(), reqStr.c_str());
        return false;
    }

    CD_INFO("SDO client request/indication (\"%s\"). %s\n", name.c_str(), reqStr.c_str());

    bool success = sdoSemaphore->await(resp);
    const std::string & respStr = msgToStr(COB_U, resp);

    if (!success)
    {
        CD_ERROR("SDO client response/confirm (\"%s\"). %s\n", name.c_str(), respStr.c_str());
        return false;
    }

    if (resp[0] == 0x80) // SDO abort transfer (ccs)
    {
        uint32_t code;
        std::memcpy(&code, resp + 4, sizeof(code));
        CD_ERROR("SDO transfer abort (\"%s\"): %s. %s\n", name.c_str(), parseAbortCode(code).c_str(), respStr.c_str());
        return false;
    }

    CD_SUCCESS("SDO client response/confirm (\"%s\"). %s\n", name.c_str(), respStr.c_str());
    return true;
}
