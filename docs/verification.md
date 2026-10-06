# Verification — DMultiTool 3.2.1

Дата: 2026-10-06. Target: ESP32 Dev Module / ESP32-WROOM-32, Arduino core 3.3.12, Arduino CLI 1.4.1, NimBLE-Arduino 2.5.1. GPIO сохранены. Наличие рабочего оборудования подтверждено пользователем для прежней прошивки; новая версия проверена сборкой и host-тестами.

## Выполненные проверки

| Проверка | Результат |
|---|---|
| Полная сборка ESP32 / huge_app | PASS; итоговые размеры приведены ниже |
| Сетка меню | PASS: перемещения по 3 столбцам, переход между страницами по 12 плиток, неполные строки, 1–70 элементов, wrap; долгий OK возвращает назад |
| DisplayManager | PASS: dirty redraw, портрет/альбом, клавиатура, инженерный калькулятор, screensavers; SVG получены настоящим renderer с TFT stubs и осмотрены |
| Calculator | PASS: приоритет операций, степени, факториал, проценты, DEG/RAD, функции, ans, пределы глубины/длины, ошибки области определения |
| MtSh | PASS: кавычки/escape, пределы команд, относительные и абсолютные пути, запрет выхода выше корня |
| DownloadService | PASS: настоящий код с HTTP/SD/FreeRTOS stubs; известная/неизвестная длина, пустой файл, HTTP ошибки, отмена, SD full, ошибки queue/task, CA/RAM/WiFi preflight, освобождение клиентских объектов; сохранение файла, появившегося во время загрузки |
| FileBridge | PASS: настоящий код с SD/transport stubs; upload/download, известный CRC32, пустой файл, запрет перезаписи, ошибочный CRC/размер, SD full, обрыв связи и очистка временного файла, list, уведомления |
| PC client | PASS: три async-теста tools/mtble.py с transport stubs; CRC, повреждённые/пустые файлы, ACK, upload, list, фрагментация, сохранение существующих файлов |
| Android companion | PASS: javac, D8, aapt, zipalign; debug APK подписан, проверен apksigner. API 35 / build-tools 35, minSdk 26 |
| Существующие функции | PASS: ADC/debounce/hold/repeat, UI/dialogs/keyboard, AudioCtrl, NFC polling, Classic 1K NDEF, scripts, игры, radio memory policy |
| Настоящий PikaPython | PASS: три SD-примера, Tk callbacks, кнопки, IR/SD adapters, Canvas/timers, отмена бесконечного цикла, OOM/recursion, десять перезапусков |
| Shell/Python syntax и whitespace | PASS: bash -n, py_compile, git diff --check |

Повторяемые команды из корня:

```bash
./tests/run.sh
./tests/run-python.sh
python3 tests/mtble_test.py
XDG_CACHE_HOME=/tmp/hp2000-cache ./scripts/build.sh --jobs 2 --export-binaries
DMULTITOOL_ANDROID_SDK=/tmp/dmt-android-sdk bash companion/android/build.sh
```

Build helper задаёт FQBN esp32:esp32:esp32, PartitionScheme=huge_app и FW_COMMIT из Git. Экспортированный образ: build/DMultiTool.bin, application offset 0x10000. Для Android нужны JDK и Android SDK; SDK использован из /tmp, системная установка не менялась.

Linker-отчёт: 1,995,003 bytes flash (63% из 3,145,728), 113,832 bytes static RAM (34% из 327,680). Это не измерение свободного heap при включённых WiFi/Bluetooth/Python; библиотеки и стеки выделяют память во время работы. Итоговый образ повторно собирается после коммита, чтобы FW_COMMIT соответствовал исходникам.

Host-модели HTTP и Bluetooth не подтверждают реальное TLS-соединение, radio scheduling, pairing, дальность или пропускную способность. Android APK собран, но на телефоне не установлен. SVG renderer не проверяет электрическую работу TFT. Firmware в ESP32 автоматически не загружалась.

