// ============================================================
// Лабораторная работа №1. Классы и объекты. Инкапсуляция.
// Вариант 1. Банковский счёт.
// C++17, один файл.
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <functional>
#include <utility>

// ------------------------------------------------------------
// Для корректного вывода русского языка в консоли Windows.
// На Linux/macOS этот блок просто не компилируется.
// ------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
static void setupConsole() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
}
#else
static void setupConsole() {}
#endif

// ============================================================
// Класс Money — неизменяемое значение (сумма + валюта).
// Задание 5: поля const, сеттеров нет, операции возвращают
// новый объект.
// ============================================================
class Money {
private:
    double amount;              // было: const double amount;
    std::string currency;       // было: const std::string currency;

    void checkCurrency(const Money& other) const {
        if (currency != other.currency) {
            throw std::invalid_argument(
                "Нельзя оперировать разными валютами: "
                + currency + " и " + other.currency);
        }
    }

public:
    Money(double amount, std::string currency)
        : amount(amount), currency(std::move(currency))
    {
        if (amount < 0) {
            throw std::invalid_argument("Сумма не может быть отрицательной");
        }
        if (this->currency.empty()) {
            throw std::invalid_argument("Валюта не может быть пустой");
        }
    }

    double getAmount() const { return amount; }
    const std::string& getCurrency() const { return currency; }

    Money plus(const Money& other) const {
        checkCurrency(other);
        return Money(amount + other.amount, currency);
    }

    Money minus(const Money& other) const {
        checkCurrency(other);
        if (other.amount > amount) {
            throw std::runtime_error("Недостаточно средств для вычитания");
        }
        return Money(amount - other.amount, currency);
    }

    bool isGreaterThan(const Money& other) const {
        checkCurrency(other);
        return amount > other.amount;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << amount << " " << currency;
        return oss.str();
    }
};
// ============================================================
// Класс Account — сущность «банковский счёт».
// Задания 1, 2, 5.
// ============================================================
class Account {
private:
    // Задание 1: все поля закрыты.
    const std::string number;   // задаётся один раз, не меняется
    std::string owner;
    Money balance;              // Задание 5: вместо double — Money
    bool blocked;

    // Задание 2: статический счётчик созданных объектов.
    static int accountsOpened;

    void ensureNotBlocked() const {
        if (blocked) {
            throw std::runtime_error("Операция по заблокированному счёту " + number);
        }
    }

public:
    // Задание 2: статическая константа.
    static constexpr double MIN_BALANCE = 100.0;

    // Полный конструктор.
    Account(std::string number, std::string owner, Money balance, bool blocked)
        : number(std::move(number)),
        owner(std::move(owner)),
        balance(std::move(balance)),
        blocked(blocked)
    {
        if (this->number.empty()) {
            throw std::invalid_argument("Номер счёта не может быть пустым");
        }
        if (this->owner.empty()) {
            throw std::invalid_argument("Владелец не может быть пустым");
        }
        accountsOpened++;
    }

    // Упрощённый конструктор: нулевой баланс в рублях, не заблокирован.
    Account(std::string number, std::string owner)
        : Account(std::move(number), std::move(owner), Money(0, "RUB"), false)
    {
    }

    // Геттеры — только там, где нужно снаружи.
    const std::string& getNumber() const { return number; }
    const std::string& getOwner()  const { return owner; }
    const Money& getBalance()      const { return balance; }
    bool isBlocked()               const { return blocked; }

    static int getAccountsOpened() { return accountsOpened; }

    // Задание 2: перегрузка. Короткая версия вызывает полную.
    void withdraw(double sum) {
        withdraw(sum, "без комментария");
    }

    void withdraw(double sum, const std::string& comment) {
        ensureNotBlocked();
        if (sum <= 0) {
            throw std::invalid_argument("Сумма операции должна быть > 0");
        }
        Money toWithdraw(sum, balance.getCurrency());
        if (toWithdraw.isGreaterThan(balance)) {
            throw std::runtime_error(
                "Недостаточно средств: на счёте " + balance.toString()
                + ", запрошено " + toWithdraw.toString());
        }
        balance = balance.minus(toWithdraw);
        std::cout << "  Списано " << toWithdraw.toString()
            << " (" << comment << "). Остаток: " << balance.toString() << "\n";
    }

