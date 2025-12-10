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
    # Пробуем несколько источников hazedumper
    urls = [
        'https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.json',
        'https://raw.githubusercontent.com/KittenPopo/csgo-offsets/main/offsets.json',
    ]
    
    for url in urls:
        try:
            data = requests.get(url, timeout=5).json()
            if 'signatures' in data:
                return data['signatures']['dwLocalPlayer'], data['netvars']['m_vecVelocity']
            elif 'dwLocalPlayer' in data:
                return data['dwLocalPlayer'], data.get('m_vecVelocity', 0x114)
        except:
            continue
    
    # Локальные оффсеты - последние известные рабочие
    # m_vecVelocity обычно 0x114 (276 в десятичной)
    return 0xDEB99C, 0x114

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

debug_info = ""

def get_velocity():
    """Получает скорость локального игрока"""
    global dwLocalPlayer, m_vecVelocity, debug_info
    
    try:
        # Пробуем разные способы чтения
        # Способ 1: read_int (32-bit pointer)
        local_player = pm.read_int(client + dwLocalPlayer)
        
        # Способ 2: если первый не работает, пробуем read_uint
        if not local_player:
            local_player = pm.read_uint(client + dwLocalPlayer)
        
        debug_info = f"LP: {hex(local_player) if local_player else '0'}"
        
        if not local_player:
            return -1  # Игрок не найден
        
        # Читаем вектор скорости
        vel_x = pm.read_float(local_player + m_vecVelocity)
        vel_y = pm.read_float(local_player + m_vecVelocity + 4)
        
        debug_info += f" V:{vel_x:.0f},{vel_y:.0f}"
        
        # Вычисляем горизонтальную скорость
        speed = math.sqrt(vel_x**2 + vel_y**2)
        return int(speed)
    except pymem.exception.MemoryReadError as e:
        debug_info = f"MemErr: {e}"
        return -2
    except Exception as e:
        debug_info = f"Err: {e}"
        return -3

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
        
        # Отладочная информация мелким шрифтом
        small_font = pygame.font.Font(None, 24)
        debug_surface = small_font.render(debug_info, True, (255, 255, 0))
        screen.blit(debug_surface, (10, 10))
        
        prev_speed = speed
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()

if __name__ == '__main__':
    main()
