Mô tả dự án:
- Xây dựng một ứng dụng chính dùng để config các project, một ứng dụng phụ chỉ dùng để chạy runtime của các dự án đã config trong nhà máy.
- Đây là ứng dụng theo hướng thương mại.
- Ứng dụng được dùng cho công nghiệp, cần độ ổn định và latency thấp.
- Ứng dụng tập trung vào computer vision để giải quyết các về đề như pick and place (với scan area camera - 2d).
- Các loại task dự kiến:
    - Localization: định vị object và xuất tọa độ cho robot picking.
    - Pick and place (phiên bản tương lai): định vị object và motion control dẫn hướng robot picking.
    - Inspection (phiên bản tương lai): hỗ trợ các tool object detection,..... như các ứng dụng computer vision thường có.
- Khi release version đầu tiên:
    - Phải xây dựng được cấu trúc của hệ thống, tính reuse cao.
    - Các loại device hỗ trợ: Cam (Camera balser), PLC (MC Protocol), VisionOutput (Hỗ trợ interface TCP/IP ở cả mode master và slave)
    - Task sẽ được release ở bản đầu tiên là loccalization.

Mục tiêu refactor toàn bộ dự án:
- Thiết lập cấu trúc tổ chức file cho dự án.
    - Nhờ teach lead tư vấn với cấu trúc hiện tại.
    - Chia các component rõ ràng hơn.
    - Mình nghĩ cần cố sự phân biệt giữ widget và các form.
- Thiết lập các quy tắc thiết kế code.
    - Tránh tối đa việc hạn chế hardcore các hằng số, nếu bắt buộc phải dùng hằng số thì phải khai báo trong một file header riêng để dễ kiểm soát.
- Đánh giá lại trúc tổ chức của hệ thống: Project, task, device, runtime.
- Thiết lập các quy tắc docs, api reference, uml diagram, mô tả cho từng module.
- Loại bỏ đi các các source code lặp đi lặp lại, hoặc trùng lập chức năng.
- Thiết lập các rule chung cho thiết kế UI, thiết kế style sheet.
    - Khi thiết kế widget ưu tiên mức độ reuse cao.
    - Khi dùng stylesheet thì không được hardcore trong file source và header, phải đọc từ file qss của project src.
    - Khi cần sử dụng các widget có sẵn của QT Framework như QPushButton, QLineEdit,... nhưng cần thiết kế style riêng thì phải dùng một class kế thừa. Làm như vậy để thống nhất style, tránh hardcore style trong qss ở nhiều nơi.
- Xây dựng các file md, hướng dẫn các agents và developer implement khi tham gia vào dự án.
