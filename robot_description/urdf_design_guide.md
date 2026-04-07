# Cẩm nang Thiết kế và Xây dựng file URDF cho ROS 2

URDF (Unified Robot Description Format) là định dạng file XML tiêu chuẩn trong ROS để mô tả cấu trúc vật lý của robot, bao gồm hình dáng (visual), đặc tính vật lý (collision, inertia) và cách các bộ phận chuyển động tương đối với nhau (joints).

Do bạn gặp nhiều lỗi trong quá trình tự trích xuất và lắp ghép, tài liệu này sẽ cung cấp nguyên tắc cốt lõi để bạn không bao giờ gắn sai trục nữa.

---

## 1. Nguyên Tắc Thiết Kế Cốt Lõi của URDF

### 1.1 Khái Niệm Link và Joint
Mọi file URDF đều được xây dựng từ 2 viên gạch cơ bản:
- **`Link` (Thực thể rắn):** Bánh xe, khung gầm, cánh tay, mắt camera,... Mỗi link chỉ được coi là 1 bộ phận cứng không thể tự uốn cong.
- **`Joint` (Khớp nối):** Thứ kết nối 2 Link lại với nhau. Giống như khuỷu tay nối cẳng tay với bắp tay. Mỗi Joint luôn cần 2 thực thể: `parent link` (thằng cha - đứng yên) và `child link` (thằng con - phụ thuộc vào cha).

### 1.2 Nguyên Tắc "Gốc Tọa Độ Địa Phương" (Local Frame)
Đây là lý do chính khiến xe của bạn bị lỗi: **Mỗi Link (file `.stl`) đều có một gốc tọa độ (0,0,0) riêng của nó**, không phải là gốc tọa độ chung của RViz.

* Khi bạn tải file từ Onshape (Assembly): Tất cả các cụm (Part) đều đã bị phần mềm CAD lén ghi nhớ vị trí của chúng so với Gốc Không Gian của phần mềm đó. 
* Do đó, khi đưa vào ROS, nếu bạn muốn tụi nó giữ thành hình chiếc xe, bạn **KHÔNG ĐƯỢC** ghi thêm chỉ số `xyz` hay `rpy` trong file URDF. Chỉ cần ghép chúng lại ở tọa độ `0 0 0`, chúng sẽ tự tìm về vị trí cũ trên file CAD. Việc bạn gõ thêm `xyz="0 0 5"` là bạn đang cộng chồng thêm 5 mét vào vị trí vốn dĩ đã lệch của nó.

### 1.3 Quy Tắc Bàn Tay Phải (Right-hand Rule)
ROS làm việc trong hệ tọa độ:
* **X (Red - Đỏ):** Hướng nhìn về phía trước của xe (Mũi xe).
* **Y (Green - Xanh lá):** Hướng sang bên trái của xe.
* **Z (Blue - Xanh dương):** Hướng từ dưới đất lên trời.

**LƯU Ý:** Các phần mềm CAD (như SolidWorks, Onshape) thường lấy trục **Y** làm trục hướng lên trời. Đây là lý do xe bạn xuất ra bị lật dập mũi hoặc dựng đứng. Bạn phải lật chúng bằng tham số `rpy="0 -1.5708 0"` trong URDF để tương thích 2 thế giới. (1.5708 radian = 90 độ).

---

## 2. Các sai lầm hay gặp (Lưu ý)

1. **Sai Tỉ Lệ (Scale):**
   - ROS dùng mét (m) và kg. CAD dùng milimét (mm) và gram (g).
   - *Cách sửa:* Luôn gắn `scale="0.001 0.001 0.001"` vào thẻ `<mesh filename="...">` nếu không xe sẽ chiếm hết cả vũ trụ ảo.
2. **Xe bị chìm xuống đất hoặc bay lơ lửng:** 
   - Nguyên nhân: Việc xuất file lắp ráp từ hệ trục ảo trên Onshape không chuẩn.
   - *Cách sửa:* Dùng `base_footprint` làm mốc cố định ảo (0,0,0) trải trên mặt phẳng đất.
3. **Màu sắc tàng hình (Trắng lóa/mất màu):** 
   - Thẻ `<color rgba="r g b a"/>` dùng hệ chuẩn từ 0.0 đến 1.0, không phải hệ màu 0-255 của web. (Giả sử bạn muốn màu đỏ thì là `1 0 0 1`, còn màu đen là `0.1 0.1 0.1 1`).