## Игры и Морзе — 3.1

Host regression проверяет настоящий ArcadeModels.cpp: слияния без двойного
merge, отсутствие spawn при неудачном ходе, 2048/win и заполненный тупик;
Pong bounce/goals/match; Breakout brick hit/paddle/lives/win;
Flappy flap/pipe/score; Dino jump/duck/bird/cactus; Invaders shots/lives/waves;
Asteroids inertia/wrap/split/lives. Дополнительно 2,000 шагов каждой
динамичной игры с детерминированными управлениями и проверкой границ sprites.

Renderer проверен для всех семи новых игр в portrait/landscape, без повторной
отрисовки неизменённого кадра; SVG сохранены в docs/screenshots/arcade-*.
Просмотрены экраны 2048, Invaders и Asteroids. Это визуализация с TFT stubs.

Настоящий Buzzer.cpp проверен с tone stubs: 700 Hz остаётся включённым при
долгом удержании и повторном key(true), системные beep не прерывают сигнал,
release/endKey выключают тон, повторный вход и возврат к UI beep работают.
IR timer channel 6 сохранён. Реальные акустика, задержка ADC/SD/BLE и fps
требуют оборудования. Проверить все четыре стрелки, короткий/длинный OK,
выход во время звука и отсутствие тона после закрытия.

## Исправление мигания — 3.1.1

Новый GameRenderer собирает изменённые области в статическом RGB565 patch
16×16 (512 bytes), без выделения полного framebuffer из heap. Стирание
старого объекта и отрисовка всех пересекающихся объектов происходят в RAM;
TFT получает готовый patch одним drawRGBBitmap. Учитываются удаление/сдвиг
индексов sprites, поворот, обрезка по краям и неполная последняя колонка.
Механизм применяется ко всем десяти играм. Счёт обновляется при изменении,
подсказки и рамка — при открытии/смене состояния.

PASS: game_render_test сравнивает пиксели incremental и full composition
для десяти игр в обеих ориентациях, включая 2048, cursor Minesweeper,
пересечения, удаление, поворот и clipping спрайтов. Нет прямой очистки
игрового поля между кадрами; неизменённый кадр не отправляется повторно,
движение мяча Breakout не перерисовывает кирпичи. Bitmap/Canvas stubs
проверяют последовательность операций и сборку пикселей; реальный ST7789,
SPI timing и визуальное отсутствие мигания требуют проверки новым BIN.

## Новые проверки на оборудовании

1. Launcher и вложенные меню: 3×4 плитки, все направления, переход страниц, удержание OK; проверить восстановление старых настроек/default app и отсутствие удалённых вкладок.
2. MtSh: открыть из разных папок, все команды на тестовых файлах, quoted filenames; HTTP/HTTPS, chunked response, отмена, SD full. HTTPS требует правильных часов и CA; встроены ISRG X1/X2, другие roots можно положить в /config/ca.pem.
3. Scripts/Python: оба раздела открываются из Scripts, OPEN_APP Python сохраняет прежний смысл; запуск/остановка и возврат в общий раздел.
4. Calculator: все клавиши, базовый/инженерный режим, DEG/RAD, ошибки выражений, обе ориентации.
5. Темы: все девять вариантов, изменение семи цветов Custom, сохранение после reboot; Pipes/Cosmos/Matrix и первая кнопка после сна.
6. File transfer: pairing ПК, ls/get/put через tools/mtble.py, двоичные и пустые файлы, CRC, отключение/повторное подключение. WROOM-32 не поддерживает USB mass storage через USB-UART; используется BLE.
7. Android: выбрать сопряжённый DMultiTool HID, разрешить доступ к уведомлениям, тестовое и реальное уведомления; Receiver/Global popups/Wake screen, уведомления во время игр/Python/калькулятора и при потухшем экране. Проверить переподключение и фоновые ограничения конкретного телефона.
8. Совместная работа WiFi/BLE, запуск Python с проверкой heap, повторные переключения, AudioCtrl после уведомлений и передач. Новые очереди и GATT service расходуют runtime RAM; реальные heap/stack watermark требуют платы.

