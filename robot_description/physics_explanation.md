# Giải thích chi tiết về việc thêm Vật lý và Va chạm (Physics & Collision) vào URDF

Để robot tự chạy trong các môi trường vật lý như **Gazebo**, file URDF ban đầu của bạn (chỉ có thẻ `<visual>`) cần được bổ sung 3 thành phần siêu quan trọng: 
1. **`<collision>`**: Khung va chạm để biết giới hạn vật lý.
2. **`<inertial>`**: Khối lượng và ma trận quán tính để tính toán trọng lực, gia tốc.
3. **`Gazebo Plugin`**: Thư viện điều khiển hệ thống truyền động.

Dưới đây là sơ đồ tính toán và giải thích thông số cho từng phần đã được chèn vào code:

---

### 1. Khung gầm chính (`base_link`)

- **Collision (Va chạm chính):**
  - **Vì sao không dùng mesh `base_link.stl`?** Dùng hình lưới (mesh) làm khung va chạm đòi hỏi máy tính phải tính toán ma sát trên hàng ngàn đa giác nhỏ, cực kỳ giật lag. Chuẩn ROS là dùng một hình hộp mờ (`<box>`) bao quanh khu vực đó.
  - **Kích thước `<box size="0.26 0.2 0.06"/>`:** Được mình ước lượng theo khoảng cách trục các bánh xe và vị trí các phụ kiện (dài 26cm, rộng 20cm, cao 6cm). 

- **Collision (Hai bánh Caster vô hình):**
  - **Vì sao phải thêm?** Xe của bạn cấu hình Diff-Drive chỉ có 2 bánh ở vị trí `X=0` (chính giữa). Trong đời thực hay Gazebo, xe có 2 bánh sẽ bị lật úp về phía trước/sau do mất thăng bằng. 
  - **Giải pháp:** Mình gắn 2 hình cầu (`<sphere radius="0.015"/>`) vô hình mọc ra từ đáy `base_link` (ở mạn trước và mạn sau). Hai hình cầu này chạm đất (ở tọa độ Z = -0.035) và hoạt động như 2 bánh xe phụ giúp giữ thăng bằng.
  - Mình thêm thẻ `<gazebo reference="base_link"> <mu1>0.0</mu1> </gazebo>` để ép gầm xe này có lực ma sát = 0. Như vậy 2 bánh phụ ảo này sẽ trượt đi rất êm mà không cản đường bánh chính quay.

- **Inertial (Khối lượng và quán tính):**
  - `<mass value="2.0"/>`: Áp dụng cho thân chính (2 kg vuông vắn, đủ nặng để ép bánh xe bám đường).
  - Khung ma trận quán tính `ixx, iyy, izz` được áp dụng công thức cho khối hộp chữ nhật đặc (`I = 1/12 * m * (w^2 + h^2)`). Mình đã điền sẵn số `0.0072`, `0.0118`, `0.0179` tương ứng.

---

### 2. Hai bánh xe (`wheel_left_link` & `wheel_right_link`)

- **Collision:**
  - Để mô phỏng bánh lăn tròn, hình trụ (`<cylinder radius="0.0325" length="0.025"/>`) là sự lựa chọn duy nhất.
  - Bán kính `R=0.0325` (3.25cm) vì mình tính ngược từ việc bạn nâng toàn bộ xe lên Z=0.05 và ngàm bánh xe ở Z=-0.0175.
  - Mình đã xoay khối trụ 90 độ (`rpy="1.57079 0 0"`) vì khối trụ mặc định đứng dọc theo trục Z, trong khi bánh xe của bạn lăn trên trục Y.

- **Inertial:**
  - Khối lượng bánh `m=0.2` (200 grams mỗi bánh). 
  - Ma trận quán tính dùng công thức cho hình trụ xoay dọc theo trục vòng cung.

- **Friction (Ma sát trên Gazebo):**
  - Có các thẻ phụ `<mu1>1.0</mu1>` ở phía dưới URDF áp cho riêng 2 bánh xe để ma sát max (bám sàn 100%), nếu không khi động cơ quay bánh xe sẽ bị "trượt patin" chứ không tiến tới được.

---

### 3. Cụm đồ trang trí, cảm biến (Cover, Holder, Camera, Lidar...)

- Với các khối như `cover_link`, `addon_link`, hay `camera_link`:
- **Collision:** Mình dùng các hình Box / Cylinder rất nhỏ bọc lại. Chúng không quá quan trọng cho vật lý vì nằm tít phía trên gầm xe, trừ khi xe chui vào gầm bàn thấp và bị quệt.
- **Inertial:** Đặt cho chúng khối lượng tượng trưng siêu nhẹ (khoảng `0.05` -> `0.1` kg) với các ma trận I rất nhỏ (`1e-5`). Việc cung cấp giá trị này là **bắt buộc** vì nếu thiếu, plugin Gazebo sẽ chối từ tải toàn bộ chiếc xe.

---

### 4. Plugin truyền động Diff Drive (`libgazebo_ros_diff_drive.so`)

Ở dòng cuối cùng của file `car.urdf`, mình đã gắn một "bộ não" thực thi nội bộ cho con ROS2:
- Tên các khớp là `wheel_left_joint` và `wheel_right_joint`.
- **`wheel_separation`** = `0.254`: Khoảng cách giữa tâm 2 bánh (tính từ khoảng cách `0.127` x 2).
- **`wheel_diameter`** = `0.065`: Đường kính lốp (`0.0325` x 2).
- Trình plugin này sẽ **tự động chực chờ nhận topic `/cmd_vel`** (phím điều hướng điều khiển) và tự tính toán tỷ lệ lực để ném vào hai bánh xe. Đồng thời nó cũng phát ra topic `/odom` ngược lại cho robot biết xe đã chạy được bao nhiêu mét thông qua dữ liệu nội suy vòng quay!

> **KẾT LUẬN:** Mình đã thay thế trực tiếp vào file `car.urdf` của bạn. Bạn hãy mở file URDF lên xem các comment mình kẹp vào và có thể spawn chiếc xe này trong Gazebo để chạy thử rồi!
