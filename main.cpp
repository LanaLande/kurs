#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <curses.h>
#include <stack>
#include <memory>
#include <algorithm>
#include <map>
#include <iomanip>  // Для std::setw

using namespace std;

// Константы
const string USERS_FILE = "users.txt";
const string PROJECTS_FILE = "projects.txt";
const string MSG_LOGIN_PROMPT = "Логин: ";
const string MSG_PASSWORD_PROMPT = "Пароль: ";
const string MSG_INVALID_CREDENTIALS = "Неверный логин или пароль.";
const string MSG_WELCOME_ADMIN = "Добро пожаловать, администратор!";
const string MSG_WELCOME_USER = "Добро пожаловать, пользователь!";
const string MSG_CONFIRM_DELETE = "Вы действительно хотите удалить запись? (y/n): ";
const string MSG_SUCCESS_DELETE = "Запись успешно удалена.";
const string MSG_SUCCESS_ADD = "Запись успешно добавлена.";
const string MSG_ERROR_FORMAT = "Ошибка: неверный формат данных.";
const string MSG_ERROR_NOT_FOUND = "Запись не найдена.";

// Структура учетной записи пользователя
struct User {
    string login;
    string password; // В реальном проекте использовать хэширование
    int role;        // 1 - администратор, 0 - пользователь
    int access;      // 1 - подтвержден, 0 - ожидает подтверждения
};

// Структура записи о проекте
struct ProjectRecord {
    int id;
    string projectName;
    string workType;    // Например: "работа над требованиями", "тестирование"
    string employeeName;
    int hours;
    double hourlyCost;
};

// Функция для загрузки пользователей
vector<User> loadUsers(const string& filename) {
    vector<User> users;
    ifstream file(filename);
    if (!file.is_open()) {
        ofstream outfile(filename);
        outfile << "admin|admin123|1|1\n"; // Администратор по умолчанию
        outfile.close();
        file.open(filename);
    }
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string field;
        User user;
        getline(ss, field, '|'); user.login = field;
        getline(ss, field, '|'); user.password = field;
        getline(ss, field, '|'); user.role = stoi(field);
        getline(ss, field, '|'); user.access = stoi(field);
        users.push_back(user);
    }
    file.close();
    return users;
}

// Функция для сохранения пользователей
void saveUsers(const vector<User>& users, const string& filename) {
    ofstream file(filename, ios::trunc);
    for (const auto& user : users) {
        file << user.login << "|" << user.password << "|" << user.role << "|" << user.access << "\n";
    }
    file.close();
}