Ниже сохранены сведения о прежних исправлениях NFC/BLE и проверки остальных функций.

## NFC correction 2.1.1

При сообщении пользователя «Waiting for tag…» PN532 уже прошёл проверку available. В firmware timeout UID-поиска был 80 ms при отключённых повторах активации. В установленной [Adafruit-PN532 1.3.4](https://github.com/adafruit/Adafruit-PN532/blob/1.3.4/Adafruit_PN532.cpp) `setPassiveActivationRetries()` ждёт готовность ответа через `sendCommandCheckAck()`, но не считывает сам ответ RFConfiguration. В проект добавлен `PN532Polling`: полный 10-byte I2C ответ (status/header/LEN/TFI/response/DCS/postamble) считывается и проверяется; retries=2, timeout=500 ms вместо 80 на каждую фазу библиотеки. Используется тот же worker и I2C 0x24/SDA21/SCL22, дополнительные GPIO не требуются. Изменение применяется к UID scan и поиску метки для NDEF.

Host regression моделирует задержанный ответ/непрочитанный ответ настройки, не электрическую работу PN532. Он показывает устранение этих сценариев в коде; причина на конкретной плате ещё требует проверки обновлённым BIN. Логи `NFC Init begin=... firmware=... SAM=... RF=...` и `Scan started...` добавлены для диагностики. При аппаратной проверке перенести метку в поле **после** запуска Scan, повторить с UID 4/7 bytes и NDEF Reader, проверить отмену и повторный Scan.

## Classic NDEF 2.1.5

Tag selection считывает полный I2C frame с проверкой header/LEN/DCS/postamble и возвращает UID/SAK/ATQA. SAK с MIFARE Classic bit направляет NDEF job в Classic handler. Для совместимых Classic с некорректным SAK 00 добавлен ограниченный fallback только при ATQA 0004; Type 2 с ATQA 0044 остаётся на прежнем handler. Другие технологии получают явный отказ. Classic использует публичные MAD/NDEF Key A, затем стандартный NFC Key A; для уже выделенного MAD NDEF-sector допускается ровно один fallback factory Key A FFFFFFFFFFFF. Аутентификация использует последние 4 байта UID, затем проверяются MAD1/CRC, allocation, mapping/access flags. Записываются только существующие NDEF data blocks, с read-back каждой записи и публикацией длины в последнюю очередь. Никаких записей block 0, MAD или trailer. Карта с заводскими/нестандартными ключами без NDEF-format не форматируется автоматически; сообщение рекомендует Format NDEF телефоном. Classic 4K/Mini/custom keys не реализованы. NFC worker stack увеличен с 4KB до 8KB для preflight buffers и передачи результата; требуется стендовый stack watermark.

## Стендовый checklist после upload

AudioCtrl 2.1.8: Utils добавлен в конец registry, прежние defaultApp indices сохранены. Используется HID input report 3 / Consumer page с usages E9/EA/B5/B6/CD и отпусканием через 20 ms в BLE service. LONG OK выходит сразу, LONG LEFT игнорируется; ownsNavigation предотвращает общий возврат в launcher и idle sleep во время работы пульта. AudioCtrl сохраняет четыре строки; остальные меню используют сетку 3×4. Проверены host input routing и actual ADC driver, полная сборка ESP32. На плате проверить pairing с Android/iOS, громкость/трек/PlayPause в приложении телефона, disconnect/reconnect, выход во время нажатия, 10 повторных запусков, сохранение уже включённого BLE и возврат WiFi при выходе из временного BLE. Host tests не подтверждают совместимость конкретного телефона.

BLE 2.1.7 (2026-10-05): пользователь сообщил 29000 bytes до запуска BLE. NetworkTools worker/очереди переведены на создание для одной операции: worker публикует результат и завершается, main task освобождает очередь только после публикации workerDone. Постоянный стек 12 KB и queues больше не расходуются на idle. ServiceManager при низкой RAM выключает WiFi перед BLE init, сохраняя настройку WiFi в NVS. Не прерываются активные AP/net jobs/capture. После полного BLE shutdown WiFi radio возвращается; прежнее сетевое подключение автоматически не гарантируется. Политика покрыта host regression; реальные heap после отключения WiFi, BLE scan/HID и 10 повторных сетевых операций требуют платы.

BLE 2.1.6 (2026-10-05): core 3.3.12 `initArduino()` освобождает память неиспользуемых режимов. NimBLE-Arduino 2.5.1 включает legacy `esp32-hal-bt-mem.h`, регистрирующий BLE и Classic одновременно. Firmware сохраняет BLE registration и переопределяет `btClassicInUse()` → false, поэтому ненужная Classic memory освобождается до setup / heap guard, а не внутри позднего NimBLE init. Это исправление подтверждено исходниками core/library и символами итогового ELF; точная причина ошибки пользователя и количество освобождённой RAM требуют стенда. Порог 75000 bytes сохранён, проверяется internal 8-bit heap; ошибки RAM и init разделены, добавлен Serial log heap/largest. Ошибка ensure больше не заменяется общим HID error. Неудачный boot init сохраняет пользовательское предпочтение BLE. Аппаратные boot / scan / HID / повторное включение требуют проверки на плате.

1. Boot с прежним железом, без SD и без PN532 по отдельности. Отсутствие модуля должно сообщаться, launcher остаётся доступным. Не включать Erase all flash; проверить прежние настройки NVS.
2. Все пять ADC кнопок, короткий/длинный OK/LEFT, UP/DOWN hold, UNKNOWN intervals, wake после timeout. Переключить темы/rotation, проверить текст и dirty redraw на реальном TFT.
3. Files: 65+ записей, длинное имя, directory traversal, page forward/back, rename, mkdir, No/Yes Delete на тестовом файле. Проверить работу TFT во время сохранения SD и logger rotation.
4. NFC: I2C 0x24, UID собственных ISO14443A, save/view metadata. NDEF Text/URL на собственной незаблокированной NTAG213/215/216, независимое чтение телефоном, cancel/no-tag timeout. На NDEF-форматированной Classic 1K записать Text/URL до 100 bytes, проверить чтение телефоном и обратную запись телефоном → NDEF Reader; UID 4/7 bytes, сообщения через 16/48 bytes границы. До/после сравнить MAD, block 0 и trailers. Проверить read-only, нестандартный ключ и cancel/снятие карты: успех не должен сообщаться без read-back. Locked/protected/другая неподдерживаемая модель должны отклоняться writer. Не использовать единственную важную метку.
5. IR: NEC и raw на тестовом приёмнике; измерить carrier GPIO17 при включённых UI clicks GPIO16. Sound закреплён за LEDC timer 3, IR использует auto allocation. Проверить конкретный RGB remote profile и назначенные Remote кнопки.
6. WiFi: scan/hidden/open network, пароль со спецсимволами, saved connect/forget, timeout/cancel, current IP/NTP. Ping/DNS/64 hosts/port range на собственной сети, TCP client/listener и HTTP body/chunked, cancel во время работы. AP SSID/password/channel и остановка.
7. PCAP: подключиться к своей AP, запустить запись ≤30 s, открыть в Wireshark, проверить linktype IEEE802_11 и relative timestamps, own BSSID management filter, отсутствие data frames. Повторить cancel и работу без SD в Packet Monitor.
8. BLE: scanner/device info, собственный advertisement, включение/выключение несколько раз, cancel scan → disable → enable. Pair DMultiTool HID на собственном host с US layout, manual text/Enter/arrows/media, mouse move/click/scroll. Проверить punctuation `/.,`, release reports, reconnect и память при совместном WiFi.
9. Script: `examples/sd/scripts/hello.script`, current line/Cancel, DELAY не блокирует навигацию, синтаксическая ошибка, execution limits, OPEN_APP. Перепроверить отсутствие действий после выхода; начатая IR передача может закончиться в worker.
10. Tools: real ADC, heap/min/largest block, I2C scanner, Serial send/receive и reboot No/Yes. Developer levels и отсутствие WiFi password в log.
11. Games: Snake направления/пауза/столкновение, Minesweeper первый ход/двойной OK/победа, Tetris повороты/линии/ускорение. LONG LEFT не выходит; LONG OK → No возобновляет, Yes закрывает. Проверить portrait/landscape.
12. Python: скопировать `examples/sd/python` в `/python`, открыть все три примера; проверить реальные IR, SD запись/чтение и ADC события. Запустить `while True: pass`, LONG OK → No → LONG OK → Yes; повторить 10 раз, проверить heap и stack high-water mark worker. Файл >4KB отклоняется; GUI с >12 виджетами ограничен. Проверить недостаток RAM с BLE/WiFi и длительное удержание кнопок.
13. Idle: display timeout, первая кнопка только будит, >5 minutes idle с WiFi/BLE OFF без pending jobs. Light sleep через timer опрашивает ADC; backlight остаётся запитан, поскольку BL GPIO отсутствует.

Эти аппаратные проверки **ещё не выполнены** в этом окружении. Firmware не была загружена автоматически.

## Media — 3.2

Настоящий TJpgDec и MediaDecoder проверены на собственных JPEG fixtures:
baseline RGB/grayscale, odd dimensions, downscale, оба положения экрана,
APP metadata с вложенными маркерами, отказ для progressive/CMYK,
обрыв файла, ошибочные сегменты, отмена и 500 детерминированных мутаций.
Проверены BMP 24/32 bit, top-down/bottom-up, padding, scaling, границы;
MJPEG EOF, multipart separators, seek и rollover кэша 64 кадров.

До аппаратной проверки остаются скорость SD/TFT, удержание OK во время
декодирования, переход между Files/Media и возврат ориентации экрана,
пауза/seek/loop, уведомления поверх фото и восстановление изображения.
Видеозвук и контейнеры MP4/AVI на устройстве не поддерживаются.

ASan/UBSan: PASS для MediaDecoder/TJpgDec и JPEG мутаций после расширения
IDCT/dequantization арифметики до int64_t. LeakSanitizer отключён: среда
запуска использует ptrace и не поддерживает его. Проверка утечек этим
запуском не подтверждается.

Media DisplayManager: PASS, декодированные RGB565 блоки переданы настоящему
renderer с TFT stubs. Проверены границы viewport, оба положения экрана,
отсутствие очистки при следующем кадре того же размера, dirty HUD;
SVG media-photo/media-landscape/media-video осмотрены.

PC converter: PASS, настоящий ffmpeg: MP4 → MJPEG, BMP/JPEG, .fps,
неверные параметры/вход, сохранение существующих файлов и очистка временных.
Команда: `python3 tests/media_convert_test.py` (нужен ffmpeg).

## BLE Scanner — 3.2.1

Пассивный scan заменён на active scan 6 s, отключён фильтр повторов для
получения обновлений advertising/scan response. Включена явная проверка
SDK start, вывод ошибок запуска/выделения task RAM и адрес вместо Unnamed.
Host-тест BLEScanSession проверяет completion, повторный запуск, отказ SDK
start, отмену до task startup, во время работы и гонку с SDK start. Копирование результатов ждёт
callback onScanEnd, чтобы получить последние scan responses; задержка
callback после остановки контроллера проверена отдельным сценарием.
Новый режим и получение имён требуют проверки на реальном BLE advertiser;
Classic discovery не добавлен.
