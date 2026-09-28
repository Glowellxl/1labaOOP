import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class Main {

    // ============================================================
    // Класс Money — неизменяемый (immutable) класс
    // ============================================================
    public static final class Money {
        private final double amount;
        private final String currency;

        private void checkCurrency(Money other) {
            if (!currency.equals(other.currency)) {
                throw new IllegalArgumentException(
                        "Нельзя оперировать разными валютами: "
                                + currency + " и " + other.currency);
            }
        }

        public Money(double amount, String currency) {
            if (amount < 0) {
                throw new IllegalArgumentException("Сумма не может быть отрицательной");
            }
            if (currency == null || currency.isEmpty()) {
                throw new IllegalArgumentException("Валюта не может быть пустой");
            }
            this.amount = amount;
            this.currency = currency;
        }

        public double getAmount() { return amount; }
        public String getCurrency() { return currency; }

        public Money plus(Money other) {
            checkCurrency(other);
            return new Money(amount + other.amount, currency);
        }

        public Money minus(Money other) {
            checkCurrency(other);
            if (other.amount > amount) {
                throw new RuntimeException("Недостаточно средств для вычитания");
            }
            return new Money(amount - other.amount, currency);
        }

        public boolean isGreaterThan(Money other) {
            checkCurrency(other);
            return amount > other.amount;
        }

        @Override
        public String toString() {
            return String.format("%.2f %s", amount, currency);
        }
    }

    // ============================================================
    // Класс Account
    // ============================================================
    public static final class Account {
        private final String number;
        private String owner;
        private Money balance;
        private boolean blocked;

        // Задание 2: статический счётчик созданных объектов.
        private static int accountsOpened = 0;

        public static final double MIN_BALANCE = 100.0;

        private void ensureNotBlocked() {
            if (blocked) {
                throw new RuntimeException("Операция по заблокированному счёту " + number);
            }
        }

        // Полный конструктор.
        public Account(String number, String owner, Money balance, boolean blocked) {
            if (number == null || number.isEmpty()) {
                throw new IllegalArgumentException("Номер счёта не может быть пустым");
            }
            if (owner == null || owner.isEmpty()) {
                throw new IllegalArgumentException("Владелец не может быть пустым");
            }
            this.number = number;
            this.owner = owner;
            this.balance = balance;
            this.blocked = blocked;
            accountsOpened++;
        }

        // Упрощённый конструктор: нулевой баланс в рублях, не заблокирован.
        public Account(String number, String owner) {
            this(number, owner, new Money(0, "RUB"), false);
        }

        public String getNumber() { return number; }
        public String getOwner()  { return owner; }
        public Money getBalance() { return balance; }
        public boolean isBlocked() { return blocked; }

        public static int getAccountsOpened() { return accountsOpened; }

        // Перегрузка. Короткая версия вызывает полную.
        public void withdraw(double sum) {
            withdraw(sum, "без комментария");
        }

        public void withdraw(double sum, String comment) {
            ensureNotBlocked();
            if (sum <= 0) {
                throw new IllegalArgumentException("Сумма операции должна быть > 0");
            }
            Money toWithdraw = new Money(sum, balance.getCurrency());
            if (toWithdraw.isGreaterThan(balance)) {
                throw new RuntimeException(
                        "Недостаточно средств: на счёте " + balance
                                + ", запрошено " + toWithdraw);
            }
            balance = balance.minus(toWithdraw);
            System.out.println("  Списано " + toWithdraw
                    + " (" + comment + "). Остаток: " + balance);
        }

        public void deposit(double sum) {
            ensureNotBlocked();
            if (sum <= 0) {
                throw new IllegalArgumentException("Сумма пополнения должна быть > 0");
            }
            balance = balance.plus(new Money(sum, balance.getCurrency()));
            System.out.println("  Зачислено " + sum + " " + balance.getCurrency()
                    + ". Баланс: " + balance);
        }

        public void block() {
            if (blocked) {
                throw new RuntimeException("Счёт уже заблокирован");
            }
            blocked = true;
            System.out.println("  Счёт " + number + " заблокирован");
        }

        public void unblock() {
            if (!blocked) {
                throw new RuntimeException("Счёт и так активен");
            }
            blocked = false;
            System.out.println("  Счёт " + number + " разблокирован");
        }

        @Override
        public String toString() {
            return "Счёт " + number + " (" + owner + "): " + balance
                    + (blocked ? " [ЗАБЛОКИРОВАН]" : "");
        }
    }

    // ============================================================
    // Класс Bank — контейнер счетов.
    // ============================================================
    public static final class Bank {
        private final List<Account> accounts = new ArrayList<>();
        private final int capacity;

        public Bank(int capacity) {
            if (capacity == 0) {
                throw new IllegalArgumentException("Ёмкость банка должна быть > 0");
            }
            this.capacity = capacity;
        }

        public void openAccount(Account account) {
            if (accounts.size() >= capacity) {
                throw new RuntimeException("Хранилище банка заполнено");
            }
            if (findByNumber(account.getNumber()) != null) {
                throw new IllegalArgumentException(
                        "Счёт с номером " + account.getNumber() + " уже существует");
            }
            accounts.add(account);
        }

        public Account findByNumber(String number) {
            for (Account acc : accounts) {
                if (acc.getNumber().equals(number)) {
                    return acc;
                }
            }
            return null;
        }

        public Money totalBalance() {
            Money total = new Money(0, "RUB");
            for (Account acc : accounts) {
                total = total.plus(acc.getBalance());
            }
            return total;
        }

        public boolean blockByNumber(String number) {
            Account acc = findByNumber(number);
            if (acc == null) {
                return false;
            }
            acc.block();
            return true;
        }

        public List<Account> lowBalanceAccounts() {
            List<Account> result = new ArrayList<>();
            for (Account acc : accounts) {
                if (acc.getBalance().getAmount() < Account.MIN_BALANCE) {
                    result.add(acc);
                }
            }
            return result;
        }

        public List<Account> getAllAccounts() {
            return new ArrayList<>(accounts);
        }

        public int size() { return accounts.size(); }
    }

    // ============================================================
    // Вспомогательная обёртка: ловим исключение и печатаем сообщение,
    // программа продолжает работу.
    // ============================================================
    @FunctionalInterface
    interface Action {
        void run();
    }

    private static void safeRun(String description, Action action) {
        System.out.println("\n>>> Попытка: " + description);
        try {
            action.run();
            System.out.println("    !!! Нарушение НЕ отбито (это ошибка)");
        } catch (Exception e) {
            System.out.println("    Отбито: " + e.getMessage());
        }
    }

    // ============================================================
    // Точка входа.
    // ============================================================
    public static void main(String[] args) {
        System.out.println("========== Корректные операции ==========");

        Account a1 = new Account("40817810001", "Иванов", new Money(5000, "RUB"), false);
        Account a2 = new Account("40817810002", "Петров", new Money(200, "RUB"), false);
        Account a3 = new Account("40817810003", "Сидоров");
        Account a4 = new Account("40817810004", "Кузнецова", new Money(50, "RUB"), true);

        Bank bank = new Bank(10);
        bank.openAccount(a1);
        bank.openAccount(a2);
        bank.openAccount(a3);
        bank.openAccount(a4);

        System.out.println("Открыто счетов: " + Account.getAccountsOpened());

        a1.deposit(1500);
        a1.withdraw(300);
        a1.withdraw(200, "оплата ЖКХ");
        a2.deposit(1000);

        System.out.println("\nСуммарный баланс банка: " + bank.totalBalance());

        System.out.println("\nСчета с балансом ниже MIN_BALANCE ("
                + Account.MIN_BALANCE + "):");
        for (Account acc : bank.lowBalanceAccounts()) {
            System.out.println("  " + acc);
        }

        System.out.println("\nВсе счета банка:");
        for (Account acc : bank.getAllAccounts()) {
            System.out.println("  " + acc);
        }

        System.out.println("\n========== Отбитые некорректные попытки ==========");

        safeRun("Снять больше, чем есть", () ->
                bank.findByNumber("40817810001").withdraw(999999));

        safeRun("Пополнить на отрицательную сумму", () ->
                bank.findByNumber("40817810001").deposit(-100));

        safeRun("Операция по заблокированному счёту", () ->
                bank.findByNumber("40817810004").deposit(100));

        // Доказательство неизменяемости Money.
        System.out.println("\n>>> Попытка: Изменить баланс в обход методов");
        {
            Money copyOfBalance = bank.findByNumber("40817810001").getBalance();
            double before = copyOfBalance.getAmount();
            System.out.println("    Скопированный баланс до: " + copyOfBalance);
            System.out.println("    Баланс в Account после работы с копией: "
                    + bank.findByNumber("40817810001").getBalance());
            System.out.println("    Изменений в Account: "
                    + (bank.findByNumber("40817810001").getBalance().getAmount() == before
                    ? "нет (защита работает)"
                    : "есть (ошибка!)"));
        }

        System.out.println("\nСостояние счёта a1 после всех попыток: "
                + bank.findByNumber("40817810001"));
        System.out.println("Состояние счёта a4 после всех попыток: "
                + bank.findByNumber("40817810004"));

        System.out.println("\n========== Проверка неизменяемости Money ==========");
        Money m1 = new Money(100, "RUB");
        Money m2 = m1.plus(new Money(50, "RUB"));
        System.out.println("m1 = " + m1 + " (не изменился)");
        System.out.println("m2 = " + m2 + " (новый объект)");

        System.out.println("\nВсего открыто счетов за сессию: "
                + Account.getAccountsOpened());
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
     * Bank::accounts      | не отдаётся наружу напрямую       | getAllAccounts (копия List)
     * Bank::capacity      | не превышается                    | openAccount
     * Bank::findByNumber  | null при отсутствии               | findByNumber
     */
}