// Сервис для работы с интерфейсом
class UIService {
public:
    UIService() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        start_color();
    }

    ~UIService() {
        endwin();
    }

    void printTableTopRight(const vector<ProjectRecord>& records) const {
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);

        const int colWidthId = 5;
        const int colWidthName = 15;
        const int colWidthType = 15;
        const int colWidthEmployee = 15;
        const int colWidthHours = 5;
        const int colWidthCost = 11;
        const int totalWidth = colWidthId + colWidthName + colWidthType + colWidthEmployee + colWidthHours + colWidthCost;

        int startX = maxX - totalWidth - 1;
        int startY = 0;
        if (startX < 0) startX = 0;

        for (int y = startY; y < maxY - 2 && y < records.size() + 2; y++) {
            move(y, startX);
            clrtoeol();
        }

        mvprintw(startY, startX, "ID   |Project        |Work Type      |Employee       |Hours|Hourly Cost");
        mvprintw(startY + 1, startX, "-----|---------------|---------------|---------------|-----|-----------");

        for (size_t i = 0; i < records.size() && startY + i + 2 < maxY - 2; i++) {
            const ProjectRecord& rec = records[i];
            stringstream ss;
            ss << left << setw(colWidthId) << rec.id << "|"
               << setw(colWidthName) << rec.projectName << "|"
               << setw(colWidthType) << rec.workType << "|"
               << setw(colWidthEmployee) << rec.employeeName << "|"
               << setw(colWidthHours) << rec.hours << "|"
               << setw(colWidthCost) << rec.hourlyCost;
            mvprintw(startY + i + 2, startX, "%s", ss.str().c_str());
        }
    }

    void clearScreen() const {
        clear();
    }

    void refreshScreen() const {
        refresh();
    }

    int getInput() const {
        return getch();
    }

    void printMessage(int y, int x, const string& msg) const {
        mvprintw(y, x, "%s", msg.c_str());
    }

    void getStringInput(int y, int x, const string& prompt, char* buffer, int size) const {
        printMessage(y, x, prompt);
        refresh();
        echo();
        move(y, x + prompt.length());
        getnstr(buffer, size);
        noecho();
    }

    int getIntInput(int y, int x, const string& prompt) const {
        while (true) {
            printMessage(y, x, prompt);
            refresh();
            char buffer[10];
            echo();
            getnstr(buffer, 10);
            noecho();
            try {
                return stoi(buffer);
            } catch (const invalid_argument&) {
                printMessage(y + 1, x, "Ошибка: введите целое число.");
                refresh();
                getch();
                clearScreen();
            } catch (const out_of_range&) {
                printMessage(y + 1, x, "Ошибка: число слишком большое.");
                refresh();
                getch();
                clearScreen();
            }
        }
    }

    double getDoubleInput(int y, int x, const string& prompt) const {
        while (true) {
            printMessage(y, x, prompt);
            refresh();
            char buffer[20];
            echo();
            getnstr(buffer, 20);
            noecho();
            try {
                return stod(buffer);
            } catch (const invalid_argument&) {
                printMessage(y + 1, x, "Ошибка: введите число.");
                refresh();
                getch();
                clearScreen();
            } catch (const out_of_range&) {
                printMessage(y + 1, x, "Ошибка: число слишком большое.");
                refresh();
                getch();
                clearScreen();
            }
        }
    }

    pair<int, int> getMaxYX() const {
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        return {maxY, maxX};
    }
};

// Сервис для работы с базой данных проектов
class ProjectDatabaseService {
private:
    string filename;

public:
    ProjectDatabaseService(const string& fname) : filename(fname) {}

    vector<ProjectRecord> load() const {
        vector<ProjectRecord> records;
        ifstream file(filename);
        if (!file.is_open()) return records; // Пустой вектор, если файл не существует
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string field;
            ProjectRecord rec;
            try {
                getline(ss, field, '|'); rec.id = stoi(field);
                getline(ss, field, '|'); rec.projectName = field;
                getline(ss, field, '|'); rec.workType = field;
                getline(ss, field, '|'); rec.employeeName = field;
                getline(ss, field, '|'); rec.hours = stoi(field);
                getline(ss, field, '|'); rec.hourlyCost = stod(field);
                records.push_back(rec);
            } catch (...) { // Исправлено: убрано "facie"
                continue; // Пропускаем некорректные записи
            }
        }
        file.close();
        return records;
    }

    void save(const vector<ProjectRecord>& records) const {
        ofstream file(filename, ios::trunc);
        for (const auto& rec : records) {
            file << rec.id << "|" << rec.projectName << "|" << rec.workType << "|"
                 << rec.employeeName << "|" << rec.hours << "|" << rec.hourlyCost << "\n";
        }
        file.close();
    }

    void insert(const ProjectRecord& rec) {
        vector<ProjectRecord> records = load();
        records.push_back(rec);
        save(records);
    }

    void remove(int id) {
        vector<ProjectRecord> records = load();
        records.erase(remove_if(records.begin(), records.end(),
                                [id](const ProjectRecord& r) { return r.id == id; }), records.end());
        save(records);
    }
};

