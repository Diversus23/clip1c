## Самостоятельная сборка библиотеки

Компонента собирается через CMake. Требуется CMake 3.10 или новее и
компилятор C++17.

> **Тип в GitHub Actions:** при пуше в ветку или создании релиза CI
> ([.github/workflows/build.yml](../.github/workflows/build.yml))
> автоматически собирает бинарники под все платформы (Windows x86/x64,
> Linux x86/x64, macOS x86_64 и arm64) и упаковывает их в `AddIn.zip` вместе с
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

Компонента поставляется двумя отдельными бинарниками — под Intel (x86_64) и
Apple Silicon (arm64), как в эталонном примере 1С. Клиент 8.3.21+ на Apple
Silicon — нативный arm64 и ищет в `MANIFEST.XML` запись `arch="ARM64"`; одной
универсальной записи ему недостаточно. Архитектура задаётся через
`CMAKE_OSX_ARCHITECTURES`, а суффикс имени файла подставляется автоматически
(`Clip1CMac64.dylib` для x86_64, `Clip1CMacARM64.dylib` для arm64):

```bash
# x86_64
cmake -B build64Intel -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build64Intel

# arm64
cmake -B build64Arm -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build64Arm
```

Оба бинарника автоматически подписываются ad-hoc-подписью (`codesign --sign -`
в CMake POST_BUILD). **Это обязательно:** без подписи arm64-слайс не грузится
через `dlopen` на Apple Silicon, и 1С не может создать объект компоненты. Для
публичной раздачи ad-hoc-подпись можно заменить на Developer ID + нотаризацию.

### Отладочная сборка

Замените в командах `RelWithDebInfo` на `Debug` либо передайте
`-DCMAKE_BUILD_TYPE=Debug` напрямую в CMake.

### Выпуск релиза

Релиз выпускается через git-тег, формат `vX.Y.Z`. Workflow CI
автоматически собирает все платформы и создаёт GitHub Release с
прикреплённым `AddIn.zip`.

Шаги:

```bash
# 1. Обновить версию в version.h
#    например VERSION_FULL 1.0.1.0 и компоненты VERSION_MAJOR/MINOR/REVISION/BUILD

# 2. Закоммитить bump версии
git add version.h
git commit -m "chore: bump version to 1.0.1"

# 3. Создать аннотированный тег
git tag -a v1.0.1 -m "Release 1.0.1"

# 4. Запушить коммит и тег
git push origin main
git push origin v1.0.1
```

Что произойдёт после push'а тега:
1. Workflow `Build` запустится автоматически (триггер `tags: ['v*']`).
2. Соберутся все 6 бинарников (Win32/Win64/Lin32/Lin64/Mac64/MacARM64).
3. Job `package` соберёт `AddIn.zip` с `MANIFEST.XML`.
4. Будет создан GitHub Release с тем же именем тега, в `Release notes`
   автоматически попадут commit-сообщения с предыдущего тега,
   а в Assets — `AddIn.zip` и `Clip1C-<version>.zip`.

**Пре-релизы:** тег вида `v1.0.1-rc1`, `v1.0.1-beta`, `v1.0.1-alpha`
автоматически помечается как pre-release.

**Откат тега, если что-то пошло не так:**
```bash
git tag -d v1.0.1                    # удалить локально
git push --delete origin v1.0.1      # удалить с GitHub
# Удалить созданный релиз через UI: github.com/<repo>/releases
```

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
