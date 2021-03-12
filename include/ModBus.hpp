    #include <Arduino.h>
    
    void GetCRC(byte *message, int length, byte *CRC)
    {
        //Function expects a modbus message of any length as well as a 2 byte CRC array in which to 
        //return the CRC values:

        ushort CRCFull = 0xFFFF;
        byte CRCHigh = 0xFF, CRCLow = 0xFF;
        char CRCLSB;

        for (int i = 0; i < length - 2; i++)
        {
            CRCFull = (ushort)(CRCFull ^ message[i]);

            for (int j = 0; j < 8; j++)
            {
                CRCLSB = (char)(CRCFull & 0x0001);
                CRCFull = (ushort)((CRCFull >> 1) & 0x7FFF);

                if (CRCLSB == 1)
                    CRCFull = (ushort)(CRCFull ^ 0xA001);
            }
        }
        CRC[1] = CRCHigh = (byte)((CRCFull >> 8) & 0xFF);
        CRC[0] = CRCLow = (byte)(CRCFull & 0xFF);
    }

    void BuildMessage(byte address, byte type, ushort start, ushort registers, byte *message, int length)
    {
        //Array to receive CRC bytes:
        byte *CRC = new byte[2];

        message[0] = address;
        message[1] = type;
        message[2] = (byte)(start >> 8);
        message[3] = (byte)start;
        message[4] = (byte)(registers >> 8);
        message[5] = (byte)registers;

        GetCRC(message, length, CRC);
        message[length - 2] = CRC[0];
        message[length - 1] = CRC[1];
    }

    bool CheckResponse(byte *response, int length)
    {
        //Perform a basic CRC check:
        byte *CRC = new byte[2];
        GetCRC(response, length, CRC);
        if (CRC[0] == response[length - 2] && CRC[1] == response[length - 1])
            return true;
        else
            return false;
    }

    void GetResponse(byte *response, int length)
    {
        //There is a bug in .Net 2.0 DataReceived Event that prevents people from using this
        //event as an interrupt to handle data (it doesn't fire all of the time).  Therefore
        //we have to use the ReadByte command for a fixed length as it's been shown to be reliable.
        for (int i = 0; i < length; i++)
        {
            response[i] = (byte)(Serial2.read());
        }
    }

    bool SendFc3(byte address, ushort start, ushort registers, short *values)
    {
        const int messageLen = 8;
        //Function 3 request is always 8 bytes:
        byte *message = new byte[messageLen];
        //Function 3 response buffer:
        auto responseLen = 5 + 2 * registers;
        byte *response = new byte[responseLen];
        //Build outgoing modbus message:
        BuildMessage(address, (byte)3, start, registers, message, messageLen);
        //Send modbus message to Serial Port:

        Serial2.write(message, messageLen);
        GetResponse(response, responseLen);

        Serial.println("got response");
        //Evaluate message:
        if (CheckResponse(response, responseLen))
        {
            //Return requested register values:
            for (int i = 0; i < (responseLen - 5) / 2; i++)
            {
                values[i] = response[2 * i + 3];
                values[i] <<= 8;
                values[i] += response[2 * i + 4];
            }

            return true;
        }
        else
        {
            Serial.println("failed CRC");
            return false;
        }
    }