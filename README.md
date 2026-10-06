# DMultiTool 3.1

Прошивка существующего ESP32 DevKit / ESP32-WROOM-32 с ST7789 240×320,
пятью кнопками ADKeyboard, microSD, PN532 и ИК-передатчиком.
Лицензия собственного кода — **GPL-3.0-only**: [LICENSE](LICENSE),
[лицензии зависимостей](NOTICE.md).

## Управление

Меню — сетка **3 столбца × 4 строки**, до 12 плиток на странице.
Стрелки перемещают выделение; **OK** открывает пункт;
**удержание OK** возвращает назад, в том числе закрывает клавиатуру.
В длинном меню DOWN/UP переходят между страницами. LEFT больше не служит
кнопкой возврата. В текстовой клавиатуре удаление выполняется иконкой Backspace,
Enter подтверждает ввод; Lang переключает EN / RU / символы.
Диалоги, журналы и результаты остаются читаемыми списками.

На главном экране: WiFi, Bluetooth, NFC, Infrared, Files, Scripts,
Tools, Settings, Games, Utils. About, SubGHz и 2.4GHz удалены.
В **Scripts** две кнопки: Scripts и Python. Старые файлы `/scripts/*.script`
и `/python/*`, исполнители и API сохранены. Игры и Python, как прежде,
выходят через удержание OK и подтверждение; AudioCtrl и калькулятор — сразу.

## Игры

Games содержит Snake, Minesweeper, Tetris, 2048, Pong, Breakout, FlappyBird,
Dino Runner, Space Invaders и Asteroids. Удержание OK открывает подтверждение
выхода; No продолжает игру. После победы/поражения короткий OK запускает заново.

| Игра | Управление |
|---|---|
| 2048 | Стрелки сдвигают плитки; одинаковые объединяются один раз за ход. Цель — 2048 |
| Pong | UP/DOWN — ракетка, OK — пауза. Матч с компьютером до пяти очков |
| Breakout | LEFT/RIGHT — ракетка, OK — пауза. Все кирпичи, три жизни |
| FlappyBird | UP или короткий OK — взмах, проходы между трубами дают очки |
| Dino Runner | UP или короткий OK — прыжок, удержание DOWN — пригнуться под птицей |
| Space Invaders | LEFT/RIGHT — корабль, удержание UP — стрельба; короткий OK также стреляет. Волны и три жизни |
| Asteroids | LEFT/RIGHT — вращение, UP — тяга, DOWN — стрельба; короткий OK также стреляет. Инерция, переход через края, дробление астероидов |

Динамичные игры обновляются с фиксированным шагом 50 ms; дополнительные
буферы экрана и динамические выделения в игровой логике не нужны.

## Utils

- **Morse**: любая стрелка включает тон 700 Hz на время удержания.
  Короткое нажатие даёт точку, длинное — тире; отпускание выключает звук.
  OK закрывает утилиту без подтверждения. Звук ручной, автоматического
  кодирования/декодирования нет. Режим явно включает зуммер независимо от
  настройки UI sound; системные писки временно блокируются.

- **AudioCtrl**: сопряжение с `DMultiTool HID` в настройках телефона.
  UP/DOWN — громкость, RIGHT/LEFT — следующий/предыдущий трек,
  короткий OK — Play/Pause, удержание OK — выход.
- **Calculator**: отдельная клавиатура. SCI/BASIC переключают режим,
  DEG/RAD — угловые единицы. Доступны скобки, степени, проценты, factorial,
  sin/cos/tan, обратные функции, sqrt, ln/log, exp, abs, pi, e и ans.
  `=` считает, DEL удаляет символ, AC очищает. Удержание OK закрывает.
- **File transfer**: чтение/запись microSD с ПК по BLE, CRC32 и подтверждение
  каждого блока. Конечное имя появляется только после проверки всей загрузки.
