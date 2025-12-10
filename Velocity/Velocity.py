import pymem
import pymem.process
import win32gui
import win32con
import win32api
import time
import os
import math
import requests
import pygame

WINDOW_WIDTH = 400
WINDOW_HEIGHT = 100

# Загружаем актуальные оффсеты CS:GO с нескольких источников
print("Загрузка оффсетов...")

def load_offsets():
    # Пробуем hazedumper
    try:
        offsets = requests.get('https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.json', timeout=5).json()
        return offsets['signatures']['dwLocalPlayer'], offsets['netvars']['m_vecVelocity']
    except:
        pass
    
    # Пробуем a2x/cs2-dumper для CS:GO (legacy)
    try:
        offsets = requests.get('https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/csgo/offsets.json', timeout=5).json()
        client = requests.get('https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/csgo/client.dll.json', timeout=5).json()
        return offsets['dwLocalPlayer'], client['C_BaseEntity']['m_vecVelocity']
    except:
        pass
    
    # Локальные оффсеты (обновлены декабрь 2024)
    return 0xDEA96C, 0x114

dwLocalPlayer, m_vecVelocity = load_offsets()
print(f"Оффсеты: dwLocalPlayer={hex(dwLocalPlayer)}, m_vecVelocity={hex(m_vecVelocity)}")

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

print("CS:GO найден!")
time.sleep(1)

def get_velocity():
    """Получает скорость локального игрока"""
    global dwLocalPlayer, m_vecVelocity
    
    try:
        # Читаем указатель на локального игрока
        local_player = pm.read_int(client + dwLocalPlayer)
        if not local_player:
            return -1  # Игрок не найден
        
        # Читаем вектор скорости
        vel_x = pm.read_float(local_player + m_vecVelocity)
        vel_y = pm.read_float(local_player + m_vecVelocity + 4)
        
        # Вычисляем горизонтальную скорость
        speed = math.sqrt(vel_x**2 + vel_y**2)
        return int(speed)
    except pymem.exception.MemoryReadError:
        return -2  # Ошибка чтения памяти
    except Exception as e:
        return -3  # Другая ошибка

def make_window_overlay(hwnd):
    """Делает окно оверлеем поверх всех окон включая игры"""
    # Убираем рамку и делаем прозрачным
    style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
    style = style | win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT | win32con.WS_EX_TOPMOST | win32con.WS_EX_NOACTIVATE
    win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, style)
    
    # Чёрный цвет будет прозрачным
    win32gui.SetLayeredWindowAttributes(hwnd, win32api.RGB(0, 0, 0), 0, win32con.LWA_COLORKEY)

def main():
    pygame.init()
    
    # Получаем размер экрана ДО создания окна
    screen_info = pygame.display.Info()
    screen_w = screen_info.current_w
    screen_h = screen_info.current_h
    
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.NOFRAME)
    pygame.display.set_caption("Velocity")
    
    hwnd = pygame.display.get_wm_info()["window"]
    
    # Позиция окна (центр экрана, внизу)
    pos_x = (screen_w - WINDOW_WIDTH) // 2
    pos_y = screen_h - WINDOW_HEIGHT - 150  # 150 пикселей от низа экрана
    
    # Сразу ставим окно в нужную позицию
    win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, pos_x, pos_y, 
        WINDOW_WIDTH, WINDOW_HEIGHT, win32con.SWP_SHOWWINDOW)
    
    make_window_overlay(hwnd)
    
    font = pygame.font.Font(None, 72)
    clock = pygame.time.Clock()
    
    prev_speed = 0
    running = True
    
    while running:
        # Держим окно поверх всех
        win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, pos_x, pos_y, 0, 0, 
            win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE)
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False
        
        screen.fill((0, 0, 0))
        
        speed = get_velocity()
        text = f"{speed} ({prev_speed})"
        
        # Зелёный цвет
        text_surface = font.render(text, True, (0, 255, 0))
        text_rect = text_surface.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2))
        screen.blit(text_surface, text_rect)
        
        prev_speed = speed
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()

if __name__ == '__main__':
    main()
