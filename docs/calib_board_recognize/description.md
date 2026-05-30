Hãy thiết kết một module calibration dùng cho computer vision:
- Class CalibrationBoard:
    - Có thể tạo calibration board image có dạng tương tự calibration board của hãng Fanuc.
    - Có thể recognize và cung cấp cv::Mat dùng cho các method calibration của opencv
- Class Calibrator:
    - Có thể calibration, chuyển đổi giữa hệ tọa độ 2D (x,y) của ảnh và hệ tọa độ 3D(x,y,z) của robot
    - Có thể dùng ma trận recognize được từ CalibrationBoard để calib
- Viết unit test cho thiết kế trên
- Viết function mẫu trong main() để có thể monitor kết quả

<!-- Confirm points:
1. Hiện tại mình chỉ dùng dạng planar cho 2D pick and place chưa cần 3D picking.
2. Dùng QT test, bạn hãy viết unit test mình sẽ tự build và phản hồi lại kết quả cho bạn.
3. Cần tạo pattern image như của fanuc vì dự định sẽ dùng board fanuc thật.
    - Hãy thêm các paramters để cấu hình board như:
        - size ma trận: số hàng, số cột.
        - size dot
        - khoảng cách dot
        - ...
    - size dot và khoảng cách có unit là mm -->

Grid pattern format:
- Số cột và hàng luôn là số lẻ
- Dot target: 
    - Là các dot có outer màu đen, inner là dot màu trắng
    - Có size outer giống với các dot bình thường
    - Gồm 8 dot target, 4 dot ở góc, 4 dot là trung tâm cạnh rìa của grid
- Dot trung tâm:
    - Là dot màu đen outer, inner là dot màu trắng
    - Có size outer to hơn bình thường dùng để định vị tâm của grid
    - Có size inner cùng size với size inner của dot target
- Dot tọa độ:
    - Là dot màu đen, không có inner
    - Có size to hơn size của dot bình thường, cùng size với dot trung tâm
    - gồm 3 dot: 2 dot cùng hàng phía bên phải của dot trung tâm, 1 dot cùng cột phía trên của dot trung tâm
    - 2 dot bên phải định hướng cho trục x trong thực tế
    - 1 dot bên trên định hướng cho trục y trong thực tế
- Tất cả các parameter đều có thể config được
- như vậy dot size params sẽ gồm: normalDotSize (size của dot thường và là size outer của dot target), coorDotSize(size outer của dot trung tâm và là size của dot tọa độ), innerTargetDotSize(là size inner của dot target và là size inner của dot trung tâm)
- mặc định size inner sẽ thường là 1.0 mm

Calibration board type develop:
- Hãy refactor abstract class cho CalibrationBoard, vì mình muốn recognize thêm nhiều loại board, hiện tại inherentance duy nhất chỉ có iRvision calibrate grid type.
- Hãy viết luôn factory class để cấp phát calibration board instance

Các tham số cần biến đổi:
- method: imageToRobot phải biến đổi được từ hệ tọa độ ảnh x,y sang hệ tọa độ robot gồm x, y, z
- method: robotToImage phải biến đổi được từ hệ tọa độ robot x,y,z sang hệ tọa độ ảnh x, y
- method: rotateImageToRobot phải biến đổi được góc từ hệ tọa độ trên ảnh (góc thuận tính theo counter clockwise) sang góc r của hệ tọa độ robot
- method: rotateRobotToImage phải làm được điều ngược lại với rotateImageToRobot

Confirm points:
1. Z trên ảnh luôn là 0, vì có thể trong thực tế mặt phẳng picking sẽ nghiên một chút nên cần phải chuyển được z
2. Khi convert robot to image thì bỏ qua z
3. đơn vị angles mặc định là degree, có arg optional để chuyển qua radian
4. Code phải tự tìm được rotation angle giữa hai hệ tọa độ vì đã có thông tin hướng của hệ tọa đọ robot trên pattern, và cả vị trí của 4 target point
5. hãy chọn giải pháp có độ chính xác cao nhất
6. clean break

Robot test point: (x, y, z)
- 1. Top left: -151.910, 713.404, 130.693
- 2. Top right: -1.753, 705,398, 131.682
- 3. Bottom left: -159.925, 563.285, 131.186
- 4. Bottom right: -9.918, 555.786, 132.182 