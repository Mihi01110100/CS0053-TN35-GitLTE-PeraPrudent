#include "Calculations.h"

namespace peraprudent {

long long totalIncomeCentavos(const AppData& data) {
    long long total = 0;

    for (const Income& income : data.receivedIncomes) {
        total += income.amountCentavos;
    }

    return total;
}

long long totalExpensesCentavos(const AppData& data) {
    long long total = 0;

    for (const Expense& expense : data.expenses) {
        total += expense.amountCentavos;
    }

    return total;
}

long long unpaidBillsCentavos(const AppData& data) {
    long long total = 0;

    for (const Bill& bill : data.bills) {
        if (!bill.paid) {
            total += bill.amountCentavos;
        }
    }

    return total;
}

long long currentBalanceCentavos(const AppData& data) {
    return totalIncomeCentavos(data) - totalExpensesCentavos(data);
}

long long spendableBalanceCentavos(const AppData& data) {
    return currentBalanceCentavos(data)
        - unpaidBillsCentavos(data)
        - data.reservedSavingsCentavos;
}

long long dailySpendingLimitCentavos(const AppData& data, int daysUntilNextIncome) {
    if (daysUntilNextIncome <= 0) {
        return 0;
    }

    const long long spendable = spendableBalanceCentavos(data);

    if (spendable <= 0) {
        return 0;
    }

    return spendable / daysUntilNextIncome;
}

}