// Аутентификация с маскировкой пароля
User authenticate(const vector<User>& users, UIService& ui) {
    char login[50], password[50];
    ui.getStringInput(0, 0, MSG_LOGIN_PROMPT, login, 50);

    mvprintw(1, 0, "%s", MSG_PASSWORD_PROMPT.c_str());
    refresh();
    int i = 0;
    noecho();
    while (i < 49) {
        int ch = getch();
        if (ch == '\n') break;
        if (ch == KEY_BACKSPACE && i > 0) {
            i--;
            mvprintw(1, MSG_PASSWORD_PROMPT.length() + i, " ");
        } else if (ch != KEY_BACKSPACE) {
            password[i++] = ch;
            mvprintw(1, MSG_PASSWORD_PROMPT.length() + i - 1, "*");
        }
        refresh();
    }
    password[i] = '\0';
    echo();

    for (const auto& user : users) {
        if (user.login == login && user.password == password && user.access == 1) {
            return user;
        }
    }
    return User{"", "", -1, -1}; // Неудачная аутентификация
}

// Абстрактный базовый класс для экранов
class Screen {
public:
    virtual void render(UIService& ui, const ProjectDatabaseService& db) = 0;
    virtual unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) = 0;
    virtual ~Screen() = default;
};

// Главное меню (объявление handleInput)
class MainMenuScreen : public Screen {
public:
    void render(UIService& ui, const ProjectDatabaseService& db) override {
        auto [maxY, maxX] = ui.getMaxYX();
        ui.printMessage(maxY - 2, 0, "1 - Вставка | 2 - Удалить | 3 - Редактировать | 4 - Просмотр | 5 - Выход");
        ui.printMessage(maxY - 1, 0, "Выберите опцию: ");
        ui.printTableTopRight(db.load());
        ui.refreshScreen();
    }

    unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) override;
};

// Экран вставки (с автоматическим ID)
class InsertScreen : public Screen {
public:
    void render(UIService& ui, const ProjectDatabaseService& db) override {
        auto [maxY, maxX] = ui.getMaxYX();
        ui.printMessage(0, 0, "Экран вставки записи");
        ui.printMessage(maxY - 2, 0, "Введите данные для новой записи (или 'b' для возврата):");
        ui.printTableTopRight(db.load());
        ui.refreshScreen();
    }

    unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) override {
        if (input == 'b') {
            navStack.pop();
            return nullptr;
        }

        ProjectRecord newRec;
        // Автоматическая генерация уникального ID
        vector<ProjectRecord> records = db.load();
        int maxId = 0;
        for (const auto& rec : records) {
            if (rec.id > maxId) maxId = rec.id;
        }
        newRec.id = maxId + 1;

        UIService ui;
        char projectName[50]; ui.getStringInput(1, 0, "Название проекта: ", projectName, 50); newRec.projectName = projectName;
        char workType[50]; ui.getStringInput(2, 0, "Вид работ: ", workType, 50); newRec.workType = workType;
        char employeeName[50]; ui.getStringInput(3, 0, "Ф.И.О. сотрудника: ", employeeName, 50); newRec.employeeName = employeeName;
        newRec.hours = ui.getIntInput(4, 0, "Часы: ");
        newRec.hourlyCost = ui.getDoubleInput(5, 0, "Стоимость часа: ");

        db.insert(newRec);
        ui.printMessage(ui.getMaxYX().first - 1, 0, MSG_SUCCESS_ADD);
        getch();
        return nullptr;
    }
};

// Экран удаления
class DeleteScreen : public Screen {
public:
    void render(UIService& ui, const ProjectDatabaseService& db) override {
        auto [maxY, maxX] = ui.getMaxYX();
        ui.printMessage(0, 0, "Экран удаления записи");
        ui.printMessage(maxY - 2, 0, "Введите ID для удаления (или 'b' для возврата):");
        ui.printTableTopRight(db.load());
        ui.refreshScreen();
    }

    unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) override {
        if (input == 'b') {
            navStack.pop();
            return nullptr;
        }

        UIService ui;
        int id = ui.getIntInput(ui.getMaxYX().first - 1, 0, "ID: ");
        db.remove(id);
        ui.printMessage(ui.getMaxYX().first - 1, 0, MSG_SUCCESS_DELETE);
        getch();
        return nullptr;
    }
};

