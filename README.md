## Clip1C — внешняя компонента 1С для работы с буфером обмена

Внешняя компонента 1С с поддержкой Native API. Обеспечивает чтение и запись
текста, файлов и изображений в буфер обмена.

Поддерживаемые платформы:
- Windows (x86, x64)
- Linux (x86, x64)
- macOS (x86_64, arm64)

### Возможности

- Чтение/запись текста
- Чтение/запись списка файлов
- Чтение/запись изображений (PNG)
- Очистка буфера обмена
- Получение списка форматов, доступных в буфере обмена
- Мониторинг изменений буфера обмена (только Windows)

### Сборка

Сборка автоматизирована через [GitHub Actions](.github/workflows/build.yml):
при каждой отправке изменений параллельно собираются бинарники под Windows (x86/x64),
Linux (x86/x64) и macOS (universal x86_64+arm64), после чего пакуются в
единый `AddIn.zip` с `MANIFEST.XML`. На релизах ZIP прикрепляется к Assets.

Подробности про локальную сборку — [Docs/BuildLibrary.md](Docs/BuildLibrary.md).

### Использование

См. [Docs/ClipboardControl.md](Docs/ClipboardControl.md).

### Происхождение и лицензия

Проект создан на базе библиотеки [VanessaExt](https://github.com/lintest/VanessaExt). Из исходной библиотеки оставлена только
подсистема буфера обмена, добавлена поддержка сборки под macOS.

Распространяется под лицензией **BSD 3-Clause** — см. файл [LICENSE](LICENSE).
Авторские права на исходный код VanessaExt сохранены.

При разработке также использовались:
- [Clip Library](https://github.com/dacap/clip) by David Capello — буфер обмена под Linux (MIT)
- [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann — сериализация JSON (MIT)
