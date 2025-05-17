# THIẾT KẾ MẠCH ĐIỀU KHIỂN ĐỘNG CƠ

## A. MÔ TẢ

Project này yêu cầu thiết kế mạch PCB (mạch in) với chức năng điều khiển tốc độ động cơ, hiển thị tốc độ trên màn hình LCD 16x2.

![Preview](Altium%20Design/Preview3D.png)

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
#### Atmega8
#### L298n
#### Encoder 
#### LCD 16x2
### 2. Mô phỏng mạch trên Proteus

![Schematic](https://cdn.mathpix.com/cropped/2025_05_17_536ac820676dc0d79ce8g-1.jpg?height=1772&width=2523&top_left_y=0&top_left_x=1)

#### Project gồm các khối chính như sau: 
+ Khối nguồn: Chịu trách nhiệm cấp nguồn cho toàn bộ hệ thống.
+ Mạch cầu H (L298n): Dùng để điều khiển động cơ.
+ Khối MCU: Vi điêu khiển của hệ thống, chịu trách nhiệm xử lý tín hiệu, điều khiển.
+ Các phím bấm, led hiển thị: Điều khiển và hiển thị trạng thái.
+ Khối hiển thị: LCD 16x2 hiển thị các thông số.

#### Demo
[![Demo](Proteus%20Design/Demo%20Simulation.mp4)]([https://youtu.be/T-D1KVIuvjA](https://drive.google.com/file/d/1lh7Z6Pq6m5kVl-becjptn-Ts8dSg2Dhr/view?usp=drive_link))

### 3. Thiết kế mạch trên Altium
![Schematic](https://cdn.mathpix.com/cropped/2025_05_17_43383524a265fd68bf44g-1.jpg?height=2106&width=2660&top_left_y=10&top_left_x=45)

### 4. Gia công mạch in 
### 5. Kiểm tra Project
## C. TỔNG KẾT
