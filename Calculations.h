#ifndef PERAPRUDENT_CALCULATIONS_H
#define PERAPRUDENT_CALCULATIONS_H

#include "Records.h"

namespace peraprudent {

long long totalIncomeCentavos(const AppData& data);
long long totalExpensesCentavos(const AppData& data);
long long unpaidBillsCentavos(const AppData& data);
long long currentBalanceCentavos(const AppData& data);
long long spendableBalanceCentavos(const AppData& data);
long long dailySpendingLimitCentavos(const AppData& data, int daysUntilNextIncome);

}

#endif