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
            - VisionTCPIP (port from current VisionOutput)
            - VisionSerial (future update)
        - Robot:
            - Kawasaki (future update)
            - Huayan (future update)
            - Nachi (future update)


temp_notes:
- handle signal when PLC ID changed, Output Vision changed -> re-setup task (wire signal, start runnner,... base on task phase)