Cấu trúc device type hiện tại:
    Device type:
        - UserType
        - Camera
            - Realsense
            - BaslerGige
            - BaslerUsb
        - McDevice
        - PLC
        - VisionOutput
        - Robot

Cấu trúc device type muốn thay đổi:
    Device type:
        - UserType
        - Camera (not change)
            - Realsense (future update)
            - BaslerGige
            - BaslerUsb (future update)
        - PLC
            - McDevice (current McDevice at top level)
        - VisionOutput:
            - VisionTCPIP (TCP/IP server — software listens, done)
            - VisionTcpipClient (TCP/IP client — software dials out + reconnect, done)
            - VisionSerial (future update)
        - Robot:
            - Kawasaki (future update)
            - Huayan (future update)
            - Nachi (future update)


temp_notes:
- handle signal when PLC ID changed, Output Vision changed -> re-setup task (wire signal, start runnner,... base on task phase)