- **Notifications**: Android-мост уведомлений; Receiver включает приём,
  Global popups показывает сообщения поверх любого приложения,
  Wake screen разрешает включить экран при уведомлении. Последнее сообщение
  доступно через Latest. Выключенный Wake screen сохраняет затухший экран.

При нехватке RAM запуск BLE временно выключает незанятый WiFi. Сохранённая
настройка WiFi не меняется. После выключения временного BLE радиомодуль WiFi
возвращается; сеть при необходимости подключите снова через Saved Networks.
Активные AP, сетевые задачи, downloads и capture не прерываются автоматически.

## Files → Open MtSh

Кнопка **Open MtSh** есть в каждой папке. Оболочка открывается с её рабочим
каталогом. OK на Command открывает клавиатуру; результат показан ниже.
Удержание OK возвращает к файлам. Путь может быть абсолютным или относительным;
имена с пробелами заключайте в одинарные/двойные кавычки.
Это собственная оболочка файлового менеджера, без запуска системных команд.

| Команда | Действие |
|---|---|
| `help`, `pwd`, `exit` | Справка, рабочая папка, выход |
| `ls [path] [offset]` | Список; страницы по 64 записи |
| `cd path`, `open path` | Сменить каталог / открыть файл или каталог в UI |
| `cat file [offset]`, `info path` | Постраничный текст / сведения |
| `mkdir path`, `touch file` | Создать каталог / новый пустой файл |
| `write file "text"`, `append file "text"` | Записать / дописать текст |
| `cp source target` | Копировать файл до 1 MiB в новое имя |
| `mv source target`, `rename source target` | Переименовать, без перезаписи назначения |
| `rm file`, `rmdir path` | Удалить файл / пустую папку |
| `wget URL file`, `download URL file` | Скачать в новое имя на microSD |
| `cancel` | Отменить download |

