# DMultiTool BLE bridge v1

Сервис `6e400001-b5a3-f393-e0a9-e50e24dcca9e` добавляется к HID GATT server.
RX: `6e400002-b5a3-f393-e0a9-e50e24dcca9e`, Write with response,
требуется шифрованное соединение. TX: `6e400003-b5a3-f393-e0a9-e50e24dcca9e`,
Notify. Сначала выполнить сопряжение с DMultiTool HID.

Первый байт — opcode. RX до 200 bytes, TX до 20 bytes. Метаданные UTF-8,
разделитель полей TAB. Значения length/CRC в бинарных ответах — uint32 LE.
Файловые команды принимаются только пока открыта Utils → File transfer.
Максимум файл 16 MiB, пути нормализуются относительно корня SD.

| RX opcode | Payload | Ответ |
|---|---|---|
| 0x01 | path TAB offset | Строки D/F TAB size TAB name; MORE TAB next-offset при наличии следующей страницы; END |
| 0x02 | path | SIZE TAB length, затем данные и итоговый CRC |
| 0x03 | path TAB length | READY; имя назначения должно быть новым |
| 0x04 | до 199 bytes upload data | ACK после записи на SD |
| 0x05 | пусто | ACK очередного download блока |
| 0x06 | CRC32 LE | DONE после проверки размера/CRC и rename, либо ERR |
| 0x30 | notification text UTF-8 | Принимается при включённом Receiver, без файловых операций |

| TX opcode | Payload |
|---|---|
| 0x20 | Фрагмент текстового ответа до 19 bytes; собрать до LF |
| 0x10 | Download data до 19 bytes; клиент обязан ответить 0x05 |
| 0x11 | 4 bytes length + 4 bytes CRC32; завершение download |

CRC — стандартный CRC32 IEEE, как zlib.crc32; пустой файл имеет CRC 0.
Все ERR ответы завершаются LF. Только одна передача одновременно.
Временный `.btpart` удаляется при ошибке/отключении; rename происходит после
проверки финального CRC. Существующий файл не перезаписывается.
Timeout передачи без новых блоков/ACK — 10 секунд. См. реализацию
`src/services/FileBridge.cpp` и клиента `tools/mtble.py`.

Android-компаньон использует только 0x30, не подписывается на TX. Текст
обрезается на границе Unicode codepoint до min(199, negotiated-MTU - 4)
bytes; уведомления не разбиваются на несколько кадров.
