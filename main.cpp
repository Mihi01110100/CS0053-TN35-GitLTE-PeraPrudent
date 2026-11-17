#include "FileStorage.h"
#include "InputValidation.h"
#include "Records.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

namespace {
// Reuse legacy files when present, including recovery entries; new installations use FLOAT branding.
std::string dataPath() {
    namespace fs = std::filesystem;
    for (const std::string suffix : {"", ".bak", ".tmp"}) {
        std::error_code error;
        const auto state = fs::symlink_status("peraprudent-data.txt" + suffix, error);
        if (fs::exists(state) || (error && error != std::errc::no_such_file_or_directory)) return "peraprudent-data.txt";
    }
    return "float-data.txt";
}
const std::string DATA_PATH = dataPath();

void printMoney(long long centavos) {
    std::cout << "PHP " << centavos / 100 << '.'
              << std::setfill('0') << std::setw(2) << centavos % 100
              << std::setfill(' ');
}

void viewIncome(const floatapp::AppData& data) {
    if (data.receivedIncomes.empty()) {
        std::cout << "No received income recorded.\n";
        return;
    }
    for (const auto& income : data.receivedIncomes) {
        std::cout << "ID: " << income.id << " | " << income.date
                  << " | " << income.source << " | ";
        printMoney(income.amountCentavos);
        std::cout << " | " << income.description << '\n';
    }
}

void addIncome(floatapp::AppData& data) {
    floatapp::Income income;
    std::string error;
    if (!floatapp::nextRecordId(data, income.id, error)) {
        std::cerr << error << '\n';
        return;
    }
    std::cout << "Source: ";
    if (!floatapp::readRequiredText(std::cin, std::cout, income.source)) return;
    std::cout << "Date received (YYYY-MM-DD): ";
    if (!floatapp::readDate(std::cin, std::cout, income.date)) return;
    std::cout << "Amount received (pesos, e.g. 125.50): ";
    if (!floatapp::readMoney(std::cin, std::cout, income.amountCentavos)) return;
    std::cout << "Description: ";
    if (!floatapp::readRequiredText(std::cin, std::cout, income.description)) return;

    // Save a proposed copy first. A failed save leaves current records unchanged.
    floatapp::AppData proposed = data;
    proposed.receivedIncomes.push_back(income);
    if (!floatapp::saveData(DATA_PATH, proposed, error)) {
        std::cerr << "Income was not added. Save failed: " << error << '\n';
        return;
    }
    data = std::move(proposed);
    std::cout << "Income saved. ID: " << income.id << '\n';
}

void printMenu() {
    std::cout << "\n========== FLOAT.cpp ==========\n"
              << "Flexible Ledger for Obligations and Allowance Tracking\n"
              << "1. Record received income\n"
              << "2. View received income\n"
              << "0. Exit\n"
              << "Choice: ";
}
} // namespace

int main() {
    floatapp::AppData data;
    std::string error;
    const auto status = floatapp::loadData(DATA_PATH, data, error);
    if (status == floatapp::LoadStatus::Error) {
        std::cerr << "Cannot load records: " << error
                  << "\nResolve the file problem before continuing.\n";
        return 1;
    }
    std::cout << (status == floatapp::LoadStatus::NotFound
                      ? "First run: no saved records yet.\n"
                      : "Saved records loaded.\n");
    while (true) {
        printMenu();
        int choice = 0;
        if (!floatapp::readMenuChoice(std::cin, std::cout, 0, 2, choice)) {
            std::cout << "\nInput ended. Closing FLOAT.cpp.\n";
            return 0;
        }
        switch (choice) {
        case 0:
            std::cout << "Goodbye. Successful additions were saved immediately.\n";
            return 0;
        case 1:
            addIncome(data);
            break;
        case 2:
            viewIncome(data);
            break;
        }
    }
}
