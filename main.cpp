#include "FileStorage.h"
#include "InputValidation.h"
#include "Records.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

namespace {
// Keep the existing filename so previously saved records remain accessible.
const std::string DATA_PATH = "peraprudent-data.txt";

void printMoney(long long centavos) {
    std::cout << "PHP " << centavos / 100 << '.'
              << std::setfill('0') << std::setw(2) << centavos % 100
              << std::setfill(' ');
}

void viewIncome(const peraprudent::AppData& data) {
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

void addIncome(peraprudent::AppData& data) {
    peraprudent::Income income;
    std::string error;
    if (!peraprudent::nextRecordId(data, income.id, error)) {
        std::cerr << error << '\n';
        return;
    }
    std::cout << "Source: ";
    if (!peraprudent::readRequiredText(std::cin, std::cout, income.source)) return;
    std::cout << "Date received (YYYY-MM-DD): ";
    if (!peraprudent::readDate(std::cin, std::cout, income.date)) return;
    std::cout << "Amount received (pesos, e.g. 125.50): ";
    if (!peraprudent::readMoney(std::cin, std::cout, income.amountCentavos)) return;
    std::cout << "Description: ";
    if (!peraprudent::readRequiredText(std::cin, std::cout, income.description)) return;

    // Save a proposed copy first. A failed save leaves current records unchanged.
    peraprudent::AppData proposed = data;
    proposed.receivedIncomes.push_back(income);
    if (!peraprudent::saveData(DATA_PATH, proposed, error)) {
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
    peraprudent::AppData data;
    std::string error;
    const auto status = peraprudent::loadData(DATA_PATH, data, error);
    if (status == peraprudent::LoadStatus::Error) {
        std::cerr << "Cannot load records: " << error
                  << "\nResolve the file problem before continuing.\n";
        return 1;
    }
    std::cout << (status == peraprudent::LoadStatus::NotFound
                      ? "First run: no saved records yet.\n"
                      : "Saved records loaded.\n");
    while (true) {
        printMenu();
        int choice = 0;
        if (!peraprudent::readMenuChoice(std::cin, std::cout, 0, 2, choice)) {
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
