| Specifier   | Meaning                        | Example                                       |
| ----------- | ------------------------------ | --------------------------------------------- |
| `%d` / `%i` | signed int (decimal)           | `printf("%d", -42); → -42`                    |
| `%u`        | unsigned int                   | `printf("%u", 42); → 42`                      |
| `%f`        | floating point                 | `printf("%f", 3.14); → 3.140000`              |
| `%.2f`      | floating point with 2 decimals | `3.14`                                        |
| `%c`        | single character               | `printf("%c", 'A'); → A`                      |
| `%s`        | string (char*)                 | `printf("%s", "ESP32"); → ESP32`              |
| `%p`        | pointer (address)              | `printf("%p", ptr); → 0x3ffb1234`             |
| `%x`        | hex lowercase                  | `printf("%x", 255); → ff`                     |
| `%X`        | hex uppercase                  | `FF`                                          |
| `%o`        | octal                          | `377`                                         |
| `%%`        | literal `%`                    | `printf("Progress: 50%%\n"); → Progress: 50%` |


**Alignment, padding, and width:**

printf("[%5d]\n", 42);   // right align in 5 spaces

printf("[%-5d]\n", 42);  // left align in 5 spaces

printf("[%05d]\n", 42);  // pad with zeros

[   42]

[42   ]

[00042]


**For floats:**

printf("%.2f\n", 3.14159);   // 2 decimals → 3.14

printf("%8.3f\n", 3.14159);  // width=8, precision=3 → '   3.142'
