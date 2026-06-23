Đây là một dự án nhỏ dạng backend, thiết kế một module RobotKinematics hỗ trợ tính động học thuận và động học nghịch cho các robot công nghiệp.
Mục đích:
    - Hỗ trợ tính động học thuận, động học nghịch cho robot.
    - Hỗ trợ add frame coordinate để hoạt động trên các coordinates do user define như robot công nghiệp.
    - Hỗ trợ setting tool, tìm ra tọa độ tool từ joint coordinate và ngược lại tìm ra joint từ tool center point.
    - Hoạt động với cấu trúc giống như gazabo sử dụng là URDF.
    - Hỗ trợ preset của các robot phổ biến trên thị trường như dạng robot nối tiếp (6dof), parallel robot (delta), scara,....
    - Có abstract class là RobotKinematic hỗ trợ tính toán, abstract class config.
    - Các kinematic của các robot sẽ có kế thừa riêng của nó.
    - Có khả năng reuse cho nhiều dự án.
Các dự án sẽ áp dụng:
    - Cho ứng dụng computer vision, robot guidance: kiểm tra over limit của joint từ tọa độ mà thuật toán matching cung cấp.
    - Bin picking: tạo motion path picking cho robot.
    - Robot remote control: kiểm tra valid của position khi điều khiển robot.
    - Và nhiều dự án khác.
Build:
    - Dự án không cần UI.
    - Build với c++, eigen.
    - Mình sẽ tạo một project qt6 với qmake config compiler cho dự án.

QA1:
    - User chính của dự án là develop engineer.
    - Scope của dự án có thể mở rộng tùy theo tình hình lên ý tưởng.
    - Độ chính xác bắt buộc phải ở mức 0.001 mm.
    - Mục tiêu gần nhất sẽ là hoàn thiện cho robot 6DOF, hai preset đầu tiên sẽ là cho robot kawasaki rs007n và nachi mz04d.
    - Ở giai đoạn này mình nghĩ nên thiết kết cấu trúc của thư viện theo kiểu có thể mở rộng và kế thừa cho robot parallel, scara và các loại 4dof, 5dof sau này.
    - Kết quả dự án này sẽ là một thư viện có khả năng mở rộng và dễ maintain.
    - Một tính năng cũng phải bắt buộc trong giai đoạn này là hỗ trợ custom presset cho các robot cùng dạng, có thể thay đổi cấu trúc link, link parameters, joint limits,....

QA2:
    - Mình xác nhận độ chính xác mà mình nói đến là joint -> FK -> pose -> IK -> joint, không phải là độ chính xác thực tế của robot, đây chỉ là độ chính xác tính toán của thư viện.
    - Mình muốn làm rõ thêm, config của robot 6dof và các dạng nối tiếp phải có hỗ trợ config arm posture: lefty, righty, below, above, flip, non-flip,... và có thể điều chỉnh các điều kiện config này theo từng loại robot cụ thể.

QA3:
    - sẽ có nhiều api hỗ trợ trả về các loại kết quả khác nhau:
        - solve: tự động chọn best theo seed/posture/options theo arg cung cấp. api này cũng hỗ trợ solve dựa vào tư thế trước đó (được input vào solve từ arg).
        - solveAll: trả về tất cả nghiệm tìm được.
