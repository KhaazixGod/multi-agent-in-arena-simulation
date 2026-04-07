# Hướng dẫn đưa thiết kế STL vào ROS 2 và hiển thị trên RViz

Tài liệu này bao gồm 2 phần: **Phần 1** Hướng dẫn thủ công xuất file STL chuyển qua URDF (hiển thị mô hình tĩnh) và **Phần 2** Cách xuất toàn bộ cụm lắp ráp động từ Onshape tự động vào thẳng ROS 2.

---

## PHẦN 1: TẠO URDF THỦ CÔNG TỪ FILE STL 

Khi bạn lắp ráp xong (Assembly) trên phần mềm CAD và xuất *toàn bộ dự án* ra các file con `.stl`, mỗi phần tử thực ra đã chứa một điểm mốc không gian bên trong file. Bạn chỉ việc quy định mã nguồn lắp chúng trùng lại lên một tọa độ (0, 0, 0) là xe sẽ tự động hình thành. 

### 1. Cấu trúc thư mục tiêu chuẩn
```text
my_robot_description/
├── CMakeLists.txt
├── package.xml
├── launch/
│   └── display.launch.py
├── meshes/
│   ├── base_link.stl
│   ├── wheel.stl
│   └── ...
├── rviz/
│   └── my_robot.rviz
└── urdf/
    └── my_robot.urdf
```

### 2. Cấu trúc một file URDF chuẩn
File URDF (Unified Robot Description Format) sử dụng ngôn ngữ dữ liệu XML. Đây là các phần quan trọng nhất tạo nên một chiếc xe:

#### a. Tọa độ gốc (Footprint) và Khung chính
```xml
<?xml version="1.0"?>
<robot name="car_robot">

  <!-- ================= TỌA ĐỘ CHUẨN MẶT ĐẤT ================= -->
  <!-- Đây là 'cái bóng' của xe in trên bản đồ RViz, luôn nằm dưới đất -->
  <link name="base_footprint"/>

  <!-- KHỚP NỐI GỐC (JOINT): Xoay trục xe nếu xe bị lấp/dựng ngược -->
  <joint name="base_joint" type="fixed">
    <parent link="base_footprint"/>
    <child link="base_link"/>
    <!-- Quan trọng: Hệ tọa độ Onshape (Y hướng lên) khác với ROS (Z hướng lên). 
         Lệnh rpy="0 -1.5708 0" giúp lật xe về bình thường. (1.5708 rad = 90 độ) -->
    <origin xyz="0 0 0" rpy="0 -1.5708 0"/>
  </joint>

  <!-- =============================== KHUNG XE CHÍNH =============================== -->
  <link name="base_link">
    <!-- Thẻ visual quy định hiển thị 3D cho RViz -->
    <visual>
      <origin xyz="0 0 0" rpy="0 0 0"/>
      <geometry>
        <!-- Thẻ scale="0.001 0.001 0.001": Chuyển đổi bản vẽ milimet sang mét quy chuẩn -->
        <mesh filename="package://car_desciptionns/meshes/base_link.stl" scale="0.001 0.001 0.001"/>
      </geometry>
      <!-- Định nghĩa chất liệu, rgba dùng kênh màu % -->
      <material name="gray"><color rgba="0.5 0.5 0.5 1"/></material>
    </visual>
  </link>
```

#### b. Lắp ráp các chi tiết linh kiện
Vì các file `stl` đã xuất từ 1 điểm Assemble chung, chúng ta cho hết chúng vào tọa độ gốc của khung xe, hệ thống sẽ tự đặt chúng vào đúng vị trí thiết kế.
```xml
  <!-- Quạt / Fan -->
  <!-- Thẻ Joint "fixed" nghĩa là quạt hàn chết vào khung -->
  <joint name="fan_joint" type="fixed">
    <parent link="base_link"/>
    <child link="fan_link"/>
    <!--xyz=0 0 0 không thêm số đo nào, để hệ thống dóng tọa độ CAD tự động -->
    <origin xyz="0 0 0" rpy="0 0 0"/> 
  </joint>
  <!-- Mô tả hình hải của Quạt -->
  <link name="fan_link">
    <visual>
      <geometry>
        <mesh filename="package://car_desciptionns/meshes/fan.stl" scale="0.001 0.001 0.001"/>
      </geometry>
      <material name="blue"><color rgba="0.0 0.0 0.8 1"/></material>
    </visual>
  </link>

</robot>
```

---

## PHẦN 2: XUẤT ĐỘNG HỌC TỪ ONSHAPE VÀO ROS (ONSHAPE-TO-ROBOT)

Cách làm từ Phần 1 là ghép file thủ công. RViz chỉ nhìn thấy đây là 1 hình đồ họa khô cứng (không cài quay bánh xe, kịch bản va chạm vật lý Gazebo).
Để làm ra ROS robot tiêu chuẩn cực nhanh, chạy vật lý thực, cộng đồng ưu tiên sử dụng `onshape-to-robot`.

### 1. Chuẩn bị bản vẽ Onshape
1. Bạn phải gán khớp chuyển động cho các bánh xe bằng lệnh **Revolute Joint** trong thẻ Assembly của phần mềm Onshape.
2. Tại Onshape, gán **Material** cho các chi tiết (Nhôm, Gỗ, Nhựa...). Onshape-to-robot sẽ tính diện tích * tỷ trọng để xuất ra số Kí-lô-gam thật cho con xe trên ROS.
3. Nếu bạn muốn đặt tên bộ phận, đổi tên trong panel của Onshape.
4. Chia sẻ thiết kế đó bằng tuỳ chọn **Link sharing** sang chế độ Public.

### 2. Cài đặt Onshape-to-robot trên Ubuntu
Mở Terminal lên và cài đặt:

```bash
sudo apt install python3-pip
pip install onshape-to-robot
```

### 3. Khai báo API Key Onshape
Cấp phép cho thư viện can thiệp vào trang web Onshape qua tài khoản của bạn:
1. Đăng nhập https://dev-portal.onshape.com/
2. Tạo **API key** với quyền read (đọc). Bạn sẽ nhận được 2 dãy mã gọi là `Access key` và `Secret key`.
3. Lưu 2 dãy này bằng lệnh:
```bash
export ONSHAPE_API=https://cad.onshape.com
export ONSHAPE_ACCESS_KEY=dãy_chữ_kí_tự_1
export ONSHAPE_SECRET_KEY=dãy_chữ_kí_tự_2
```

### 4. Tự động chuyển đổi
1. Lấy dãy ID thư mục trên link url gốc (ví dụ url là: `cad.onshape.com/documents/b732.../w/a3.../e/080c...`). Bạn copy lưu lại.
2. Tạo 1 thư mục riêng (`my_new_bot`) trên Ubuntu và tạo file `config.json` như sau:
```json
{
    "documentId": "b732...",
    "versionId": "",
    "workspaceId": "a3...",
    "elementId": "080c...",
    "outputFormat": "urdf"
}
```
3. Chạy lệnh:
```bash
onshape-to-robot my_new_bot
```
**Kết quả**: Chương trình sẽ mất vài phút tải mô hình từ web, tự động vẽ URDF có sẵn khối lượng (`<inertial>`), sẵn bánh quay, sẵn mọi thứ (nếu Onshape của bạn làm chuẩn). Thư mục này chỉ việc Build và ngắm nghía!
