# Client-UI (Messenger2)

Простой клиентский мессенджер: UI-приложение на C++ с сетевым подключением и базовым чатом.

## Стандарт C++

- C++23 (`CMAKE_CXX_STANDARD=23`)

## Возможности (на текущем этапе)

- Регистрация/подключение к серверу
- Отправка и приём сообщений чата
- UI на RmlUi (HTML/CSS-подобная разметка)

## Архитектура (коротко)

- `App` — сценарии (use-cases), сервисы и UI-сцены
- `Engine` — базовый движок приложения/сцен
- `Core` — общие типы/утилиты
- `network` — WebSocket-клиент и сессии (транспорт)
- `protocol` — JSON-парсинг/пакетирование сообщений

В `App` бизнес-логика вынесена в сервисы (например `RegistrationService`, `ChatService`), которые обрабатывают сетевые события и публикуют события для UI через `AppEventHub`. Сцены подписываются на `AppEventHub` и обновляют интерфейс (Rml DOM).

## Используемые библиотеки

- SDL3 / SDL3_image / SDL3_mixer — окно/рендер/ввод/аудио
- SDLWrapper — C++-обёртка над SDL3
- RmlUi — UI (RML/RCSS)
- FreeType — рендеринг шрифтов
- Boost (system, asio/beast) — сеть/вспомогательные компоненты
- nlohmann_json — JSON
- pugixml — XML (вендорится в `extern/`)

## Сборка (CMake)

Проект использует CMake Presets (`CMakePresets.json`). Библиотеки подтягиваются через `find_package(...)` и/или внешнюю директорию `EXTERNAL_DIR` (см. пресеты).

Пример (Windows):

```powershell
cmake --preset win-debug
cmake --build build
```

## Лицензия

MIT License — см. файл `LICENSE`.
