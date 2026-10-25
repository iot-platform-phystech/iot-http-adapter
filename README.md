# iot-http-adapter

HTTP-адаптер IoT-платформы на C++20/Drogon. `iot::Decoder` декодирует данные
сенсора перед отправкой в core, `iot::Encoder` кодирует команды перед отправкой
сенсору. Поддерживаются `base64` и `aes-256-gcm` через OpenSSL EVP.
Base64 — кодирование, оно не скрывает данные.

## Сборка

Нужны CMake >= 3.24, C++20, Ninja, установленные Drogon и OpenSSL (включая
заголовки). Preset ищет библиотеки в `$HOME/.local`; другой путь можно указать
через `-DCMAKE_PREFIX_PATH`.

```bash
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
python3 tests/integration.py ./build/debug/iot-http-adapter
```

HTTP-тест запускает локальные заглушки core/сенсора и требует свободный порт 8080.
Dockerfile также устанавливает зависимости и собирает сервис.

## Настройки и запуск

| Переменная | Значение по умолчанию | Назначение |
| --- | --- | --- |
| `IOT_ADAPTER_HOST` | `127.0.0.1` | Адрес прослушивания, порт 8080; в Docker `0.0.0.0` |
| `IOT_CORE_URL` | `http://127.0.0.1:8000` | Адрес core; в Docker `http://core:8000` |
| `IOT_SENSOR_URL` | не задан | Адрес одного сенсора для команд |
| `IOT_SENSOR_PATH` | `/api/v1/commands` | Путь приёма команд сенсором |
| `IOT_SENSOR_ALGORITHM` | `aes-256-gcm` | Алгоритм в обоих направлениях |
| `IOT_AES_KEY_BASE64` | не задан | Общий ключ: ровно 32 байта, представленные в Base64 |

Для AES задайте одинаковый ключ на адаптере и сенсоре. Один раз сгенерировать
ключ можно командой `openssl rand -base64 32`; сохраните его вне репозитория.
Без ключа адаптер в режиме AES завершает запуск с ошибкой.

```bash
export IOT_AES_KEY_BASE64='<общий ключ в Base64>'
export IOT_SENSOR_URL='http://127.0.0.1:9000'
./build/debug/iot-http-adapter
```

Для запуска без шифрования явно задайте `IOT_SENSOR_ALGORITHM=base64`.
`GET /ping` возвращает `pong`.

## Сенсор → адаптер → core

Сенсор отправляет `POST /api/v1/ingest` с `Content-Type: application/json`:

```json
{"algorithm":"aes-256-gcm","payload":"<Base64(nonce || ciphertext || tag)>"}
```

После расшифровки ожидается JSON-объект:

```json
{"sensor_id":"sensor-1","value":23.5,"unit":"C"}
```

`sensor_id` — непустая строка, `value` — число, `unit` — необязательная строка.
Адаптер пересылает этот JSON в `POST {IOT_CORE_URL}/api/v1/ingest` и возвращает
статус и тело ответа core. Прежний открытый JSON на входе адаптера необходимо
заменить конвертом `algorithm`/`payload`; формат запроса к core не меняется.

Пример для режима Base64:

```bash
payload=$(printf '%s' '{"sensor_id":"sensor-1","value":23.5,"unit":"C"}' | openssl base64 -A)
curl http://127.0.0.1:8080/api/v1/ingest \
  -H 'Content-Type: application/json' \
  -d "{\"algorithm\":\"base64\",\"payload\":\"$payload\"}"
```

## Core → адаптер → сенсор

Core или другой внутренний клиент отправляет открытый JSON-объект команды:

```bash
curl http://127.0.0.1:8080/api/v1/commands \
  -H 'Content-Type: application/json' \
  -d '{"sensor_id":"sensor-1","command":"set_interval","seconds":10}'
```

Адаптер сериализует объект, кодирует выбранным алгоритмом и отправляет конверт
`algorithm`/`payload` в `POST {IOT_SENSOR_URL}{IOT_SENSOR_PATH}`. Возвращает статус
и тело ответа сенсора. Адрес сенсора/шлюза фиксируется настройкой адаптера. Core инициирует команды
через свой маршрут `/api/v1/devices/{id}/commands`, проверяя устройство и
подставляя `sensor_id`; этот маршрут также доступен через адаптер.
Ответы upstream передаются как есть, кодирование применяется к телам запросов.

## Формат и ошибки

- Base64: стандартный алфавит с `=`-padding, без пробелов и переносов строк.
- AES-256-GCM: ключ 32 байта, случайный nonce 12 байт для каждого сообщения,
  authentication tag 16 байт, AAD отсутствует. `payload` — стандартный Base64
  от объединения `nonce || ciphertext || tag`. Сенсор должен использовать тот же формат.
- Алгоритм конверта обязан совпадать с `IOT_SENSOR_ALGORITHM`; автоматического
  перехода на Base64 нет. Повреждённый AES tag отклоняется до пересылки в core.
- Максимальный размер исходных данных — 1 MiB, HTTP-тела — 2 MiB.
- Некорректные данные: `400`; сенсор не настроен: `503`; ошибка соединения или
  таймаут upstream (5 секунд): `502`.

Шифрование payload не добавляет авторизацию HTTP-клиентов или защиту от
повторной отправки сообщений. Внешний доступ к API и транспорт до core требуют
отдельной настройки доступа/TLS.


## Слои и CRUD

`src/controllers/AdapterController` принимает HTTP и формирует ответы;
`src/services/AdapterService` декодирует измерения, кодирует команды и выбирает
направление передачи; `src/repositories/HttpRepository` выполняет HTTP-запросы к
core и сенсору. `main.cpp` только собирает зависимости и запускает сервер.
Классы `Encoder` и `Decoder` находятся в `src/codec`.

CRUD пользователей и устройств выполняется в core, доступен через адаптер:

| Метод | Путь |
| --- | --- |
| GET / POST | `/api/v1/users`, `/api/v1/devices` |
| GET / PUT / DELETE | `/api/v1/users/{id}`, `/api/v1/devices/{id}` |
| GET | `/api/v1/data?limit=100` |
| POST | `/api/v1/devices/{id}/commands` |

Адаптер сохраняет HTTP-статус и тело ответа core. Для CRUD используются обычные
JSON-объекты, без конверта кодирования. Шифрование применяется к измерениям от
сенсора и командам к сенсору. Перед отправкой измерений создайте пользователя и
включённое устройство в core. Уведомления Telegram отправляет core после записи
измерения владельцу устройства.

Совместный запуск, миграция БД, поля CRUD и настройка Telegram описаны в
`../iot-core/README.md`; Compose находится в `../iot-core/compose.yaml`.
