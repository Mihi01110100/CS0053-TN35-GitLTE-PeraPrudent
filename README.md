# FLOAT.cpp
**Flexible Ledger for Obligations and Allowance Tracking**  
*A C++ Console System for Irregular Cashflow & Daily Runway Management*

C++17 console project for working students receiving income on irregular dates.

## Implemented on feature/menu-income

- Record/view received income; add/view/reschedule expected payments.
- Receive an expected payment once using its actual amount and date.
- Add/view/search/update/delete manual expenses; search is case-sensitive.
- Add/view/pay bills once; linked bill expenses are protected from manual edits/deletion.
- Reserve savings; show financial and category summaries.
- Estimate daily spending funds from a user-entered planning date to the earliest future pending payment.
- Save successful mutations and load records at startup; validation and recovery handling.

Expected income is excluded until received. All unpaid bills are reserved, including bills beyond the next payment date. The daily limit is floored to centavos. An overdue pending payment is flagged; if no future pending date exists, update the schedule before calculating. Savings is a reservation, not a bank transfer. This is manual, single-user local storage.

## Requirements and Windows build

Use G++ with C++17 filesystem support (MSYS2 UCRT64 G++ works). Run commands from the repository folder. If G++ is not on PATH in PowerShell:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;" + $env:Path
```

Close any running FLOAT.exe before rebuilding.

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp Calculations.cpp InputValidation.cpp FileStorage.cpp -o FLOAT.exe
.\FLOAT.exe
```

Do not compile tests into the application or recursively compile every CPP file.

## Example

With received income PHP 2200, savings PHP 300, unpaid bills PHP 700, planning date 2026-11-17 and a pending payment dated 2026-11-23:

```text
Available spending funds: PHP 1200.00
Days to cover: 6
Estimated daily limit: PHP 200.00
```

This is an expected example, not a claim of a completed runtime test.

## Storage and compatibility

Code uses namespace floatapp. New data uses float-data.txt and the FLOAT 1 format header. If legacy peraprudent-data.txt or its recovery entries exist, the application retains that path to avoid silently abandoning existing records. The loader accepts PERAPRUDENT 1 as well as FLOAT 1; writes use FLOAT 1. Keep real financial data and executables out of Git. Do not manually delete recovery files without inspecting the storage error and recovery instructions in README_TIMOTHY.md.

## Tests

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/module_tests.cpp InputValidation.cpp FileStorage.cpp -o module_tests.exe
.\module_tests.exe .
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/menu_integration_tests.cpp Calculations.cpp InputValidation.cpp FileStorage.cpp -o menu_tests.exe
.\menu_tests.exe
```

Menu integration tests use isolated temporary data and simulated input. They cover income validation/reload, expected-payment receipt once, bill conversion once, protected linked expenses, expense operations, dates, financial calculations, and overflow handling. A failure retains the temporary fixtures and prints their path.

**Verification on November 17, 2026:** The integrated application and menu tests compiled using G++ 16.2.0 with -Wall -Wextra -Wpedantic -Werror. The assistant's local attempt to execute the menu tests was blocked by Windows Application Control. Runtime results remain pending; record the actual output from an authorized environment before reporting tests passed. Earlier storage-module results belong to their original test version and must not be represented as this integration test run.

## Collaboration and remaining work

This branch integrates Louie's calculation API with the console menu, adapts it to floatapp, and adds checked arithmetic for totals/balances. Teammates must review the integration, run tests, and provide actual evidence before merge. Expected-income/bill conversions save a proposed copy before changing active data. There is no permanent historical transaction-link model; do not add record-edit/delete operations that undermine conversion tracking without designing that relationship.

AI assisted the integration and test code. Group members must review, understand, test, and explain all accepted code, and record their decisions in the activity AI log.
