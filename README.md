# iot-http-adapter

Небольшой HTTP-сервис для IoT-платформы на C++ и [Drogon](https://github.com/drogonframework/drogon). Зависимости управляются через **vcpkg в manifest mode**: Drogon не хранится в репозитории и автоматически восстанавливается при конфигурации CMake.

## Что нужно установить

На macOS выполните один раз:

```bash
xcode-select --install
brew install cmake ninja pkg-config git
```

Проверьте, что инструменты доступны:

```bash
cmake --version
ninja --version
pkg-config --version
git --version
```

На Linux понадобятся C++-компилятор, CMake, Ninja, Git и `pkg-config`. Например, в Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config git curl zip unzip tar
```

На Windows нужны Git и Visual Studio с компонентом **Desktop development with C++** (включая CMake Tools).

## Установка vcpkg

vcpkg устанавливается один раз на компьютер, а не внутрь этого репозитория. Например, на macOS/Linux:

```bash
mkdir -p "$HOME/Documents/Utils"
git clone https://github.com/microsoft/vcpkg.git "$HOME/Documents/Utils/vcpkg"
"$HOME/Documents/Utils/vcpkg/bootstrap-vcpkg.sh"
```

На Windows:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
```

Затем укажите путь к vcpkg в переменной окружения. На macOS/Linux с zsh:

```bash
echo 'export VCPKG_ROOT="$HOME/Documents/Utils/vcpkg"' >> ~/.zshrc
source ~/.zshrc
```

Проверьте путь:

```bash
echo "$VCPKG_ROOT"
"$VCPKG_ROOT/vcpkg" version
```

На Windows в PowerShell для текущего окна:

```powershell
$env:VCPKG_ROOT = "C:/dev/vcpkg"
```

## Сборка и запуск

Клонируйте проект и перейдите в его каталог:

```bash
git clone git@github.com:<organization>/iot-http-adapter.git
cd iot-http-adapter
```

Сконфигурируйте Debug-сборку:

```bash
cmake --preset debug
```

Во время первой конфигурации CMake запустит vcpkg. Тот прочитает `vcpkg.json`, скачает и соберёт Drogon и его зависимости. Это занимает заметное время только при первом запуске.

Соберите программу:

```bash
cmake --build --preset debug
```

Запустите сервер:

```bash
./build/debug/iot-http-adapter
```

Сервис слушает `127.0.0.1:8080`. Остановить его можно сочетанием `Ctrl+C`.

Если на машине используется локальный preset `debug-local`, команды будут такими:

```bash
cmake --preset debug-local
cmake --build --preset debug-local
./build/debug-local/iot-http-adapter
```

## Как устроены зависимости

- `vcpkg.json` перечисляет прямые зависимости проекта, сейчас это `drogon`.
- `vcpkg-configuration.json` фиксирует baseline реестра vcpkg, чтобы команда и CI получали согласованные версии пакетов.
- `CMakePresets.json` подключает vcpkg toolchain и определяет Debug/Release-сборки.
- `CMakeLists.txt` находит Drogon через `find_package(Drogon CONFIG REQUIRED)` и линкует цель с `Drogon::Drogon`.

Чтобы добавить новую библиотеку, используйте:

```bash
"$VCPKG_ROOT/vcpkg" add port <имя-пакета>
```

После этого коммитятся изменения `vcpkg.json` и, если он изменился, `vcpkg-configuration.json`.

## Что не нужно коммитить или добавлять в ZIP

Не включайте следующие локально генерируемые файлы и каталоги:

```text
build/
vcpkg_installed/
CMakeUserPresets.json
.git/
```

Каталог `build/` содержит бинарник и скомпилированные Drogon-зависимости, поэтому может занимать сотни мегабайт. Он не является исходным кодом: после клонирования он воспроизводится командами сборки выше. `.gitignore` исключает эти файлы из Git, но при создании ZIP их нужно исключать вручную.
