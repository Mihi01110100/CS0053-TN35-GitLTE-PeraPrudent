// Test menu handlers in the same translation unit, with isolated input and files.
#define main floatApplicationMain
#include "../main.cpp"
#undef main
#include <chrono>
#include <functional>

namespace {
int checks = 0;
void require(bool condition, const char* label) {
    if (!condition) throw std::runtime_error(label);
    ++checks;
}
std::string interact(const std::string& input, const std::function<void()>& action) {
    std::istringstream in(input);
    std::ostringstream out;
    auto* oldIn = std::cin.rdbuf(in.rdbuf());
    auto* oldOut = std::cout.rdbuf(out.rdbuf());
    std::cin.clear();
    try { action(); } catch (...) {
        std::cin.rdbuf(oldIn); std::cout.rdbuf(oldOut); std::cin.clear(); throw;
    }
    std::cin.rdbuf(oldIn); std::cout.rdbuf(oldOut); std::cin.clear();
    return out.str();
}
}
int main() {
    namespace fs = std::filesystem;
    const fs::path folder = fs::temp_directory_path() /
        ("float-menu-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(folder)) return 1;
    const std::string path = (folder / "data.txt").string();
    try {
        AppData data;
        interact("Work\n2026-11-17\nabc\n-1\n1000.50\nTest income\n", [&] { addIncome(data, path); });
        require(data.receivedIncomes.size() == 1, "income added once");
        require(data.receivedIncomes[0].amountCentavos == 100050, "exact centavos");
        AppData reloaded; std::string error;
        require(floatapp::loadData(path, reloaded, error) == floatapp::LoadStatus::Loaded, "reload succeeds");
        require(reloaded.receivedIncomes[0].amountCentavos == 100050, "reload preserves income");
        interact("Client\n2026-11-23\n500\nExpected fee\n", [&] { addExpected(data, path); });
        require(data.expectedPayments.size() == 1, "expected created");
        require(floatapp::totalIncomeCentavos(data) == 100050, "expected excluded");
        interact("2\n2026-11-17\n450\n", [&] { receivePayment(data, path); });
        require(data.expectedPayments[0].received && data.receivedIncomes.size() == 2, "received atomically");
        require(data.receivedIncomes.back().amountCentavos == 45000, "actual amount recorded");
        interact("2\n", [&] { receivePayment(data, path); });
        require(data.receivedIncomes.size() == 2, "duplicate receipt blocked");
        interact("Internet\n2026-11-20\n200\n", [&] { addBill(data, path); });
        require(floatapp::unpaidBillsCentavos(data) == 20000, "unpaid bill reserved");
        const auto before = floatapp::spendableBalanceCentavos(data);
        interact("4\n2026-11-17\n", [&] { payBill(data, path); });
        require(data.bills[0].paid && data.expenses.size() == 1, "bill converts once");
        require(floatapp::spendableBalanceCentavos(data) == before, "no double deduction");
        interact("4\n", [&] { payBill(data, path); });
        require(data.expenses.size() == 1, "duplicate bill payment blocked");
        interact("5\n", [&] { editExpense(data, path, true); });
        require(data.expenses.size() == 1, "linked expense protected");
        interact("Food\n2026-11-17\n50\nLunch\n", [&] { addExpense(data, path); });
        require(data.expenses.size() == 2, "manual expense added");
        interact("6\nFood\n2026-11-17\n60\nUpdated lunch\n", [&] { editExpense(data, path, false); });
        require(data.expenses.back().amountCentavos == 6000, "manual expense updated");
        require(interact("Lunch\n", [&] { searchExpenses(data); }).find("No matches") != std::string::npos,
                "search is case-sensitive");
        interact("6\n1\n", [&] { editExpense(data, path, true); });
        require(data.expenses.size() == 1, "manual expense deleted");
        require(dayNumber("2028-03-01") - dayNumber("2028-02-28") == 2, "leap date difference");
        require(dayNumber("2026-12-01") - dayNumber("2026-11-30") == 1, "month date difference");
        AppData budget;
        budget.receivedIncomes.push_back({1,"Work","2026-11-17","",220000});
        budget.bills.push_back({2,"Bills","2026-11-20",70000,false});
        budget.reservedSavingsCentavos = 30000;
        require(floatapp::dailySpendingLimitCentavos(budget, 6) == 20000, "200 daily limit");
        require(floatapp::dailySpendingLimitCentavos(budget, 0) == 0, "zero days safe");
        budget.expectedPayments.push_back({3,"Work","2026-11-23","",50000,false});
        require(interact("2026-11-17\n", [&] { dailyPlan(budget); }).find("PHP 200.00") != std::string::npos,
                "date-based daily screen");
        budget.expectedPayments[0].expectedDate = "2026-11-17";
        require(interact("2026-11-17\n", [&] { dailyPlan(budget); }).find("No future pending payment") != std::string::npos,
                "today-only date does not divide");
        budget.receivedIncomes[0].amountCentavos = LLONG_MAX;
        budget.receivedIncomes.push_back({4,"Other","2026-11-17","",1});
        bool overflow = false;
        try { floatapp::totalIncomeCentavos(budget); } catch (const std::overflow_error&) { overflow = true; }
        require(overflow, "sum overflow rejected");
        require(money(LLONG_MIN) == "-PHP 92233720368547758.08", "minimum balance display safe");
        std::cout << checks << " menu/calculation checks passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << "\nFixtures: " << folder << '\n';
        return 1;
    }
    fs::remove(path); fs::remove(path + ".bak"); fs::remove(path + ".tmp"); fs::remove(folder);
    return 0;
}