Пример: `wget "https://example.org/file.txt" "/downloads/file.txt"`.
Папка назначения должна существовать: `mkdir /downloads`.
Downloads требуют подключённого WiFi; максимум 16 MiB, тело до 120 секунд.
HTTP/HTTPS streaming выполняется в worker, SD записывает основная задача.
Используется временный `.mtpart`, при ошибке/отмене он удаляется.
HTTPS проверяет сертификаты: встроены ISRG Root X1/X2 (Let's Encrypt).
Для другого CA поместите доверенные корневые сертификаты PEM в
`/config/ca.pem` (до 12 KB); этот файл заменяет встроенные CA.
Для проверки дат сертификатов нужен корректный часовой источник/NTP;
подключение через WiFi UI запускает NTP. Нет insecure TLS fallback.

В UI файл открывает меню Open / Info / Rename / Delete / Open MtSh;
папки имеют Folder actions. Бинарные файлы не отображаются текстовым viewer.

## Bluetooth: файлы на ПК

У ESP32-WROOM-32 нет native USB mass storage; USB-разъём обычной DevKit работает
через UART-мост. Реализована предусмотренная альтернатива — BLE.

1. На DMultiTool откройте **Utils → File transfer**.
2. Выполните сопряжение `DMultiTool HID` на ПК (запись требует шифрования).
3. Установите клиент и зависимости:

```bash
python3 -m venv .venv
.venv/bin/pip install -r tools/requirements.txt
.venv/bin/python tools/mtble.py ls /
.venv/bin/python tools/mtble.py get /python/buttons.py buttons.py
.venv/bin/python tools/mtble.py put demo.py /python/demo.py
.venv/bin/python tools/mtble.py ls /python --offset 64
```

Windows: команды `Scripts/python.exe` вместо `.venv/bin/python`.
Если устройств несколько, задайте `--address` перед командой.
Bluetooth Low Energy медленнее USB; размер файла ограничен 16 MiB,
метаданные команды — 199 UTF-8 bytes. Перезапись существующего файла запрещена.
Передача требует открытой утилиты; удержание OK останавливает её.
Notifications используют тот же BLE service, но не разрешают файловые операции
вне File transfer.

## Android: уведомления

Готовый APK: `build/android/DMultiTool-Notifications.apk`.
Исходники и воспроизводимая сборка: [companion/android](companion/android).

1. В Utils → Notifications включите **Receiver**.
2. Сопрягите телефон с `DMultiTool HID`.
3. Установите APK, разрешите Bluetooth и доступ к уведомлениям,
   выберите DMultiTool в приложении и нажмите «Отправить тест».
4. На устройстве настройте Global popups и отдельно Wake screen.

Приложение Android 8+ использует NotificationListenerService и BLE,
не требует интернета и не передаёт уведомления на сервер.
Постоянные ongoing-уведомления отфильтрованы; одинаковые обновления не дублируются.
Текст ограничен 199 UTF-8 bytes и согласованным MTU. Содержимое, которое Android
скрывает от notification listeners, получить нельзя. APK подписан ключом
разработческой сборки; ключ не включён в Git. Автоматическое переподключение
реализовано на обеих сторонах; работа с конкретным телефоном требует стенда.

## Display

Interface объединён с Display. Здесь доступны rotation, timeout, animations,
menu wrap, status bar, темы и screensaver. Темы: Bruce-like, Dark, Light,
Green terminal, Midnight, Solarized, Amber, Ocean и Custom.
Редактор меняет семь RGB565 цветов и сохраняет их в NVS; Copy current theme
создаёт основу Custom. Скринсейверы: Off, Pipes, Cosmos, Matrix;
они запускаются после Screen timeout. Первая кнопка будит экран.
Регулировка яркости недоступна: отдельный GPIO подсветки не подключён.

## Сохранённые функции

WiFi scanner, соединения/AP, DNS/ping/hosts/ports/TCP/HTTP и ограниченный PCAP;
BLE scanner/advertiser/keyboard/mouse; UID/NDEF NFC, IR presets/NEC/raw;
десять игр; инструменты ADC/heap/Serial/I2C и настройки.
NDEF writer поддерживает NTAG213/215/216 и уже NDEF-форматированную Classic 1K,
публичные ключи и необходимые выделенные MAD data sectors. Ключи, MAD и
trailers автоматически не заменяются. Физическая работа требует проверки.
Python — PikaPython с небольшим TFT tkinter adapter, кнопками, IR и microSD;
источник до 4 KB, VM до 64 KB, worker 32 KB. Нужны >=125 KB свободного heap
и непрерывный блок >=36 KB; desktop Tcl/Tk отсутствует.

## Железо и сборка

GPIO не изменены: SPI SCK18/MISO19/MOSI23, TFT CS5/DC27/RST26, SD CS13,
PN532 I2C SDA21/SCL22, клавиатура ADC36, buzzer16, IR17.
Target: Arduino ESP32 core **3.3.12**, Arduino CLI 1.4.1,
NimBLE-Arduino 2.5.1, Adafruit GFX 1.12.6, ST7789 1.11.0, PN532 1.3.4,
IRremote 4.7.1. 4 MB flash, factory app partition 3 MB, без OTA/PSRAM.

```bash
./tests/run.sh
./tests/run-python.sh
python3 tests/mtble_test.py
XDG_CACHE_HOME=/tmp/hp2000-cache ./scripts/build.sh --export-binaries
DMULTITOOL_ANDROID_SDK=/path/to/android-sdk bash companion/android/build.sh
```

Приложение: **build/DMultiTool.bin**, записывать по **0x10000**.
Arduino sketch и папка сохраняют прежнее имя для совместимости сборки.
NVS namespace сохранён; старые defaultApp indices мигрируют один раз.
Логи сборки/тестов и ограничения проверки описаны в [docs/verification.md](docs/verification.md).

Основа UI/UX вдохновлена переносными мультитулами, включая Bruce; приложения и
renderer — собственные. Библиотеки сторонних авторов сохраняют свои лицензии.
