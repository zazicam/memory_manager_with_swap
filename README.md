**Задание. Написать менеджер памяти со свопом.**

*Краткая постановка задачи*

Надо создать менеджер памяти, позволяющий не только выделять блоки из 
из имеющейся оперативной памяти, но и автоматически перемещать неиспользуемые
в данный момент блоки на диск (в случае нехватки оперативной памяти) и
возвращать их обратно в RAM при необходимости доступа к ним.

Это может быть нужно, например, при обработке больших объемов
данных, которые не помещаются целиком в оперативную память.

Для меня это совершенно новая область, поэтому это скорее учебный,
чем прикладной проект. Однако я старался сделать его работоспособным
и полезным, насколько мог за эти пару недель.

*Сборка*

Для сборки программы я использовал Makefile. Поэтому просто выполните:
```
make
```
Имейте ввиду, что я использовал g++ с поддержкой C++17. В основном тестировал под linux,
но пару раз собирал под Windows с помощью Visual Studio 2020 (C++17) и проблем не было.

*Запуск программы*

После сборки в директории будет собратно 2 программы.

1. memory_manager_test - выполняет копирование файлов из директории 'input' в папку 'output'
используя менеджер памяти. В качестве аргумента командной строки она получает размер
доступной менеджеру памяти оперативной памяти в мегабайтах. 

Если мы хотим разрешить менеджеру управлять 10 Мб, то запустим с параметром 10:

```
./memory_manager_test 10
```

При этом это ограничение действует только на объем памяти напрямую выделяемый менеджером с помощью malloc.
Дополнительно в процессе работы менеджер использует порядка 40 байт на каждый выделенный блок (служебная информация о выделенных блоках: адрес, размер и т.п.). На эти данные ограничение не действует.

В данной реализации есть некоторые ограничения на размер свопа. В связи с чем программа может работать
только с объемами памяти, которые не более чем в 100 раз превышают объем предоставленной менеджеру 
оперативной памяти. Таким образом, если вы хотите работать с виртуальными 10Гб, то необходимо
разрешить менеджеру памяти использовать хотя бы 100 Мб реальной RAM. Это ограничение не сложно убрать, 
но я посчитал, что на практике и этого будет вполне достаточно.

2. check_result - простая утилита для проверки правильности копирования. Она побайтово (очень медленно)
сравнивает файлы во входной и выходной директории и выводит результат. 
```
./check_result
```
Я использовал ее для тестов при
копировании небольших файлов. При копировании файлов размером в несколько Гб она будет выполнять проверку
очень долго, лучше проверить как-то иначе.


*Формальная постановка задачи*

Вход: набор файлов большого размера (например, 1.5 Гб каждый) во входном
каталоге - каталог может быть задан фиксированно.

Выход: тот же набор файлов в выходном каталоге - каталог может быть задан фиксированно.

Каждый файл:
- обрабатывается в отдельном потоке (thread)
- сначала считывается блоками случайного размера от 1 до 4096 байт
- для каждого блока выделяется память в менеджере памяти со swap. Адрес
  выделенного блока сохраняется (в массив)
- после полного считывания файла в память происходит обратный процесс:
  считывание блоков памяти
  (с последующим освобождением) и запись на диск.

Менеджер памяти со swap должен реализовывать следующие функции:
- поддерживать две универсальные функции malloc/free в качестве интерфейса взаимодействия
- во внутреннй реализации содержать блоки разного размера 1..16,17..32 и
  т.д. до 2049..4096 байт (по степени 2) в разных пулах памяти (блоках,
  содержащих N блоков одинакового максимального размера в 16,32..4096 байт)
- при вызове malloc определять, в каком пуле памяти выделять блок
- быть многопоточным
- при заполнении пула при очередном malloc производить сброс самого старого
  выделенного блока из данного пула на диск в частную реализацию swap-файл
  (набор файлов) на диске
- для актуализации блока памяти (для считывания) реализовать функцию,
  которая, при необходимости считывает требуемый блок со swap и размещает в
  оперативной памяти; альтенатив - можно реализовать функции,
  разрешающие/запрещающие размещать данный блок в swap (при запрещении при
  необходимости возвращать блок из диска в память)

При работе процесс обязательно должен выполнять фунции сброса данных в swap.
Чтобы исключить зависимость от размера входных данных, можно предусмотреть в
качестве параметра командной строки задание оперативного размера менеджера
памяти (размер используемой оперативной памяти).

Дополнительно можно реализовать вывод в консоль (1 раз в секунду) состояния менеджера памяти:
количество занятых/всего блоков/размер, аналогичное количество в swap.

Требований по использованию новых стандартов C/C++ нет.

*Проблемы данной реализации*

1. Для хранения информации о выгруженных на диск блоках я использовал таблицу следующего вида

```
swapTable
+--------+-----------------------+
|        |    Blocks indexes     |
| Swap   +---+---+---+-----+-----+
| Level  | 0 | 1 | 2 | ... | N-1 |
+--------+---+---+---+-----+-----+
| 0      | 1 | 1 | 3 |  .  | 2   |
+--------+---+---+---+-----+-----+
| 1      | 2 | 0 | 0 |  .  | 1   |
+--------+---+---+---+-----+-----+
| 2      | 3 | 0 | 0 |  .  | 0   |
+--------+---+---+---+-----+-----+
| 3      | 0 | 0 | 2 |     | 0   |
+--------+---+---+---+-----+-----+
```

Эта таблица ставит в соответсвие каждому уровню свопа (строка) и индексу блока в RAM целое число - идентификатор
выгруженного блока (swapId). Уровень 0 соответсвует RAM, остальные уровни - файлам. Эта реализация имеет ряд недостатков:
- Наверно логичнее было бы хранить в таблице уровни свопа, которые соответствуют идентификаторам блоков. В таком
случае при поиске блока по его swapId не нужно было бы пробегаться по столбцу таблицы. Так будет эффективнее, но
я думаю это не сильно повляет на производительность так как поиск в массиве из ~100 элементов выполняется гораздо
быстрее чем считывание блоков из файлов (а нужен он только вместе со считыванием).

- Каждый уровень свопа записывается в отдельный файл. Но во всех ОС есть ограничение на количество открытых 
программой файлов (обычно 1024), поэтому такая реализация не позволяет использовать более 100 уровней свопа.
Таким образом размер виртуальной памяти (со свопом) не превышает 100 размеров реально используемой оперативной
памяти.

2. Задача копирования файлов не требует одновременной работы в одном потоке сразу с несколькими блоками, 
однако в других задачах такая необхомость скорее всего будет, например, хотелось бы использовать memncpy()
для копирования данных из одного блока в другой. В данной реализации если 2 блока окажутся в таблице в одном
столбце (память под них будет выделена в одном реальном блоке RAM), то только один из них сможет находиться
на нулевом уровне (в оперативной памяти). 
Этот недостаток можно исправить переносом блока в другой блок оперативной памяти, но я пока этого не сделал.
