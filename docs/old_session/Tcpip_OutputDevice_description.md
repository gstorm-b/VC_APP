Vision Output Device:
- Interface mặc định là TCP/IP, vai trò là server
- Gồm 2 port, Port 1 là kênh giao tiếp chính, Port 2 là kênh kiểm tra kết nối
- Sau khi connect Port 2 sẽ gửi một package và chờ client reply với interval 1s.
    - Tin nhắn phía server: "connection_check."
    - Trả lời từ client: "ack,{msg_count}."
        - msg_count: là đếm số tin nhắn với tối đa là 2^16, bắt đầu từ 0, vượt quá giá trị limit sẽ reset về 0
    - Các trường hợp lỗi:
        - Nếu client không phản hồi sau thời gian timeout sẽ báo lỗi lost connect.
        - Nếu sai reply format sẽ báo lỗi lost connect.
- Port 1: là kênh yêu cầu matching và trả kết quả.
    - Format trả kết quả sẽ là: "{detected number},{Position 1},{Position 2}, ... ;"
    - Position sẽ là 4 trục x, y, z, r với format "x,y,z,r"
- Yêu cầu thiết kết module:
    - Kết thừa từ IDevice (src/device/idevice.h)
    - Có config riêng kết thừa từ IDeviceConfig (src/device/idevice_config.h)
    - file source và header đặt trong folder (src/device/output_device)
    - Test chức năng hoạt động sau khi thiết kế để kiểm tra lỗi

Yêu cầu thiết kế cho client
    Giả sử phía client device là Nachi robot controller CFDs series, hãy viết 2 user task có thể connect được với Vision Output device vừa thiết kế
    -	2 User task sẽ được chạy tự động khi khởi động controller
    -	User task 1 connect port 1, user task 2 connect port 2
    -	User task 2 sẽ liên tục kiểm tra kết nối và hỗ trợ kết nối, ngắt kết nối cho user task 1
    -	Sau khi task 2 connect được với server thì task 1 sẽ được phép connect
    -	Task 2 listen timeout thì có nghĩa là lost connect và Task 1 sẽ bị reset để tránh treo, đồng thời báo lỗi connect ra output (IO) của conntroller
    -	Task 1 sau khi nhận được package sẽ parse ra các biến sử dụng cho chương trình khác
    -	Viết thành các chương trình con tách bạch để dễ bảo trình code và debug
    -   File code sau khi thiết kế sẽ được đặt trong folder tests (folder chứa unit test)
