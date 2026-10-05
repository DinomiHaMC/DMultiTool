# Verification — Hacker Pro 2000 2.1.8

## Выполнено в окружении разработки

Дата: 2026-10-04. Target: ESP32 Dev Module / ESP32-WROOM-32, core 3.3.12, Arduino CLI 1.4.1. Исходный проект изучен до рефакторинга; baseline собран успешно. GPIO остаются прежними. Наличие исправного железа в baseline подтверждено пользователем, а не повторным стендовым тестом агента.

| Проверка | Результат |
|---|---|
| AudioCtrl host regression | Пять Consumer commands; нет отправки без подключения / при LONG OK, ошибка отправки; реальный ADC driver: LONG OK без последующего Play/Pause, короткий/длинный LEFT, повтор громкости |
| Baseline: `arduino-cli compile --fqbn esp32:esp32:esp32 .` | PASS: 1,052,656 flash bytes / 69,812 static RAM bytes |
| Полная версия, default partition | Компиляция/линковка проходят; CLI size check отклоняет >1.3 MB image |
| Полная версия 2.1.8, huge_app | PASS: 1,824,311 bytes flash (57% / 3,145,728); 108,008 bytes static RAM (32% / 327,680), 219,672 bytes оставлено для runtime |
| NFC polling regression | PASS: RFConfiguration ответ полностью считывается/проверяется до нового запроса; ответ метки через 180 ms принимается (80 ms его пропускает), no-tag/timeout, UID bounds, corrupt/truncated response и ACK failure |
| Radio memory policy 2.1.7 | Host tests: fallback при 29000 bytes, совместная работа при достаточной RAM, запрет паузы занятого WiFi, ошибка выключения WiFi, rollback после init failure, отложенный возврат WiFi после scan shutdown, сохранённая настройка OFF |
| Host Keyboard tests | PASS: все ADC 0–4095, debounce, long/short OK и LEFT, repeat, UNKNOWN, rollover |
| Classic 1K NDEF | PASS: MAD CRC 0x14, phone layout read/write, short NDEF succeeds with inaccessible later sector, 100-byte multi-sector Text, URI, TLV offsets 0/2/14/15, metadata/key/trailer preservation; no writes on size/CRC/auth/read-only failure, cancellation and corrupt read-back |
| Host NDEF / Script parser | PASS: Text/URI, malformed/truncated records, MIME, boundaries, quotes/comments/errors |
| Host DisplayManager | PASS: неизменённое меню не рисуется повторно, смена selection, четыре темы, portrait/landscape, sleep |
| Host UI | PASS: wrap, confirmation No default / explicit Yes, input edit/submit, progress cancellation |
| Fullscreen keyboard | PASS: menu state preserved, Enter submit, EN/РУ/symbol cycle, ё/Ё, Shift, UTF-8 deletion and byte bounds, complete ASCII punctuation, password masking, unchanged keyboard produces no redraw |
| Games host logic | PASS: Snake growth/reverse/collision; 100 seeds first-safe Minesweeper + win; all tetromino rotations, row clear, game over |
| Games / Python renderer | PASS: unchanged frame does not redraw, changed cells/widgets/selection, pause overlay; SVG/PNG inspected |
| Real PikaPython host | PASS: Tk Button callback → label/NEC/SD, all 3 SD examples, key binding, Canvas timer, held key, SD read; infinite loop cancellation, 20KB OOM, undefined variable, recursion limit, 10 restarts |
| Foreground exit review | Games/Python own navigation while active; LONG LEFT ignored, LONG OK opens No-default confirmation; dialog pauses execution, Yes stops Python and waits for cleanup; idle disabled while active |
| Firmware static review | No analogRead outside input driver; no delay(1000) in main UI; fillScreen only init/splash/invalidated scene |

Повторяемые команды из корня:

```bash
./tests/run.sh
./tests/run-python.sh
XDG_CACHE_HOME=/tmp/hp2000-cache ./scripts/build.sh
```

Build helper задаёт FQBN `esp32:esp32:esp32`, `PartitionScheme=huge_app` и optional commit define. Локальный partitions.csv задаёт factory app 3 MB и сохраняет NVS offset. Default CLI size limit не учитывает размер локальной partition без board option.

В ходе сборок стандартный BLE stack давал IRAM overflow. Применён NimBLE-Arduino 2.5.1; libc strftime заменён ограниченным snprintf календарных полей, после чего IRAM link проходит. В final configuration нет обещания совместимости с другой core/library version без новой сборки.

