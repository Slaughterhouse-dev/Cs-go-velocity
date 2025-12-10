# Velocity - CS:GO Movement Trainer

Показывает скорость игрока в реальном времени для тренировки движения в CS:GO.

## Версии

### C++ (рекомендуется)
```sh
cd Velocity
build.bat
```
Или через CMake:
```sh
cd Velocity
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
```

### Python
```sh
pip install -r requirements.txt
python Velocity/Velocity.py
```

## Функции
- Отображение текущей скорости игрока
- Первое число - текущая скорость
- Число в скобках - максимальная скорость предыдущего движения

## Оффсеты
Оффсеты из [hazedumper](https://github.com/frk1/hazedumper) в папке `Offsets/`

## Лицензия
MIT License - см. файл `LICENSE`
