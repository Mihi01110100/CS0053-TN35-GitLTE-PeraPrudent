#include "FileStorage.h"
#include "InputValidation.h"
#include <chrono>
#include <climits>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
using namespace std;
using namespace peraprudent;
namespace {
int checks=0;
void check(bool condition, const string& name) {
    ++checks;
    if (!condition) throw runtime_error(name);
}
string readFile(const filesystem::path& path) {
    ifstream in(path,ios::binary);
    if (!in) throw runtime_error("test fixture read failed");
    return string(istreambuf_iterator<char>(in),istreambuf_iterator<char>());
}
void writeFile(const filesystem::path& path, const string& contents) {
    ofstream out(path, ios::binary); out<<contents;
    if (!out) throw runtime_error("test fixture write failed");
}
}
int main(int argc, char** argv) {
    filesystem::path directory;
    try {
        const auto base=argc>1 ? filesystem::path(argv[1]) : filesystem::temp_directory_path();
        directory=base/("peraprudent-tests-"+to_string(chrono::steady_clock::now().time_since_epoch().count()));
        filesystem::create_directories(directory);
        long long money=17;
        check(parseMoney("12.3",money) && money==1230,"one decimal");
        check(parseMoney("0",money) && money==0,"zero money");
        check(parseMoney("92233720368547758.07",money) && money==LLONG_MAX,"maximum money");
        for (const string bad:{"", "-1", "+1", " 1", "1 ", ".5", "1.", "1.234", "1e2", "1.2x", "92233720368547758.08", "999999999999999999999"}) {
            money=17; check(!parseMoney(bad,money) && money==17,"invalid money: "+bad);
        }
        int number=8;
        check(parseInteger("-2147483648",number,INT_MIN,INT_MAX) && number==INT_MIN,"minimum integer");
        for (const string bad:{"1x", "+1", " 1", "2147483648", "-2147483649"}) check(!parseInteger(bad,number,INT_MIN,INT_MAX),"invalid integer");
        check(isValidDate("2000-02-29") && isValidDate("0001-01-01") && isValidDate("9999-12-31"),"valid dates");
        for (const string bad:{"1900-02-29","2025-04-31","0000-01-01","2025-1-01","2025-13-01"}) check(!isValidDate(bad),"invalid date");
        check(isValidText("Café") && !isValidText("   ") && !isValidText("a\nb") && !isValidText(string(201,'x')),"required text");
        istringstream menu("x\n3\n"); ostringstream prompts;
        check(readMenuChoice(menu,prompts,1,4,number) && number==3,"menu retry");
        istringstream amountInput("0\n2.50\n");
        check(readMoney(amountInput,prompts,money) && money==250,"money retry");
        istringstream dateInput("2025-02-29\n2024-02-29\n"); string value;
        check(readDate(dateInput,prompts,value) && value=="2024-02-29","date retry");
        istringstream textInput(" \nSource\n");
        check(readRequiredText(textInput,prompts,value) && value=="Source","text retry");
        istringstream empty; value="unchanged"; money=19; number=7;
        check(!readMenuChoice(empty,prompts,1,4,number) && number==7,"menu EOF");
        check(!readMoney(empty,prompts,money) && money==19,"money EOF");
        check(!readDate(empty,prompts,value) && value=="unchanged","date EOF");
        check(!readRequiredText(empty,prompts,value) && value=="unchanged","text EOF");
        AppData data; string error;
        data.receivedIncomes.push_back({1,"A \"quoted\" \\ source","2024-02-29","description with spaces",12345});
        data.expectedPayments.push_back({2,"Allowance","2026-12-01","",5000,true});
        data.expenses.push_back({3,"Food","2026-11-16","Lunch",15000});
        data.bills.push_back({4,"Rent","2026-12-01",300000,false});
        data.reservedSavingsCentavos=2000;
        check(validateData(data,error) && error.empty(),"valid complete data");
        check(nextRecordId(data,number,error) && number==5,"next ID");
        AppData invalid=data; invalid.bills[0].id=1;
        check(!validateData(invalid,error),"global duplicate ID");
        invalid=data; invalid.bills[0].id=INT_MAX-1;
        check(!nextRecordId(invalid,number,error),"ID exhaustion");
        const auto path=directory/"data.txt";
        AppData loaded; loaded.reservedSavingsCentavos=99;
        check(loadData(path.string(),loaded,error)==LoadStatus::NotFound && loaded.reservedSavingsCentavos==99 && error.empty(),"first run");
        check(saveData(path.string(),data,error) && error.empty(),"initial save");
        check(loadData(path.string(),loaded,error)==LoadStatus::Loaded && loaded.receivedIncomes[0].source==data.receivedIncomes[0].source && loaded.expectedPayments[0].received && !loaded.bills[0].paid && loaded.reservedSavingsCentavos==2000,"round trip");
        data.reservedSavingsCentavos=3000;
        check(saveData(path.string(),data,error) && filesystem::exists(path.string()+".bak"),"overwrite backup");
        AppData backup;
        check(loadData(path.string()+".bak",backup,error)==LoadStatus::Loaded && backup.reservedSavingsCentavos==2000,"backup contains previous data");
        invalid.bills[0].id=1;
        check(!saveData(path.string(),invalid,error),"invalid save rejected");
        writeFile(path.string()+".tmp","preserve me");
        check(!saveData(path.string(),data,error),"preexisting temporary protected");
        filesystem::remove(path.string()+".tmp");
        const string header="PERAPRUDENT 1\n";
        const string income="INCOME 1 \"Source\" \"2026-11-16\" \"\" 100\n";
        const string ending="SAVINGS 0\nEND 1 0 0 0\n";
        const string malformed[]={"", header+income, header+"SAVINGS 0\nEND 1 0 0 0\n", header+income+"SAVINGS 0\nSAVINGS 0\nEND 1 0 0 0\n", header+income+ending+"extra\n", header+"WHAT 2\n"+ending, header+"EXPECTED 1 \"S\" \"2026-11-16\" \"\" 1 2\nSAVINGS 0\nEND 0 1 0 0\n", header+"INCOME 1 Source \"2026-11-16\" \"\" 100\n"+ending, header+income+"SAVINGS 0 extra\nEND 1 0 0 0\n", header+income+"INCOME 1 \"S\" \"2026-11-16\" \"\" 1\nSAVINGS 0\nEND 2 0 0 0\n"};
        for (const auto& fixture:malformed) {
            writeFile(path,fixture); loaded.reservedSavingsCentavos=777;
            check(loadData(path.string(),loaded,error)==LoadStatus::Error && loaded.reservedSavingsCentavos==777 && !error.empty(),"malformed file preserves caller");
        }
        const string corruptMain="CORRUPT MAIN\n";
        writeFile(path,corruptMain);
        const string goodBackup=readFile(path.string()+".bak");
        check(!saveData(path.string(),data,error) && !error.empty() &&
              readFile(path)==corruptMain && readFile(path.string()+".bak")==goodBackup &&
              !filesystem::exists(path.string()+".tmp"),"corrupt main refuses save and preserves all files");
        filesystem::remove(path);
        loaded.reservedSavingsCentavos=777;
        check(loadData(path.string(),loaded,error)==LoadStatus::Error &&
              loaded.reservedSavingsCentavos==777 && !error.empty() &&
              readFile(path.string()+".bak")==goodBackup && !filesystem::exists(path),
              "missing main plus backup is recovery error");
        check(!saveData(path.string(),data,error) && filesystem::exists(path.string()+".bak"),"unrecovered backup protected");
        filesystem::remove(path.string()+".bak");
        writeFile(path.string()+".tmp","UNCOMMITTED DATA\n");
        check(loadData(path.string(),loaded,error)==LoadStatus::Error &&
              loaded.reservedSavingsCentavos==777 && !error.empty() &&
              readFile(path.string()+".tmp")=="UNCOMMITTED DATA\n" && !filesystem::exists(path),
              "missing main plus temporary is recovery error");
        check(!saveData(path.string(),data,error) &&
              readFile(path.string()+".tmp")=="UNCOMMITTED DATA\n" && !filesystem::exists(path),
              "missing main plus temporary refuses save");
        filesystem::remove(path.string()+".tmp");
        check(loadData(path.string(),loaded,error)==LoadStatus::NotFound && error.empty() &&
              loaded.reservedSavingsCentavos==777,"all data paths absent is genuine NotFound");
        check(loadData(directory.string(),loaded,error)==LoadStatus::Error,"directory not readable data");
        check(!saveData(directory.string(),data,error) && !error.empty() &&
              !filesystem::exists(directory.string()+".tmp"),"nonregular main refuses save before temporary write");
        filesystem::remove_all(directory);
        cout<<checks<<" checks passed\n";
        return 0;
    } catch (const exception& e) {
        cerr<<"FAILED after "<<checks<<" checks: "<<e.what()<<'\n';
        if (!directory.empty()) cerr<<"Fixtures: "<<directory<<'\n';
        return 1;
    }
}