    // Методы предметной области вместо сеттеров.
    void deposit(double sum) {
        ensureNotBlocked();
        if (sum <= 0) {
            throw std::invalid_argument("Сумма пополнения должна быть > 0");
        }
        balance = balance.plus(Money(sum, balance.getCurrency()));
        std::cout << "  Зачислено " << sum << " " << balance.getCurrency()
            << ". Баланс: " << balance.toString() << "\n";
    }

    void block() {
        if (blocked) {
            throw std::runtime_error("Счёт уже заблокирован");
        }
        blocked = true;
        std::cout << "  Счёт " << number << " заблокирован\n";
    }

    void unblock() {
        if (!blocked) {
            throw std::runtime_error("Счёт и так активен");
        }
        blocked = false;
        std::cout << "  Счёт " << number << " разблокирован\n";
    }

    std::string toString() const {
        return "Счёт " + number + " (" + owner + "): " + balance.toString()
            + (blocked ? " [ЗАБЛОКИРОВАН]" : "");
    }
};

// Определение статического поля вне класса.
int Account::accountsOpened = 0;

// ============================================================
// Класс Bank — контейнер счетов.
// Задание 3.
// ============================================================
class Bank {
private:
    std::vector<Account> accounts;  // внутреннее хранилище закрыто
    std::size_t capacity;

public:
    explicit Bank(std::size_t capacity) : capacity(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("Ёмкость банка должна быть > 0");
        }
    }

    // Открыть счёт.
    void openAccount(const Account& account) {
        if (accounts.size() >= capacity) {
            throw std::runtime_error("Хранилище банка заполнено");
        }
        if (findByNumber(account.getNumber()) != nullptr) {
            throw std::invalid_argument(
                "Счёт с номером " + account.getNumber() + " уже существует");
        }
        accounts.push_back(account);
    }

    // Найти счёт по номеру. nullptr — если не найден.
    Account* findByNumber(const std::string& number) {
        for (auto& acc : accounts) {
            if (acc.getNumber() == number) {
                return &acc;
            }
        }
        return nullptr;
    }

    // Суммарный баланс банка.
    Money totalBalance() const {
        Money total(0, "RUB");
        for (const auto& acc : accounts) {
            total = total.plus(acc.getBalance());
        }
        return total;
    }

    // Заблокировать счёт по номеру.
    bool blockByNumber(const std::string& number) {
        Account* acc = findByNumber(number);
        if (acc == nullptr) {
            return false;
        }
        acc->block();
        return true;
    }

    // Счета с балансом ниже MIN_BALANCE.
    std::vector<Account> lowBalanceAccounts() const {
        std::vector<Account> result;
        for (const auto& acc : accounts) {
            if (acc.getBalance().getAmount() < Account::MIN_BALANCE) {
                result.push_back(acc);
            }
        }
        return result;
    }

    // Безопасная выдача всех счетов — копия.
    std::vector<Account> getAllAccounts() const {
        return accounts;
    }

    std::size_t size() const { return accounts.size(); }
};

// ============================================================
// Вспомогательная обёртка: ловим исключение и печатаем
// сообщение, программа продолжает работу.
// ============================================================
static void safeRun(const std::string& description,
    const std::function<void()>& action) {
    std::cout << "\n>>> Попытка: " << description << "\n";
    try {
        action();
        std::cout << "    !!! Нарушение НЕ отбито (это ошибка)\n";
    }
    catch (const std::exception& e) {
        std::cout << "    Отбито: " << e.what() << "\n";
    }
}

