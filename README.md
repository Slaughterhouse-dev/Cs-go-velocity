# Velocity - CS:GO Speed Overlay

External overlay для отображения скорости игрока в CS:GO в реальном времени.

![Velocity Overlay](https://private-user-images.githubusercontent.com/168532046/524657583-6b1b5c9b-9373-4db8-9b75-bbc11156e884.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjUzNjE3MzEsIm5iZiI6MTc2NTM2MTQzMSwicGF0aCI6Ii8xNjg1MzIwNDYvNTI0NjU3NTgzLTZiMWI1YzliLTkzNzMtNGRiOC05Yjc1LWJiYzExMTU2ZTg4NC5wbmc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjUxMjEwJTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI1MTIxMFQxMDEwMzFaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT1iMzAzMWZhMWViYWFhNjNmMDYyZTE1MTcxZDU3NjM4NjJlMjNhNmQ5NjlmNDM2NmFmNzZhNDA4OWYyM2ZkZmY5JlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCJ9.wHrsmmBZfn4yKhUHOWoAgd8bLahmOWkdkc-ZfgbzXeY)

## Функции
- Overlay поверх игры с текущей скоростью
- Формат: `скорость (последняя)` — например `247 (240)`
- Цвет меняется в зависимости от скорости

## Сборка

Открой `Velocity/Velocity.sln` в Visual Studio и собери проект (Release x86).

Или через командную строку:
```sh
cd Velocity
msbuild Velocity.sln /p:Configuration=Release /p:Platform=x86
```

## Использование

1. Запусти CS:GO
2. Запусти `Velocity.exe` от имени администратора
3. Зайди на карту
4. ESC для выхода

## Оффсеты

Оффсеты в папке `Offsets/` — нужно обновлять под актуальную версию игры.
Свежие оффсеты: [hazedumper](https://github.com/frk1/hazedumper)

## Лицензия

MIT License
