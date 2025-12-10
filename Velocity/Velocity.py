import pymem
import pymem.process
import win32gui
import win32con
import win32api
import time
import math
import pygame
import json

WINDOW_WIDTH = 400
WINDOW_HEIGHT = 100

# Загружаем оффсеты из JSON
with open("Offsets/csgo.json", "r") as f:
    offsets = json.load(f)

dwLocalPlayer = offsets["signatures"]["dwLocalPlayer"]
dwEntityList = offsets["signatures"]["dwEntityList"]
dwClientState = offsets["signatures"]["dwClientState"]
dwClientState_GetLocalPlayer = offsets["signatures"]["dwClientState_GetLocalPlayer"]
m_vecVelocity = offsets["netvars"]["m_vecVelocity"]

# Ожидаем запуска csgo.exe
print("Ожидание запуска CS:GO...")
while True:
    time.sleep(1)
    try:
        pm = pymem.Pymem("csgo.exe")
        client = pymem.process.module_from_name(pm.process_handle, "client.dll").lpBaseOfDll
        engine = pymem.process.module_from_name(pm.process_handle, "engine.dll").lpBaseOfDll
        break
    except:
        pass

print(f"CS:GO найден!")
print(f"client.dll: {hex(client)}")
print(f"engine.dll: {hex(engine)}")

# Пробуем разные способы получить LocalPlayer
time.sleep(2)

# Способ 1: напрямую через dwLocalPlayer
lp1 = pm.read_int(client + dwLocalPlayer)
print(f"Способ 1 (dwLocalPlayer): {hex(lp1) if lp1 else 'NULL'}")

# Способ 2: через ClientState -> GetLocalPlayer -> EntityList
try:
    client_state = pm.read_int(engine + dwClientState)
    local_player_index = pm.read_int(client_state + dwClientState_GetLocalPlayer)
    lp2 = pm.read_int(client + dwEntityList + local_player_index * 0x10)
    print(f"Способ 2 (EntityList): index={local_player_index}, addr={hex(lp2) if lp2 else 'NULL'}")
except Exception as e:
    print(f"Способ 2 ошибка: {e}")
    lp2 = 0

def get_local_player():
    """Пробует разные способы получить LocalPlayer"""
    # Способ 1: напрямую
    lp = pm.read_int(client + dwLocalPlayer)
    if lp:
        return lp
    # Способ 2: через EntityList
    try:
        client_state = pm.read_int(engine + dwClientState)
        local_player_index = pm.read_int(client_state + dwClientState_GetLocalPlayer)
        lp = pm.read_int(client + dwEntityList + local_player_index * 0x10)
        if lp:
            return lp
    except:
        pass
    return 0

def get_velocity():
    """Читает скорость через LocalPlayer + m_vecVelocity"""
    try:
        local_player = get_local_player()
        if local_player == 0:
            return 0
        vel_x = pm.read_float(local_player + m_vecVelocity)
        vel_y = pm.read_float(local_player + m_vecVelocity + 4)
        speed = math.sqrt(vel_x**2 + vel_y**2)
        return int(speed)
    except:
        return 0

def make_window_overlay(hwnd):
    style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
    style = style | win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT | win32con.WS_EX_TOPMOST | win32con.WS_EX_NOACTIVATE
    win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, style)
    win32gui.SetLayeredWindowAttributes(hwnd, win32api.RGB(0, 0, 0), 0, win32con.LWA_COLORKEY)

def main():
    pygame.init()
    
    screen_info = pygame.display.Info()
    screen_w = screen_info.current_w
    screen_h = screen_info.current_h
    
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.NOFRAME)
    pygame.display.set_caption("Velocity")
    
    hwnd = pygame.display.get_wm_info()["window"]
    
    pos_x = (screen_w - WINDOW_WIDTH) // 2
    pos_y = screen_h - WINDOW_HEIGHT - 150
    
    win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, pos_x, pos_y, 
        WINDOW_WIDTH, WINDOW_HEIGHT, win32con.SWP_SHOWWINDOW)
    
    make_window_overlay(hwnd)
    
    font = pygame.font.Font(None, 72)
    clock = pygame.time.Clock()
    
    prev_speed = 0
    max_speed = 0
    was_moving = False
    running = True
    
    while running:
        win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, pos_x, pos_y, 0, 0, 
            win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE)
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False
        
        screen.fill((0, 0, 0))
        
        speed = get_velocity()
        
        is_moving = speed > 10
        if is_moving:
            if speed > max_speed:
                max_speed = speed
            was_moving = True
        else:
            if was_moving and max_speed > 0:
                prev_speed = max_speed
                max_speed = 0
            was_moving = False
        
        text = f"{speed} ({prev_speed})"
        
        text_surface = font.render(text, True, (0, 255, 0))
        text_rect = text_surface.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2))
        screen.blit(text_surface, text_rect)
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()

if __name__ == '__main__':
    main()
