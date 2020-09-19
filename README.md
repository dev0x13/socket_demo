# Тестовое задание на сокеты

Пакет содержит решение тестового задания на C++, сборка производится с помощью CMake.

## Постановка задачи

Необходимо разработать два приложения под Linux: клиент и сервер.

Клиент посылает введенное пользователем на консоль сообщение, сервер при получении данного сообщения обрабатывает его: находит в данном сообщении все числа, вычисляет их сумму и отправляет в ответ результат. Если в сообщении от клиента нет ни одного числа, то сервер должен просто отправить в ответ клиенту полученное сообщение. Можно считать, что для хранения каждого из чисел в сообщении хватает размерности встроенных типов.

Допустим, клиент отправил сообщение:
```
20 apples, 30 bananas, 15 peaches and 1 watermelon
```
 в ответ он должен получить сообщение: 
```
1 15 20 30
66
```

**Обязательные требования:**

* При реализации используйте С++/C (использование STL приветствуется, сторонних библиотек - нет).
* Работа с сетью должна выполняться средствами Berkeley sockets API (использование сторонних фреймворков для работы с сетью не допускается).
* Клиент и сервер должны поддерживать следующие протоколы: TCP, UDP.
* Сервер должен корректно обслуживать несколько одновременно подключенных клиентов по любому из перечисленных выше протоколов.
* Клиент может выбирать (при старте приложения), какой протокол будет использован для взаимодействия с сервером, при этом должна быть возможность отправки нескольких сообщений без перезапуска или переустановки соединения.

**На что стоит обратить внимание:**

* Приложения должны легко собираться, приложениями должно быть легко и удобно пользоваться, они должны быть рассчитаны на длительную и стабильную работу, а также иметь возможность корректно завершаться.
* Специфика работы с сетью в том, что многое может пойти не так (ошибки, некорректные данные, некорректное поведение удаленной стороны), приложения должны быть готовы к этому, учитывайте разницу между протоколами TCP и UDP.
* По возможности старайтесь сделать реализацию эффективной и надежной (возможно удастся обойтись без использования потоков (threads) или сократить накладные расходы при их использовании).

## Сборка и запуск проекта

### Системные требования

 * ОС Linux
 * CMake 3.8+
 * Компилятор C++ с поддержкой C++11
 
 Код тестировался на Ubuntu 20.04 + GCC 9.3 + CMake 3.13.
 
### Сборка
 
Сборка производится следующими командами:
```bash
mkdir _build
cd _build
cmake ..
cmake --build . --target install
``` 

В результате будет создана папка `_stage` с тремя исполняемыми файлами:
* `client` - сам сервер; проткол, порт и остальные параметры задаются аргументами командной строки
* `server` - клиент; параметры также задаются аргументами командной строки
* `smoke_test` - простой тест, который проверяет работоспособность клиента и сервера, в том числе под нагрузкой

Каждый из исполняемых файлов снабжен минимальной документацией по его параметрам. Для ее просмотра необходимо
запустить файл без аргументов.

### Примеры запуска

#### TCP (для UDP вывод аналогичный)

Терминал 1:
```bash
george@george:~/socket_demo/_stage$ ./server 8888 TCP 
```

Терминал 2:
```
george@george:~/socket_demo/_stage$ ./client 127.0.0.1 8888 TCP
Enter your message: hello
hello
Enter your message: 1 15 60
1 15 60
76
```

#### Smoke-тест

Терминал 1:
```bash
george@george:~/socket_demo/_stage$ ./server 8888 TCP 
```

Терминал 2:
```
george@george:~/socket_demo/_stage$ ./smoke_test 127.0.0.1 8888 TCP 5 1024
OK!
```

## Комментарии к реализации

* Реализация сервера однопоточная, smoke-тест проверяет корректность работы сервера при одновременном
обслуживании нескольких клиентов (корректность ответа и гарантия его получения).
* В решении сделано ограничение на длину сообщения - 65507 байт (максимальная длина данных в 
одном датаграме UDP). Ограничение введено для того, чтобы не реализовавать ARQ проткол поверх UDP.
Для TCP это ограничение носит искусственный характер и введено для единообразия.
* Сервер не может работать одновременно по двум протоколам, для этого нужно запускать отдельные экземпляры `server`
с соответствующими аргументами. Принципиальных ограничений для этого нет, просто изначально я не обратил внимания на то,
что задание можно трактовать и так.
* Smoke-тест для UDP рекомендуется запускать с маленьким `operations_timeout_s` (1) и большим `operations_timeout_s`
(пропорционально `operations_timeout_s`), так как иначе он не проходит по понятным причинам.
