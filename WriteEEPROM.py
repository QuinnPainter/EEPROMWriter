# Requirements: Python 3, PySerial
# Arg 1 = COM port of EEPROM Arduino (usually COM3)
# Arg 2 = R, W, V (read, write or verify)
# Arg 3 = Name of file to read from or write to

# todo: "verify" function to compare EEPROM with file

import sys
from time import sleep
import serial

if (len(sys.argv) < 4):
    print("Not enough arguments!")
    print("Arg 1 = COM port of EEPROM Arduino (usually COM3)")
    print("Arg 2 = R, W, V (read, write or verify)")
    print("Arg 3 = Name of file to read from or write to")
    sys.exit()
    
if (sys.argv[2] not in ["R", "W", "V"]):
    print ("Invalid second argument!")
    print ("Must be R, W or V")
    sys.exit()
    
try:
    ser = serial.Serial(sys.argv[1], 38400, timeout=1)
except serial.serialutil.SerialException as e:
    print("Could not open serial port " + sys.argv[1])
    print(e)
    sys.exit()

sleep(2) # Arduino resets when serial connection is opened, so we need to wait for it    

ser.write(b'P')
if not (ser.read() == b'P'):
    print("Arduino not detected on " + sys.argv[1])
    ser.close()
    sys.exit()
    
print("Arduino found!")
if (sys.argv[2] == 'R'):
    ser.write(b'R')
    f = open(sys.argv[3], 'wb+')
    print("Reading to " + sys.argv[3])
    for i in range(0, 64):
        f.write(ser.read(256))
        print('\r' + str((i / 64) * 100) + "%       ", flush=True, end='')
    f.close()
    print("\r100%     ") # newline
elif (sys.argv[2] == 'W'):
    try:
        f = open(sys.argv[3], 'rb')
    except FileNotFoundError as e:
        print (e)
        ser.close()
        sys.exit()
    fileData = f.read()
    f.close()
    if (len(fileData) != 16384):
        print("File is wrong size! (not 16K)")
        ser.close()
        sys.exit()
    print ("Writing " + sys.argv[3])
    fileSplit = [fileData[i:i+256] for i in range(0, len(fileData), 256)]
    ser.write(b'W')
    prevPercentage = 0
    for i in range(0, len(fileSplit)):
        ser.write(fileSplit[i])
        percentage = (i / len(fileSplit)) * 100
        while ser.in_waiting < 1:
            if not prevPercentage == percentage:
                print('\r' + str((i / len(fileSplit)) * 100) + "%       ", flush=True, end='')
                prevPercentage = percentage
        ser.read()
    print("\r100%     ") # newline
elif (sys.argv[2] == 'V'):
    try:
        f = open(sys.argv[3], 'rb')
    except FileNotFoundError as e:
        print (e)
        ser.close()
        sys.exit()
    fileData = f.read()
    f.close()
    ser.write(b'R')
    print("Verifying with " + sys.argv[3])
    goodBytes = 0
    badBytes = 0
    for i in range(0, 64):
        chunkRead = ser.read(256)
        for x in range(0, 256):
            if (chunkRead[x] == fileData[(256 * i) + x]):
                goodBytes += 1
            else:
                badBytes += 1
        print('\r' + str((i / 64) * 100) + "%       ", flush=True, end='')
    print("\r100%     ") # newline
    print("Bytes correct: " + str(goodBytes))
    print("Bytes incorrect: " + str(badBytes))
    print("Percentage correct: " + str((goodBytes / (goodBytes + badBytes)) * 100))
    
print ("Done!")
ser.close()