// Экран редактирования
class EditScreen : public Screen {
public:
    void render(UIService& ui, const ProjectDatabaseService& db) override {
        auto [maxY, maxX] = ui.getMaxYX();
        ui.printMessage(0, 0, "Экран редактирования записи");
        ui.printMessage(maxY - 2, 0, "Введите ID для редактирования (или 'b' для возврата):");
        ui.printTableTopRight(db.load());
        ui.refreshScreen();
    }

    unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) override {
        if (input == 'b') {
            navStack.pop();
            return nullptr;
        }

        UIService ui;
        int id = ui.getIntInput(ui.getMaxYX().first - 1, 0, "ID: ");
        vector<ProjectRecord> records = db.load();
        for (auto& rec : records) {
            if (rec.id == id) {
                char projectName[50]; ui.getStringInput(1, 0, "Новое название проекта: ", projectName, 50); rec.projectName = projectName;
                char workType[50]; ui.getStringInput(2, 0, "Новый вид работ: ", workType, 50); rec.workType = workType;
                char employeeName[50]; ui.getStringInput(3, 0, "Новое Ф.И.О. сотрудника: ", employeeName, 50); rec.employeeName = employeeName;
                rec.hours = ui.getIntInput(4, 0, "Новые часы: ");
                rec.hourlyCost = ui.getDoubleInput(5, 0, "Новая стоимость часа: ");
                db.save(records);
                ui.printMessage(ui.getMaxYX().first - 1, 0, "Запись обновлена.");
                getch();
                return nullptr;
            }
        }
        ui.printMessage(ui.getMaxYX().first - 1, 0, MSG_ERROR_NOT_FOUND);
        getch();
        return nullptr;
    }
};

// Экран просмотра
class ViewScreen : public Screen {
public:
    void render(UIService& ui, const ProjectDatabaseService& db) override {
        ui.clearScreen();
        vector<ProjectRecord> records = db.load();
        ui.printTableTopRight(records);
        ui.refreshScreen();
        getch();
    }

    unique_ptr<Screen> handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) override {
        navStack.pop();
        return nullptr;
    }
};

// Определение handleInput для MainMenuScreen
unique_ptr<Screen> MainMenuScreen::handleInput(int input, stack<unique_ptr<Screen>>& navStack, ProjectDatabaseService& db, bool isAdmin) {
    switch (input) {
        case '1': return make_unique<InsertScreen>();
        case '2': return make_unique<DeleteScreen>();
        case '3': return make_unique<EditScreen>();
        case '4': return make_unique<ViewScreen>();
        case '5': while (!navStack.empty()) navStack.pop(); return nullptr; // Выход
        default: return nullptr;
    }
}

// Сервис навигации
class NavigationService {
private:
    stack<unique_ptr<Screen>> navStack;
    ProjectDatabaseService& db;
    UIService& ui;
    bool isAdmin;

public:
    NavigationService(ProjectDatabaseService& database, UIService& uiService, bool admin)
        : db(database), ui(uiService), isAdmin(admin) {
        navStack.push(make_unique<MainMenuScreen>());
    }

    void run() {
        while (!navStack.empty()) {
            ui.clearScreen();
            Screen* currentScreen = navStack.top().get();
            currentScreen->render(ui, db);
            int input = ui.getInput();
            unique_ptr<Screen> nextScreen = currentScreen->handleInput(input, navStack, db, isAdmin);
            if (nextScreen) navStack.push(move(nextScreen));
            if (input == 'b' && navStack.size() > 1) navStack.pop();
        }
    }
};

// Главная функция
int main() {
    UIService ui;
    vector<User> users = loadUsers(USERS_FILE);
    User currentUser = authenticate(users, ui);
    if (currentUser.role == -1) {
        ui.printMessage(3, 0, MSG_INVALID_CREDENTIALS);
        refresh();
        getch();
        return 1;
    }
    ui.clearScreen();
    ui.printMessage(0, 0, currentUser.role == 1 ? MSG_WELCOME_ADMIN : MSG_WELCOME_USER);
    refresh();
    getch();

    ProjectDatabaseService projectDb(PROJECTS_FILE);
    NavigationService nav(projectDb, ui, currentUser.role == 1);
    nav.run();
    return 0;
}