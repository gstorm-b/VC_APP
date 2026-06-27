Yêu cầu thiết kế calibration tables point:
- Là table dùng để hiển thị, và chỉnh sửa thông tin calibration.
- Mỗi row là một row, một row có 1 cặp điểm.
- 1 cặp điểm gồm 2D image point (gồm X, y), 3D world point (gồm X, Y, Z).
- Thứ tự cột: point number, image point x, image point y, world point x, world point y, world point z
- Cho phép chỉ cho phép nhập tay các cột của 3D world point, mỗi khi có sự kiện edit, sẽ có signal báo đã edit. (wire signal này với một method trong widget camera device, enable button apply, thay đổi trạng thái của calibration status label thêm vào content "(point changed)" ở cuối.)
- Có method chuẩn bị sẵn cho việc input trực tiếp 3D world point từ module Robot (hiện tại chưa dùng).
- các cột của 2D image point sẽ được input bởi calibration board sau khi detect.
- Ẩn đi verical header.
