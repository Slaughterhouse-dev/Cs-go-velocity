# Velocity - CS:GO Movement Trainer

Показывает скорость игрока в реальном времени для тренировки движения в CS:GO.

## Установка

Python 3.10+
```sh
pip install -r requirements.txt
```

## Запуск
```sh
python Velocity/Velocity.py
```

## Функции
- Отображение текущей скорости игрока

![Velocity](https://github.com/user-attachments/assets/6b1b5c9b-9373-4db8-9b75-bbc11156e884)

- Первое число - текущая скорость
- Число в скобках - предыдущая скорость
- Работает в оконном режиме ("В окне" или "Полноэкранный в окне")

## Оффсеты
Оффсеты загружаются автоматически с [hazedumper](https://github.com/frk1/hazedumper)

## Ошибка с imgui
При установке imgui может возникнуть ошибка из-за отсутствия Microsoft C++ Compiler. Скачать можно здесь: [Microsoft C++ Build Tools](https://visualstudio.microsoft.com/ru/visual-cpp-build-tools/)

## Лицензия
MIT License - см. файл `LICENSE`
