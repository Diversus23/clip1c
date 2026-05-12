## Самостоятельная сборка библиотеки

Компонента собирается через CMake. Требуется CMake 3.10 или новее и
компилятор C++17.

> **Тип в GitHub Actions:** при пуше в ветку или создании релиза CI
> ([.github/workflows/build.yml](../.github/workflows/build.yml))
> автоматически собирает бинарники под все платформы (Windows x86/x64,
> Linux x86/x64, macOS Universal) и упаковывает их в `AddIn.zip` вместе с
> `MANIFEST.XML`. На релиз ZIP добавляется в Assets автоматически.

Ниже описаны шаги для **локальной** сборки.

### Windows (Visual Studio)

Требуется Visual Studio 2019 или новее с поддержкой C++ Desktop.

```bat
Compile.bat
```

Скрипт собирает обе архитектуры (Win32 и x64) в `bin32/Release` и `bin64/Release`.

### Linux

Установить зависимости (на примере Ubuntu 20.04+):

```bash
sudo apt update
sudo apt install -y build-essential cmake git \
    libx11-dev libxtst-dev libpng-dev

# Для 32-битной сборки дополнительно:
sudo dpkg --add-architecture i386
sudo apt install -y gcc-multilib g++-multilib \
    libx11-dev:i386 libxtst-dev:i386 libpng-dev:i386
```

Запустить сборку:

```bash
./build.sh        # обе архитектуры
./build.sh 64     # только x86_64
./build.sh 32     # только i386
./build.sh 0      # очистить каталоги сборки
```

Артефакты: `bin32/Clip1CLin32.so`, `bin64/Clip1CLin64.so`.

### macOS

Требуется Xcode Command Line Tools (`xcode-select --install`) и CMake.

```bash
./build.sh
```

По умолчанию собирается только 64-битная версия (`bin64/Clip1CMac64.dylib`).
Архитектура определяется компилятором: на Apple Silicon будет arm64, на Intel
— x86_64. Для универсальной сборки задайте `CMAKE_OSX_ARCHITECTURES`:

```bash
mkdir -p build64L && cd build64L
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
```

### Отладочная сборка

Замените в командах `RelWithDebInfo` на `Debug` либо передайте
`-DCMAKE_BUILD_TYPE=Debug` напрямую в CMake.

### Локальная упаковка AddIn.zip

После того как собраны бинарники под все нужные платформы (и они лежат
в каталогах `bin32/` и `bin64/`), можно собрать `AddIn.zip` локально:

```bash
python3 tools/make_bundle.py --artifacts-dir . --output AddIn.zip
```

Скрипт:
- читает версию из [version.h](../version.h);
- ищет файлы вида `Clip1C{Win,Lin,Mac}{32,64}.{dll,so,dylib}`;
- добавляет к каждому суффикс версии (например `_1-0-0-0`);
- генерирует `MANIFEST.XML` по схеме `http://v8.1c.ru/8.2/addin/bundle`;
- упаковывает всё в один ZIP.

Опция `--strict` заставляет скрипт упасть, если не найден хотя бы один
из ожидаемых бинарников (полезно в CI).

***

При разработке использовались сторонние библиотеки:
- [Clip Library by David Capello](https://github.com/dacap/clip)
- [JSON for Modern C++ by Niels Lohmann](https://github.com/nlohmann/json)
