# Velocity - Cs:go Speed Overlay

Отображение скорости игрока в CS:GO в реальном времени.

| Velocity | Move | Resize |
|:--------:|:----:|:------:|
| <img src="./images/Velocity.png" width="200"> | <img src="./images/Move.png" width="200"> | <img src="./images/Resize.png" width="200"> |


## Примечание
- Также если вы хотите запустить его без Visual Studio 2022, ибо некоторые люди не используют его и не знаю что это
- То можно использовать просто файл который лежит уже в проекте он находится в папке Release и для External и для Internal (только там Dll для неё нужен инжектор)

## Функции

- Overlay поверх игры с текущей скоростью
- Формат: `Скорость в настоящие время передвижение (последняя)` - например `230 (220)`
- Pattern scanning - не нужно обновлять оффсеты вручную
- Две версии: External (Exe) и Internal (Dll)

## Версии

### External (рекомендуется)
Отдельная программа, читает память игры извне.

**Плюсы:**
- Не требует инжектор
- Безопаснее
- Легко обновлять

**Сборка:**
1. Открой `External/External.sln` в Visual Studio 2022
2. Выбери Release | x86
3. Build → Build Solution (Ctrl+Shift+B) или же Локальный откладчик Windows

**Использование:**
1. Запусти Cs:go
2. Запусти `External.exe` от Администратора или можно просто, если не сработает, то тогда от имени Администратора
3. Готово можно играть и видеть скорость

### Internal
Dll для инжекта в процесс игры.

**Плюсы:**
- Работает изнутри процесса

**Сборка:**
1. Открой `Internal/Internal.sln` в Visual Studio 2022
2. Выбери Release | x86
3. Build → Build Solution (Ctrl+Shift+B) или же Локальный откладчик Windows

**Использование:**
1. Запусти Cs:go
2. Инжектни `Internal.dll` любым инжектором (Extreme Injector Standard / LoadLibrar)
3. Готово

## Управление

| Клавиша | Действие |
|---------|----------|
| Home | Скрыть / показать консоль |
| Delete | Скрыть / показать overlay |
| End | Выход (только External) |

## Структура проекта

```
├── External/          # External версия (Exe)
│   ├── src/
│   │   └── main.cpp
│   ├── External.sln
│   └── External.vcxproj
├── Internal/          # Internal версия (Dll)
│   ├── src/
│   │   └── main.cpp
│   ├── Internal.sln
│   └── Internal.vcxproj
├── Offsets/           # Оффсеты hazedumper (для справки)
└── images/            # Скриншоты
```

## Требования

- Visual Studio 2022
- Windows SDK 10.0
- CS:GO (не CS2!)
