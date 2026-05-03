this is my prompt notes, claude don't need to read this file

Mô tả Project Tree Widget:
- Trong giai đoạn này Project Tree Widget sẽ chỉ cho phép mở 1 project duy nhất, trong tree widget nó sẽ là top-level item, trong tương lai có thể sẽ cho phép config nhiều project cùng lúc
- Sub item của top Project sẽ là các Task
- Sub item của các task sẽ là các Device

Mình đã có được design handoff trong folder ui_scratch, hãy refactor lại cơ chế UI như file thiết kế:
- Về Project Tree Widget:
    - Trong giai đoạn này Project Tree Widget sẽ chỉ cho phép mở 1 project duy nhất, trong tree widget nó sẽ là top-level item, trong tương lai có thể sẽ cho phép config nhiều project cùng lúc
    - Sub item của top Project sẽ là các Task
    - Sub item của các task sẽ là các Device 
    - Device có thể được di chuyển quyền sở hữu giữa các task, bỏ cơ chế own-shared cũ.
- Phiên bản hiện tại chỉ cho phép tạo localization task
- Cơ chế dock vẫn sử dụng advance docking như cũ
- Giữ nguyên cơ chế Pattern matching vẫn như cũ.
- Cơ chế theme vẫn như cũ, mặc định sẽ được chọn dark, light, sử dụng qss



Mình có một project computer vision, module PLC là một phần trong project:
- Được dùng chính để monitor dữ liệu và output.
- Mục tiêu dùng cho nhiều dòng PLC, điển hình là PLC Mitsubishi qua MC Protocol
- Hãy thiết kế cấu trúc abstract, interface cho module PLC,
- Phải có interface dùng để đọc dữ liệu, observer sự thay đổi dữ liệu (các dữ liệu thay đổi sẽ được đăng kí trước)
- Phải có interface để output dữ liệu
- Có interface để update dữ liệu cho tính năng monitoring trên màn hình
- Dùng cơ chế slot và signal của QT Framework

Pattern setting:
- Step 1: Set image (From camera or choose an image)
- Step 2: Crop image (or continue without crop)
- Step 3: Set Picking condition: Picking box (gripper shape: 2 synmetric rectangle), picking position
- Step 4: Show thumb image with picking position, select finish to done