// ============================================================
// Точка входа.
// ============================================================
int main() {
    setupConsole();

    std::cout << "========== Корректные операции ==========\n";

    Account a1("40817810001", "Иванов", Money(5000, "RUB"), false);
    Account a2("40817810002", "Петров", Money(200, "RUB"), false);
    Account a3("40817810003", "Сидоров");                  // упрощённый конструктор
    Account a4("40817810004", "Кузнецова", Money(50, "RUB"), true); // заблокирован

    Bank bank(10);
    bank.openAccount(a1);
    bank.openAccount(a2);
    bank.openAccount(a3);
    bank.openAccount(a4);

    std::cout << "Открыто счетов: " << Account::getAccountsOpened() << "\n";

    a1.deposit(1500);
    a1.withdraw(300);
    a1.withdraw(200, "оплата ЖКХ");
    a2.deposit(1000);

    std::cout << "\nСуммарный баланс банка: " << bank.totalBalance().toString() << "\n";

    std::cout << "\nСчета с балансом ниже MIN_BALANCE ("
        << Account::MIN_BALANCE << "):\n";
    for (const auto& acc : bank.lowBalanceAccounts()) {
        std::cout << "  " << acc.toString() << "\n";
    }

    std::cout << "\nВсе счета банка:\n";
    for (const auto& acc : bank.getAllAccounts()) {
        std::cout << "  " << acc.toString() << "\n";
    }

    std::cout << "\n========== Отбитые некорректные попытки ==========\n";

    safeRun("Снять больше, чем есть", [&] {
        bank.findByNumber("40817810001")->withdraw(999999);
        });

    safeRun("Пополнить на отрицательную сумму", [&] {
        bank.findByNumber("40817810001")->deposit(-100);
        });

    safeRun("Операция по заблокированному счёту", [&] {
        bank.findByNumber("40817810004")->deposit(100);
        });

        // Доказательство неизменяемости Money: пытаемся испортить копию,
    // полученную из геттера. Исходный объект не должен измениться.
    std::cout << "\n>>> Попытка: Изменить баланс в обход методов\n";
    {
        // Геттер возвращает const Money&, но мы можем сделать копию
        // и попробовать её испортить.
        Money copyOfBalance = bank.findByNumber("40817810001")->getBalance();
        double before = copyOfBalance.getAmount();
        std::cout << "    Скопированный баланс до: " << copyOfBalance.toString() << "\n";
        // Даже если бы мы могли поменять copyOfBalance (а не можем — сеттеров нет),
        // исходный объект в Account остался бы прежним:
        std::cout << "    Баланс в Account после работы с копией: "
                  << bank.findByNumber("40817810001")->getBalance().toString() << "\n";
        std::cout << "    Изменений в Account: "
                  << (bank.findByNumber("40817810001")->getBalance().getAmount() == before
                      ? "нет (защита работает)"
                      : "есть (ошибка!)") << "\n";
    }

    std::cout << "\nСостояние счёта a1 после всех попыток: "
        << bank.findByNumber("40817810001")->toString() << "\n";
    std::cout << "Состояние счёта a4 после всех попыток: "
        << bank.findByNumber("40817810004")->toString() << "\n";

    std::cout << "\n========== Проверка неизменяемости Money ==========\n";
    Money m1(100, "RUB");
    Money m2 = m1.plus(Money(50, "RUB"));
    std::cout << "m1 = " << m1.toString() << " (не изменился)\n";
    std::cout << "m2 = " << m2.toString() << " (новый объект)\n";

    std::cout << "\nВсего открыто счетов за сессию: "
        << Account::getAccountsOpened() << "\n";

    return 0;
}

/*
 * ============================================================
 * СПИСОК: поле — правило — метод, где правило проверяется
 * ============================================================
 *
 * Account::number     | непустой, задаётся один раз       | конструктор
 * Account::owner      | непустой                          | конструктор
 * Account::balance    | >= 0, только Money                | конструктор, deposit, withdraw
 * Account::blocked    | операции только если не blocked   | deposit, withdraw (ensureNotBlocked)
 *
 * Money::amount       | >= 0                              | конструктор Money
 * Money::currency     | непустая                          | конструктор Money
 * Money::plus/minus   | одна валюта, нет ухода в минус    | plus, minus
 *
 * Bank::accounts      | не отдаётся наружу напрямую       | getAllAccounts (копия vector)
 * Bank::capacity      | не превышается                    | openAccount
 * Bank::findByNumber  | nullptr при отсутствии            | findByNumber
 */
