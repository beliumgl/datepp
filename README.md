# Date and Time Manager for Unix Timestamps

*A single-header C++11 library for parsing, formatting, and managing Unix timestamps-including negative values (dates before 1970).*

---

## Features

- **Handles both positive and negative Unix timestamps** (dates before and after 1970)
- **Customizable date/time formatting**  
  Choose order (MDY, DMY, YMD), delimiter, show/hide day of week, time, UTC offset, use 12/24-hour time, month as number or name, and zero-padding
- **Timezone offset support**
- **Operator overloading** for date comparison and arithmetic
- **Zero dependencies** beyond the C++11 standard library

---

## Quick Start

### 1. Include the Header

```cpp
#include "datepp.hpp"
```

### 2. Create and Use a DateTime

```cpp
#include 
#include "datepp.hpp"

int main() {
    beliumgl::DateTime d("1700000000"); // Unix timestamp as string
    std::cout << d.toString() << std::endl; // Default format
}
```

### 3. Custom Formatting

```cpp
beliumgl::DateTimeFormat fmt("W, DD.MM.YYYY HH:II:SS O UTC");
std::cout << dt.toString(fmt) << std::endl;
```

---

## How Does the String Format Work?

- **Case-insensitive:** You can use upper or lower case letters; it doesn't matter.
- **Unknown characters are ignored** (except when used as delimiters).
- **Delimiter:** The delimiter will be the last character separating D, M (or A), and Y in your format string.

### Tokens

The format string uses the following tokens:

| Token | Meaning              | Note                                     |
|-------|----------------------|------------------------------------------|
| W     | Day of the week      | Use `WW` for full names                  |
| D     | Day                  | Use `DD` for zero-padded day             |
| M     | Month (number)       | Use `MM` for zero-padded month           |
| Y     | Year                 | Use `YY` for zero-padded year            |
| H     | Hour                 |                                          |
| I     | Minute               | `'I'` is used because `'M'` is for month |
| S     | Second               |                                          |
| O     | UTC offset           |                                          |
| _     | 12-hour format       |                                          |
| A     | Alphabetical month   |             |

### Special Patterns

- `WW` - Show the full name of the day of the week (e.g., "Thursday").
- `DD`, `MM`, `YY` - Fill with zeros (e.g., "01", "09", "05").
- Single token (e.g., `D`, `M`, `Y`) - Short names or no leading zeros.

- NOTE: If there are at least one DD or MM, then date and time will also be filled with zeros
### Format Order

- **The order of D, M (or A), and Y is important!**
    - For example, `MM/DD/YY` and `DD/MM/YY` will be interpreted differently.

### Optional Tokens

- `H`, `I`, `S`, `W`, `O`, `_`, and `A` are **not required** in the format.
- Omitting them will result in the loss of some information (e.g., time, UTC offset, time format, etc.).

---

**Example format:**  
`"W, DD/MM/YY, HH:II:SS O"`  
- This would display something like: `Thu, 01/01/1970, 00:00:00 +00 UTC`
---

## API Overview

### `DateTimeFormat`
- Customize how dates are displayed
- Example:
  ```cpp
  beliumgl::DateTimeFormat fmt("DD.MM.YYYY");
  ```

### `DateTime`
- Construct from Unix timestamp (string or char*)
- Convert to string in your chosen format
- Get individual components: year, month, day, hour, minute, second, day of week, UTC offset
- Operator overloads: compare, add, subtract, multiply, divide dates

---

## Example Usage

```cpp
beliumgl::DateTime d("1700000000", 2.0); // timestamp, UTC+2
std::cout << d.toString("W, DD/MM/YY HH:II:SS O UTC") << std::endl;
```

---

## Notes

- **Month and day are zero-based internally** (January = 0, first day = 0), but formatted output is 1-based.

---
