# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Что это

Native API внешняя компонента 1С для работы с буфером обмена. Один экспортируемый объект — `ClipboardControl`. Собирается под Windows (x86/x64), Linux (x86/x64) и macOS (x86_64 и arm64 — отдельными бинарниками). Выпускается как `AddIn.zip` с `MANIFEST.XML` по схеме `http://v8.1c.ru/8.2/addin/bundle`.

Это форк подсистемы буфера обмена из [VanessaExt](https://github.com/lintest/VanessaExt) с добавленной поддержкой macOS. Тестов в репозитории нет — единственная проверка корректности это успешная компиляция под все целевые платформы.

## Сборка

| Платформа | Команда | Артефакты |
|-----------|---------|-----------|
| Windows (Win32+x64) | `Compile.bat` (требует MSVC; CMake падает на не-MSVC) | `bin32\Release\Clip1CWin32.dll`, `bin64\Release\Clip1CWin64.dll` |
| Linux (x86_64) | `./build.sh 64` | `bin64/Clip1CLin64.so` |
| Linux (i386) | `./build.sh 32` (нужны i386-multilib пакеты) | `bin32/Clip1CLin32.so` |
| macOS (x86_64) | `cmake -B build64Intel -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 && cmake --build build64Intel` | `bin64/Clip1CMac64.dylib` |
| macOS (arm64) | `cmake -B build64Arm -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 && cmake --build build64Arm` | `bin64/Clip1CMacARM64.dylib` |
| Очистка | `./build.sh 0` (Linux/macOS) | удаляет `build*L/` |

Имя выходного файла следует шаблону `Clip1C{Win\|Lin\|Mac}{32\|64}.{dll\|so\|dylib}`, а для macOS arm64 — `Clip1CMacARM64.dylib` (суффикс `ARM64` вместо битности; каталог вывода при этом остаётся `bin64`). На имена завязаны `tools/make_bundle.py` и `MANIFEST.XML` (macOS объявляется двумя записями — `arch="x86_64"` и `arch="ARM64"`, как в эталоне 1С). Если меняешь `OUTPUT_NAME` в CMake — синхронизируй с `default_components()` в `make_bundle.py`.

На macOS бинарник обязательно подписывается (в CMake POST_BUILD — ad-hoc `codesign --sign -`): без подписи arm64-слайс не грузится через `dlopen` на Apple Silicon, и 1С не может создать объект компоненты.

Локальная сборка AddIn.zip (после того как собраны нужные платформы):
```bash
python3 tools/make_bundle.py --artifacts-dir . --output AddIn.zip
```

Версия читается из `version.h` (макрос `VERSION_FULL`) и в C++, и в `make_bundle.py`. Бамп версии — единственная точка изменения.

## Релиз

CI собирает релиз автоматически по push'у тега `vX.Y.Z`:
1. Обновить `VERSION_FULL` и компоненты `VERSION_MAJOR/MINOR/REVISION/BUILD` в `version.h`.
2. Закоммитить, поставить аннотированный тег `git tag -a vX.Y.Z -m "..."`, запушить.
3. Workflow `.github/workflows/build.yml` соберёт все 6 бинарников (Win32/Win64/Lin32/Lin64/Mac64/MacARM64), упакует в `AddIn.zip`, создаст GitHub Release с авто-генерируемыми release notes.

Теги вида `v1.0.1-rc1`, `-beta`, `-alpha` помечаются как pre-release.

## Архитектура

Поток вызова от 1С:
```
1С runtime ──► экспортируемые из DLL функции (AddInNative.cpp / .def / version.script)
              GetClassObject ─► AddInNative::CreateObject ─► фабрика по имени класса
                                                              │
                                                              ▼
                                                       ClipboardControl::ClipboardControl()
                                                       регистрирует свойства и методы
                                                              │
              CallAsFunc/SetPropVal ────────────────────────► dispatch через std::variant<MethFunction0..7>
                                                              │
                                                              ▼
                                                       ClipboardManager (per-platform impl)
```

### Ключевые слои

- **`src/AddInNative.{h,cpp}`** — адаптер к 1С Native API SDK. Реализует `IComponentBase`. Содержит `VarinantHelper` для маршалинга `tVariant` ↔ C++ типы (строки, числа, BLOB), систему регистрации свойств/методов через лямбды и `AddComponent` для саморегистрации классов.
- **`src/ClipboardControl.{h,cpp}`** — *единственный* экспортируемый класс. Конструктор объявляет всё API через `AddProperty`/`AddFunction` парами имён (английское + русское). Здесь же Windows-specific мониторинг буфера через `WM_CLIPBOARDUPDATE` → `ExternalEvent`.
- **`src/ClipboardManager.{cpp,_osx.mm}`** — три отдельные реализации под `#ifdef _WINDOWS / __APPLE__ / else`. Один и тот же интерфейс `BaseHelper::ClipboardManager`, разные backends:
  - Windows: WinAPI (`OpenClipboard`, `CF_UNICODETEXT`, `CF_HDROP`, `CF_DIBV5`, `CF_PNG` через `RegisterClipboardFormat`), изображения — через `ImageHelper` (GDI+).
  - macOS: `NSPasteboard` из AppKit; пишет картинку и как PNG, и как TIFF для совместимости.
  - Linux: `3rdparty/clip-1.3` (David Capello), X11 + libpng.
- **`src/BaseHelper.h`** — пустой shim между `AddInNative` и `ClipboardControl`, объявляет вложенные классы `ClipboardManager`/`ImageHelper` для разных платформ. Не трогать без необходимости.
- **`include/`** — заголовки 1С Native API SDK (`ComponentBase.h`, `AddInDefBase.h`, `IMemoryManager.h`, `com.h`, `types.h`). **Сторонний код, не редактировать.**

### Регистрация API

Свойство или метод появляется в 1С только если зарегистрирован в `ClipboardControl::ClipboardControl()`. Регистрация = пара имён `(English, Русское)` + лямбда-геттер/сеттер/обработчик. `AddInNative` сам перебирает вектор `properties`/`methods` для `FindProp`/`FindMethod` (с case-insensitive fallback через `upper()`). 1С локаль (`SetLocale`) переключает алиасы в текстах ошибок.

Сигнатура обработчика метода (количество параметров) определяется компилятором через `std::variant<MethFunction0..7>` — `GetNParams` использует `std::get_if` для подсчёта. Если нужно больше 7 параметров — расширь `MethFunction` в `AddInNative.h`.

### Платформенные тонкости

- **macOS BOOL conflict (см. `stdafx`/`ClipboardManager_osx.mm`):** Apple ObjC runtime определяет `BOOL` как `signed char` и выставляет `OBJC_BOOL_DEFINED`. 1С SDK (`include/com.h`) тоже хочет определить `BOOL=int`. **Apple-фреймворки (`Foundation`, `AppKit`) обязательно импортируются ПЕРВЫМИ** — иначе typedef redefinition. Это уже починено, но при добавлении нового `.mm` файла соблюдай порядок.
- **Linux экспорт символов:** ограничен `src/version.script` — наружу видны только `DllMain`, `GetClassObject`, `DestroyObject`, `GetClassNames`, `SetPlatformCapabilities`. Если добавишь новую экспортируемую функцию — пропиши и в `version.script`, и в `AddInNative.def` (для Windows).
- **Windows-only функции:** мониторинг буфера (`Monitoring` property) и `ImageHelper` (GDI+). Под `#ifdef _WINDOWS`. На других платформах эти возможности недоступны.
- **MSVC обязателен под Windows.** CMake падает с `FATAL_ERROR` на MinGW/Clang.

### Память и tVariant

Возврат строки/BLOB в 1С идёт через `IMemoryManager::AllocMemory` — буфер потом освободит 1С runtime. `VarinantHelper::operator=` и `AllocMemory` это уже инкапсулируют — снаружи просто `var = u"строка"` или `var.AllocMemory(size); memcpy(var.data(), ...)`. Не вызывай `delete`/`free` для данных, которые уходят в `tVariant`.

## Документация

- `Docs/README.md` — общая инструкция для разработчика-1С (как подключить компоненту, обработка JSON).
- `Docs/ClipboardControl.md` — публичное API объекта.
- `Docs/BuildLibrary.md` — детальная инструкция по локальной сборке и релизу.
- `CHANGELOG.md` — описание изменений.
