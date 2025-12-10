import pymem
import pymem.process
import win32gui, win32con
import time, os
import math
import imgui
from imgui.integrations.glfw import GlfwRenderer
import glfw
import OpenGL.GL as gl
import requests

WINDOW_WIDTH = 1920
WINDOW_HEIGHT = 1080

# Загружаем оффсеты CS:GO с hazedumper
offsets = requests.get('https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.json').json()

# Signatures
dwLocalPlayer = offsets['signatures']['dwLocalPlayer']

# Netvars
m_vecVelocity = offsets['netvars']['m_vecVelocity']
m_fFlags = offsets['netvars']['m_fFlags']

# Ожидаем запуска csgo.exe
print("Ожидание запуска CS:GO...")
while True:
    time.sleep(1)
    try:
        pm = pymem.Pymem("csgo.exe")
        client = pymem.process.module_from_name(pm.process_handle, "client.dll").lpBaseOfDll
        break
    except:
        pass

time.sleep(1)
os.system("cls")
print("CS:GO найден!")

def get_velocity():
    """Получает скорость локального игрока"""
    try:
        local_player = pm.read_int(client + dwLocalPlayer)
        if not local_player:
            return 0, 0
        
        # Читаем вектор скорости (x, y, z)
        vel_x = pm.read_float(local_player + m_vecVelocity)
        vel_y = pm.read_float(local_player + m_vecVelocity + 4)
        vel_z = pm.read_float(local_player + m_vecVelocity + 8)
        
        # Горизонтальная скорость (без учёта вертикальной)
        speed_2d = math.sqrt(vel_x**2 + vel_y**2)
        # Полная скорость
        speed_3d = math.sqrt(vel_x**2 + vel_y**2 + vel_z**2)
        
        return int(speed_2d), int(speed_3d)
    except:
        return 0, 0

def main():
    # Инициализация glfw
    glfw.init()
    glfw.window_hint(glfw.TRANSPARENT_FRAMEBUFFER, glfw.TRUE)
    window = glfw.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Velocity", None, None)

    hwnd = glfw.get_win32_window(window)
    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
    style &= ~(win32con.WS_CAPTION | win32con.WS_THICKFRAME)
    win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
    ex_style = win32con.WS_EX_TRANSPARENT | win32con.WS_EX_LAYERED
    win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
    win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, -2, -2, 0, 0,
                          win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE)

    glfw.make_context_current(window)
    imgui.create_context()
    impl = GlfwRenderer(window)

    # Переменная для хранения предыдущей скорости (для отображения в скобках)
    prev_speed = 0

    # Основной цикл
    while not glfw.window_should_close(window):
        glfw.poll_events()
        impl.process_inputs()
        imgui.new_frame()

        imgui.set_next_window_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        imgui.set_next_window_position(0, 0)
        imgui.begin("Velocity Overlay",
                    flags=imgui.WINDOW_NO_TITLE_BAR |
                          imgui.WINDOW_NO_RESIZE |
                          imgui.WINDOW_NO_SCROLLBAR |
                          imgui.WINDOW_NO_COLLAPSE |
                          imgui.WINDOW_NO_BACKGROUND)

        draw_list = imgui.get_window_draw_list()
        
        # Получаем скорость
        speed_2d, speed_3d = get_velocity()
        
        # Цвет в зависимости от скорости (зелёный)
        color = imgui.get_color_u32_rgba(0, 1, 0, 1)
        
        # Позиция текста (центр экрана, чуть ниже)
        text_x = WINDOW_WIDTH // 2 - 50
        text_y = WINDOW_HEIGHT // 2 + 100
        
        # Формат: "247 (240)" - текущая скорость и предыдущая в скобках
        speed_text = f"{speed_2d} ({prev_speed})"
        draw_list.add_text(text_x, text_y, color, speed_text)
        
        # Обновляем предыдущую скорость
        prev_speed = speed_2d

        imgui.end()
        imgui.end_frame()

        gl.glClearColor(0, 0, 0, 0)
        gl.glClear(gl.GL_COLOR_BUFFER_BIT)
        imgui.render()
        impl.render(imgui.get_draw_data())
        glfw.swap_buffers(window)

    impl.shutdown()
    glfw.terminate()

if __name__ == '__main__':
    main()
