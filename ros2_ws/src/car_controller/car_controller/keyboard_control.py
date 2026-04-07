import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys
import termios
import tty
import select

msg = """
Điều khiển Xe (Car Robot)
---------------------------
Chuyển động tịnh tiến:
        w
   a    s    d
        x

w/x : Tăng/giảm tốc độ tiến lùi (+/- 0.1 m/s)
a/d : Tăng/giảm tốc độ xoay trái/phải (+/- 0.1 rad/s)
s   : DỪNG HẲN (Force Stop)

Nhấn CTRL-C để thoát
"""

class KeyboardController(Node):
    def __init__(self):
        super().__init__('keyboard_controller')
        self.publisher_ = self.create_publisher(Twist, 'cmd_vel', 10)
        self.settings = termios.tcgetattr(sys.stdin)
        self.speed = 0.0
        self.turn = 0.0

        self.get_logger().info(msg)

        # Chạy vòng lặp nhận phím điều khiển
        self.timer = self.create_timer(0.1, self.loop)

    def getKey(self):
        tty.setraw(sys.stdin.fileno())
        rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
        if rlist:
            key = sys.stdin.read(1)
        else:
            key = ''
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

    def loop(self):
        key = self.getKey()
        if key != '':
            if key == 'w':
                self.speed += 0.1
            elif key == 'x':
                self.speed -= 0.1
            elif key == 'a':
                self.turn += 0.1
            elif key == 'd':
                self.turn -= 0.1
            elif key == 's':
                self.speed = 0.0
                self.turn = 0.0
            elif key == '\x03': # CTRL+C
                sys.exit()

            # Giới hạn vận tốc
            self.speed = round(max(min(self.speed, 1.0), -1.0), 2)
            self.turn = round(max(min(self.turn, 2.0), -2.0), 2)
            
            print(f"Vận tốc: {self.speed} m/s | Góc quay: {self.turn} rad/s")

            # Gửi lệnh điều khiển `/cmd_vel`
            twist = Twist()
            twist.linear.x = float(self.speed)
            twist.linear.y = 0.0
            twist.linear.z = 0.0
            twist.angular.x = 0.0
            twist.angular.y = 0.0
            twist.angular.z = float(self.turn)

            self.publisher_.publish(twist)

def main(args=None):
    rclpy.init(args=args)
    node = KeyboardController()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # Dừng xe khi thoát
        twist = Twist()
        node.publisher_.publish(twist)
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