Host screenshots созданы настоящим renderer с TFT stubs; не доказывают электрическую работу ST7789. Для настоящего PikaPython на 64-bit host пик GUI/native примера ~21 KB с адаптером tkinter и заголовками выделений; это не измерение heap ESP32. Worker имеет 32KB stack, VM budget 64KB, проверяет свободный heap >=125000 и largest block >=36000 до запуска. Для Python FreeRTOS scheduling, достаточность stack, задержки SD, IR carrier и cancel latency требуют стенда.

Host tests не моделируют FreeRTOS scheduling, radio controller, PN532, SD hardware или pairing с конкретным OS. Статическая RAM из отчёта linker не включает runtime heap библиотек и стеков задач.

## NFC correction 2.1.1

При сообщении пользователя «Waiting for tag…» PN532 уже прошёл проверку available. В firmware timeout UID-поиска был 80 ms при отключённых повторах активации. В установленной [Adafruit-PN532 1.3.4](https://github.com/adafruit/Adafruit-PN532/blob/1.3.4/Adafruit_PN532.cpp) `setPassiveActivationRetries()` ждёт готовность ответа через `sendCommandCheckAck()`, но не считывает сам ответ RFConfiguration. В проект добавлен `PN532Polling`: полный 10-byte I2C ответ (status/header/LEN/TFI/response/DCS/postamble) считывается и проверяется; retries=2, timeout=500 ms вместо 80 на каждую фазу библиотеки. Используется тот же worker и I2C 0x24/SDA21/SCL22, дополнительные GPIO не требуются. Изменение применяется к UID scan и поиску метки для NDEF.

Host regression моделирует задержанный ответ/непрочитанный ответ настройки, не электрическую работу PN532. Он показывает устранение этих сценариев в коде; причина на конкретной плате ещё требует проверки обновлённым BIN. Логи `NFC Init begin=... firmware=... SAM=... RF=...` и `Scan started...` добавлены для диагностики. При аппаратной проверке перенести метку в поле **после** запуска Scan, повторить с UID 4/7 bytes и NDEF Reader, проверить отмену и повторный Scan.

## Classic NDEF 2.1.5

Tag selection считывает полный I2C frame с проверкой header/LEN/DCS/postamble и возвращает UID/SAK/ATQA. SAK с MIFARE Classic bit направляет NDEF job в Classic handler. Для совместимых Classic с некорректным SAK 00 добавлен ограниченный fallback только при ATQA 0004; Type 2 с ATQA 0044 остаётся на прежнем handler. Другие технологии получают явный отказ. Classic использует публичные MAD/NDEF Key A, затем стандартный NFC Key A; для уже выделенного MAD NDEF-sector допускается ровно один fallback factory Key A FFFFFFFFFFFF. Аутентификация использует последние 4 байта UID, затем проверяются MAD1/CRC, allocation, mapping/access flags. Записываются только существующие NDEF data blocks, с read-back каждой записи и публикацией длины в последнюю очередь. Никаких записей block 0, MAD или trailer. Карта с заводскими/нестандартными ключами без NDEF-format не форматируется автоматически; сообщение рекомендует Format NDEF телефоном. Classic 4K/Mini/custom keys не реализованы. NFC worker stack увеличен с 4KB до 8KB для preflight buffers и передачи результата; требуется стендовый stack watermark.

## Стендовый checklist после upload

AudioCtrl 2.1.8: Utils добавлен в конец registry, прежние defaultApp indices сохранены. Используется HID input report 3 / Consumer page с usages E9/EA/B5/B6/CD и отпусканием через 20 ms в BLE service. LONG OK выходит сразу, LONG LEFT игнорируется; ownsNavigation предотвращает общий возврат в launcher и idle sleep во время работы пульта. UI содержит четыре строки, подходит для обеих ориентаций. Проверены host input routing и actual ADC driver, полная сборка ESP32. На плате проверить pairing с Android/iOS, громкость/трек/PlayPause в приложении телефона, disconnect/reconnect, выход во время нажатия, 10 повторных запусков, сохранение уже включённого BLE и возврат WiFi при выходе из временного BLE. Host tests не подтверждают совместимость конкретного телефона.

BLE 2.1.7 (2026-10-05): пользователь сообщил 29000 bytes до запуска BLE. NetworkTools worker/очереди переведены на создание для одной операции: worker публикует результат и завершается, main task освобождает очередь только после публикации workerDone. Постоянный стек 12 KB и queues больше не расходуются на idle. ServiceManager при низкой RAM выключает WiFi перед BLE init, сохраняя настройку WiFi в NVS. Не прерываются активные AP/net jobs/capture. После полного BLE shutdown WiFi radio возвращается; прежнее сетевое подключение автоматически не гарантируется. Политика покрыта host regression; реальные heap после отключения WiFi, BLE scan/HID и 10 повторных сетевых операций требуют платы.

BLE 2.1.6 (2026-10-05): core 3.3.12 `initArduino()` освобождает память неиспользуемых режимов. NimBLE-Arduino 2.5.1 включает legacy `esp32-hal-bt-mem.h`, регистрирующий BLE и Classic одновременно. Firmware сохраняет BLE registration и переопределяет `btClassicInUse()` → false, поэтому ненужная Classic memory освобождается до setup / heap guard, а не внутри позднего NimBLE init. Это исправление подтверждено исходниками core/library и символами итогового ELF; точная причина ошибки пользователя и количество освобождённой RAM требуют стенда. Порог 75000 bytes сохранён, проверяется internal 8-bit heap; ошибки RAM и init разделены, добавлен Serial log heap/largest. Ошибка ensure больше не заменяется общим HID error. Неудачный boot init сохраняет пользовательское предпочтение BLE. Аппаратные boot / scan / HID / повторное включение требуют проверки на плате.

1. Boot с прежним железом, без SD и без PN532 по отдельности. Отсутствие модуля должно сообщаться, launcher остаётся доступным. Не включать Erase all flash; проверить прежние настройки NVS.
2. Все пять ADC кнопок, короткий/длинный OK/LEFT, UP/DOWN hold, UNKNOWN intervals, wake после timeout. Переключить темы/rotation, проверить текст и dirty redraw на реальном TFT.
3. Files: 65+ записей, длинное имя, directory traversal, page forward/back, rename, mkdir, No/Yes Delete на тестовом файле. Проверить работу TFT во время сохранения SD и logger rotation.
4. NFC: I2C 0x24, UID собственных ISO14443A, save/view metadata. NDEF Text/URL на собственной незаблокированной NTAG213/215/216, независимое чтение телефоном, cancel/no-tag timeout. На NDEF-форматированной Classic 1K записать Text/URL до 100 bytes, проверить чтение телефоном и обратную запись телефоном → NDEF Reader; UID 4/7 bytes, сообщения через 16/48 bytes границы. До/после сравнить MAD, block 0 и trailers. Проверить read-only, нестандартный ключ и cancel/снятие карты: успех не должен сообщаться без read-back. Locked/protected/другая неподдерживаемая модель должны отклоняться writer. Не использовать единственную важную метку.
5. IR: NEC и raw на тестовом приёмнике; измерить carrier GPIO17 при включённых UI clicks GPIO16. Sound закреплён за LEDC timer 3, IR использует auto allocation. Проверить конкретный RGB remote profile и назначенные Remote кнопки.
6. WiFi: scan/hidden/open network, пароль со спецсимволами, saved connect/forget, timeout/cancel, current IP/NTP. Ping/DNS/64 hosts/port range на собственной сети, TCP client/listener и HTTP body/chunked, cancel во время работы. AP SSID/password/channel и остановка.
7. PCAP: подключиться к своей AP, запустить запись ≤30 s, открыть в Wireshark, проверить linktype IEEE802_11 и relative timestamps, own BSSID management filter, отсутствие data frames. Повторить cancel и работу без SD в Packet Monitor.
8. BLE: scanner/device info, собственный advertisement, включение/выключение несколько раз, cancel scan → disable → enable. Pair HP2000 HID на собственном host с US layout, manual text/Enter/arrows/media, mouse move/click/scroll. Проверить punctuation `/.,`, release reports, reconnect и память при совместном WiFi.
9. Script: `examples/sd/scripts/hello.script`, current line/Cancel, DELAY не блокирует навигацию, синтаксическая ошибка, execution limits, OPEN_APP. Перепроверить отсутствие действий после выхода; начатая IR передача может закончиться в worker.
10. Tools: real ADC, heap/min/largest block, I2C scanner, Serial send/receive и reboot No/Yes. Developer levels и отсутствие WiFi password в log.
11. Games: Snake направления/пауза/столкновение, Minesweeper первый ход/двойной OK/победа, Tetris повороты/линии/ускорение. LONG LEFT не выходит; LONG OK → No возобновляет, Yes закрывает. Проверить portrait/landscape.
12. Python: скопировать `examples/sd/python` в `/python`, открыть все три примера; проверить реальные IR, SD запись/чтение и ADC события. Запустить `while True: pass`, LONG OK → No → LONG OK → Yes; повторить 10 раз, проверить heap и stack high-water mark worker. Файл >4KB отклоняется; GUI с >12 виджетами ограничен. Проверить недостаток RAM с BLE/WiFi и длительное удержание кнопок.
13. Idle: display timeout, первая кнопка только будит, >5 minutes idle с WiFi/BLE OFF без pending jobs. Light sleep через timer опрашивает ADC; backlight остаётся запитан, поскольку BL GPIO отсутствует.

Эти аппаратные проверки **ещё не выполнены** в этом окружении. Firmware не была загружена автоматически.
