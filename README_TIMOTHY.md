# Timothy: records, validation, and local file storage

C++17 storage/validation module for FLOAT.cpp. The module has no entry point; the group application entry point is main.cpp. Teammates own menus, balances, marking payments received/paid, and savings allocation. Include these headers instead of redefining the shared records. Do not overwrite the group's README or existing .gitignore.

Files: `Records.h` defines `floatapp::AppData` and its four record types; `InputValidation.h/.cpp` validates input and records; `FileStorage.h/.cpp` persists all records in one local file. Headers use explicit `std::`; implementation files use `using namespace std;`.

## Build and test

From this module directory, Windows PowerShell with a C++17 g++ installation:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -c InputValidation.cpp FileStorage.cpp
# Link the module with the group's actual application entry point:
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp InputValidation.cpp FileStorage.cpp -o FLOAT.exe
# Compile regression tests separately; never include tests in the application build:
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/module_tests.cpp InputValidation.cpp FileStorage.cpp -o module_tests.exe
.\module_tests.exe .
```

`main.cpp` above belongs to the group and is not supplied here. Tests create and remove a uniquely named subdirectory of the supplied argument; without an argument, they use the system temporary directory. Specify `.` if that directory is unavailable. Do not compile recursive `*.cpp` globs, which can include the test-only `main`.

The revised module was verified here on Linux with g++ using `-std=c++17 -Wall -Wextra -Wpedantic -Werror`; regression output: `65 checks passed`. The user previously reported `59 checks passed` on Windows for the earlier module. The revised 65-check version has not yet been run on Windows.

The new recovery regression cases ran as follows. Quoted `\n` denotes a newline in the exact fixture bytes; backup bytes come from the earlier valid save in the same test.

| Input / operation | Expected | Actual here (Linux) |
| --- | --- | --- |
| Main bytes `CORRUPT MAIN\n`, valid `.bak`, no `.tmp`; save valid AppData | `false`; main and backup byte-identical to before; no `.tmp`; nonempty error | `false`; both files byte-identical; `.tmp` absent; error nonempty |
| Main absent, same valid `.bak`; load into AppData with savings 777 | `Error`; savings stays 777; backup unchanged; main remains absent | `Error`; savings 777; backup byte-identical; main absent |
| Main and `.bak` absent, `.tmp` bytes `UNCOMMITTED DATA\n`; load into same AppData | `Error`; savings stays 777; temporary unchanged; main remains absent | `Error`; savings 777; temporary byte-identical; main absent |
| Same temporary-only state; save valid AppData | `false`; temporary unchanged; main remains absent | `false`; temporary byte-identical; main absent |
| Main, `.bak`, `.tmp` all absent; load into same AppData | `NotFound`; empty error; savings stays 777 | `NotFound`; error empty; savings 777 |
| Existing main path is the test directory; save valid AppData | `false`; nonempty error; no adjacent `.tmp` created | `false`; error nonempty; adjacent `.tmp` absent |

## Integration

Example statements to place inside the group's startup function (include `<iostream>` and the module headers):

```cpp
floatapp::AppData data;
std::string error;
const std::string path = "float-data.txt"; // Existing application handles legacy filenames.
auto status = floatapp::loadData(path, data, error);
if (status == floatapp::LoadStatus::Error) {
    std::cerr << error << '\n';
    return; // adapt to the enclosing function; do not overwrite a failed load
}
// NotFound is a normal first run: data is still the empty default AppData.
```

Example addition after a successful startup, inside a function with access to `data`, `path`, and `error`:

```cpp
floatapp::Income income;
if (!floatapp::nextRecordId(data, income.id, error)) {
    std::cerr << error << '\n';
    return;
}
std::cout << "Source: ";
if (!floatapp::readRequiredText(std::cin, std::cout, income.source)) return;
std::cout << "Date (YYYY-MM-DD): ";
if (!floatapp::readDate(std::cin, std::cout, income.date)) return;
std::cout << "Amount: ";
if (!floatapp::readMoney(std::cin, std::cout, income.amountCentavos)) return;
data.receivedIncomes.push_back(income);
if (!floatapp::saveData(path, data, error)) {
    std::cerr << error << '\n'; // in-memory addition remains; surface failure to user
}
```

All input helpers use `getline` only. Use `getline` in the surrounding menu too, avoiding a leftover newline from formatted `cin >>`. Print the initial prompt yourself. Helpers print another prompt on invalid input and return `false` at EOF or stream failure.

## Public API

All functions and types are in `floatapp`. String inputs are `const std::string&`; errors and string outputs are `std::string&`.

| Function | Result and behavior |
| --- | --- |
| `parseMoney(text, long long& out)` | `bool`; nonnegative decimal to exact integer centavos, up to two fractional digits; requires a digit before the point and after a supplied point. No sign, whitespace, exponent, suffix, or overflow. Output unchanged on failure. |
| `parseInteger(text, int& out, int minimum, int maximum)` | `bool`; decimal digits with optional leading minus, no plus/whitespace/junk; checks int and requested inclusive range. Output unchanged on failure. |
| `isValidDate(text)` | `bool`; exactly YYYY-MM-DD, Gregorian years 0001–9999 with leap-year rules. |
| `isValidText(text, std::size_t maxLength=200)` | `bool`; required nonblank text, byte length limit, no bytes 0–31 or 127. UTF-8 bytes are accepted without Unicode normalization or encoding validation. |
| `validateData(const AppData&, error)` | `bool`; checks all stored records and clears error on success. |
| `nextRecordId(const AppData&, int& out, error)` | `bool`; validates then returns maximum existing ID plus one; reports exhaustion, preserves output on failure. |
| `readMenuChoice(std::istream&, std::ostream&, int minimum, int maximum, int& value)` | `bool`; reprompts for a valid integer in range, false for invalid range or EOF. |
| `readMoney(std::istream&, std::ostream&, long long& value, bool allowZero=false)` | `bool`; positive money by default; optionally accepts zero. |
| `readDate(std::istream&, std::ostream&, std::string& value)` | `bool`; reprompts for a valid date. |
| `readRequiredText(std::istream&, std::ostream&, std::string& value)` | `bool`; reprompts for required text, max 200 bytes. |
| `loadData(path, AppData&, error)` | `LoadStatus::Loaded`, `NotFound`, or `Error`. Uses temporary AppData; caller unchanged unless fully loaded. Errors clear on Loaded/NotFound. |
| `saveData(path, const AppData&, error)` | `bool`; validates before writing; clears error on success. |

Amounts in records must be positive centavos. Reserved savings must be nonnegative; this module does not compare savings to a calculated balance. IDs must be positive and globally unique across all four vectors, up to `INT_MAX-1`. ID generation uses the maximum ID currently present plus one. Gaps below that maximum are not filled, but deleting the highest record can allow its ID to be reused later. IDs are unique among current records, not permanently unique historical identifiers. Reaching the ceiling is reported as exhaustion. Required labels are source/category/title. Dates are required. Descriptions may be empty, at most 200 bytes, with no control characters. Validation does not impose chronology or financial business rules.

## Teammate integration contracts

The teammate implementing “mark expected payment received” should first reject an already received payment. Create exactly one corresponding received-income record, using a fresh global ID, and mark the expected payment received in the same proposed AppData update. Likewise, “mark bill paid” should reject an already paid bill, create exactly one corresponding expense, and mark the bill paid in the same proposed update. Save each combined update once; report failure rather than indicating successful persistence. Keep or restore the previous in-memory state on a failed operation so a blind retry does not duplicate the transaction. Validate all needed input and allocate the fresh ID before mutating the working state.

These are contracts for the teammates' business logic. This module supplies no marking operations or record links, and cannot prove that a received flag has a corresponding income or a paid flag has a corresponding expense. If editing or deletion is supported, the group must define how those actions preserve these relationships and prevent repeat conversion. Maximum-plus-one IDs alone cannot track deleted historical transactions.

Use checked integer arithmetic when adding incomes, expenses, bill totals, savings, or computing balances. A valid individual amount may be as large as `LLONG_MAX` centavos; several valid amounts can overflow a `long long` total. Before adding nonnegative `amount` to nonnegative `total`, check `amount <= LLONG_MAX - total`; reject or report an out-of-range result otherwise. Check subtraction and any multiplication as well, particularly signed balances. Per-record validation does not guarantee safe aggregates; this module does not calculate totals or lower the parsing limit.

## Storage schema and recovery

Version 1 example (amounts below are integer centavos, not decimal pesos):

```text
FLOAT 1
INCOME 1 "Allowance" "2026-11-16" "Weekly allowance" 100000
EXPECTED 2 "Client" "2026-12-01" "" 50000 0
EXPENSE 3 "Food" "2026-11-16" "Lunch" 15000
BILL 4 "Internet" "2026-12-01" 120000 0
SAVINGS 25000
END 1 1 1 1
```

Strings use `std::quoted` with mandatory double quotes; quotes and backslashes are escaped and round-trip. Record lines may be reordered. Exactly one SAVINGS line is required. END contains the counts of INCOME, EXPECTED, EXPENSE, BILL in that order and must be last. Boolean fields accept only `0` or `1`. Every line must contain exactly the indicated fields. Unknown versions, unknown records, blank lines, bad values, duplicate IDs, repeated SAVINGS, trailing content, and truncated/mismatched counts fail without changing caller data. END counts detect lost record rows if the original footer remains; they are not a cryptographic integrity check.

Saving first reads and validates any existing main file. If that file is corrupt, unreadable, or not a regular file, saving refuses before opening `.tmp` or replacing `.bak`; all existing files are preserved. No earlier successful load or session flag is required. After that check, saving writes and closes `path.tmp`, renames an existing target to `path.bak`, then renames the temporary file to the target. Existing backups are replaced only when a target exists. This avoids reliance on renaming over an existing file on Windows. A failed final rename attempts to restore the original and reports rollback failures. A successful overwrite retains the previous file as `.bak`.

A preexisting `.tmp` is preserved and saving is refused. Inspect it and the target/backup before manually removing or moving it and retrying. If the target is missing and `.bak` exists, saving is refused to protect recovery data: inspect/load `.bak`, then restore it to the target path before retrying. Loading never automatically substitutes a backup. A missing main with either a `.bak` or `.tmp` entry returns Error with recovery instructions and leaves caller data and files unchanged. These checks use `symlink_status`, so dangling links also count as recovery entries. NotFound means the main and both recovery paths are absent. Inspection errors also return Error. Inspect and restore the appropriate recovery file before continuing; do not treat a recovery error as an empty first run.

Expected invalid input and filesystem failures return status/errors. This is single-user storage with no concurrent writers, locking, automatic recovery, fsync, encryption, or guarantee of complete crash/power-loss atomicity. Allocation failures are not handled. Keep ordinary backups for important data.

Append the patterns in `.gitignore_timothy.txt` to the group's existing `.gitignore` if desired. Do not replace its existing contents.

## Windows verification update - 2026-11-17
The revised module compiled with G++ on Windows. The regression run reported: 65 checks passed.

## FLOAT branding compatibility
The namespace is floatapp. New saved files use the FLOAT 1 header; the loader also accepts the legacy PERAPRUDENT 1 header. The application uses float-data.txt for new installations and retains peraprudent-data.txt if legacy data or recovery entries exist. Legacy names in this section describe compatibility, not current project branding.
