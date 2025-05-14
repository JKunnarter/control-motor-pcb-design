# THIẾT KẾ MẠCH ĐIỀU KHIỂN ĐỘNG CƠ

## A. MÔ TẢ

Project này yêu cầu thiết kế mạch PCB (mạch in) với chức năng điều khiển tốc độ động cơ, hiển thị tốc độ trên màn hình LCD 16x2.

## B. TRIỂN KHAI
### 0. Ý tưởng
- Sử dụng các bộ timer trên MCU (vi điều khiển) để phát xung PWM điều khiển động cơ.
- Sử dụng ngắt/timer để thiết lập trạng thái bật/tắt động cơ.
- Sử dụng capture để đếm xung từ encoder đo vận tốc góc.
- Hiển thị các thông số Duty (~Tốc độ), RPM (Vòng/phút) ra màn hình LCD 16x2 bằng chế độ 4 bit.
### 1. Chọn phần cứng
- Với yêu cầu và ý tưởng trên chọn MCU là Atmega8 kết hợp sử dụng thạch anh ngoại 16MHz.
- Động cơ chọn động cơ 6V DC Yellow, sử dụng driver L298n bên ngoài mạch.
- Encoder, đĩa tròn đục 20 lỗ.
- Màn hình chọn LCD 16x2.
- Nút bấm, chiết áp, led để điều khiển và hiển thị trạng thái.
### 2. Mô phỏng mạch trên Proteus
### 3. Thiết kế mạch trên Altium
### 4. Gia công mạch in 
### 5. Kiểm tra Project
## C. TỔNG KẾT