4. **Không có thanh trượt Slider trên Joint State Publisher:** 
   - Lý do: Khớp Joint của bạn cài là `type="fixed"` (chết cứng). Hãy đổi thành `type="continuous"` (xoay vô hạn như bánh xe) hoặc `type="revolute"` (xoay có giới hạn như bản lề cửa).

---

## 3. Cách Xây Dựng File URDF Chi Tiết (Từng bước)

Đây là khung sườn để bạn tự viết một file URDF không bao giờ sinh lỗi:

### Bước 1: Khai báo tên xe
```xml
<?xml version="1.0"?>
<robot name="ten_xe_cua_ban">
```

### Bước 2: Khai báo cái bóng trên mặt đất
Để RViz biết đâu là trục số 0 của mặt đất, bạn phải tạo 1 link trống có tên là footprint.
```xml
  <link name="base_footprint"/>
```

### Bước 3: Đưa khung gầm chính vào và "Lật lại hình"
Khung gầm phải gắn liền với cái bóng dưới sàn. Đặt khớp nối (joint) "Fixed".
Phần `rpy="Roll Pitch Yaw"` chính là chỗ lật xe 90 độ để bù đắp khác biệt giữa Onshape và ROS.
```xml
  <joint name="base_joint" type="fixed">
    <parent link="base_footprint"/>
    <child link="base_link"/>
    <!-- Xoay trục Y 90 độ (Pitch) -->
    <origin xyz="0 0 0" rpy="0 -1.5708 0"/>
  </joint>

  <!-- Đọc bản vẽ STL của khung xe -->
  <link name="base_link">
    <visual>
      <geometry>
        <mesh filename="package://<Tên_Package>/meshes/khungxe.stl" scale="0.001 0.001 0.001"/>
      </geometry>
      <material name="Silver"><color rgba="0.75 0.75 0.75 1.0"/></material>
    </visual>
  </link>
```

### Bước 4: Lắp các linh kiện hoặc bánh xe vào khung gầm
Giờ khung xe đã lật đúng, tất cả mọi thứ bám vào nó sẽ kế thừa việc lật đó (sẽ tự động ngửa mặt lên theo cùng mẹ nó). 
Do bạn dùng chung file xuất từ Assembly, KHÔNG ĐƯỢC CỘNG THÊM TỌA ĐỘ ẢO (để mọi `origin` rỗng hoặc bằng 0).
```xml
  <!-- Quạt / Lidar / Động cơ / Bánh ... -->
  <joint name="linh_kien_joint" type="fixed">
    <parent link="base_link"/>
    <child link="linh_kien_link"/>
    <origin xyz="0 0 0" rpy="0 0 0"/>
  </joint>

  <link name="linh_kien_link">
    <visual>
      <geometry>
        <!-- Thay bằng tên STL tương ứng -->
        <mesh filename="package://<Tên_Package>/meshes/linhkien.stl" scale="0.001 0.001 0.001"/>
      </geometry>
      <material name="Black"><color rgba="0.1 0.1 0.1 1.0"/></material>
    </visual>
  </link>
```

### Bước 5: Đóng file
```xml
</robot>
```

### Lời khuyên tối thượng: 
**Hãy bỏ làm tay định dạng này.** Trừ khi bạn sinh viên đang làm bài tập tìm hiểu lý thuyết ROS đồ họa, không ai trên thế giới dùng tay để nối chuỗi STL từ phần mềm CAD qua cả, đặc biệt khi robot có trên 10 linh kiện động cơ. **Hãy sử dụng bộ chuyển đổi của Phần mềm Thiết kế của bạn.**
Tại tài liệu `/home/hiwe/project/car_automation/robot_description/stl_to_urdf_guide.md`, tôi đã viết hướng dẫn sử dụng thư viện **onshape-to-robot** (Cài đặt bằng dòng lệnh `pip install`). Thư viện đó tự đọc link hệ thống Onshape của bạn và sinh ra cấu trúc URDF không sai lệch 1 mi-li-mét nào, bao gồm luôn cả Trọng Lượng và Khối lượng quán tính mô hình!
