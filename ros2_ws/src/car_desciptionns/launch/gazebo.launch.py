import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, ExecuteProcess, RegisterEventHandler
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    pkg_name = 'car_desciptionns'
    pkg_share = get_package_share_directory(pkg_name)
    urdf_file = os.path.join(pkg_share, 'urdf', 'car.urdf')

    # Đọc file URDF
    with open(urdf_file, 'r') as infp:
        robot_desc = infp.read()

    # Định tuyến thư mục model của Gazebo để nó tìm thấy thư mục meshes
    os.environ['GAZEBO_MODEL_PATH'] = os.path.join(get_package_share_directory('car_desciptionns'), '..')

    # Khởi động Gazebo Server & Client (Nền giả lập)
    gazebo_pkg = get_package_share_directory('gazebo_ros')
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_pkg, 'launch', 'gazebo.launch.py')
        )
    )

    # Nút dịch URDF và phát sinh các hệ tọa độ.
    # LƯU Ý MẤU CHỐT: Bật `use_sim_time` = True để ROS dùng chung đồng hồ với Gazebo
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_desc,
            'use_sim_time': True
        }]
    )

    # Nút thả xe (Spawn Entity) vào trong Gazebo
    spawn_entity = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=['-entity', 'car_robot', '-file', urdf_file, '-z', '0.1'],
        output='screen'
    )

    return LaunchDescription([
        gazebo,
        robot_state_publisher_node,
        spawn_entity
    ])
