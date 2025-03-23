#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <curses.h>
#include <openssl/sha.h>

// Константы
const std::string ACCOUNTS_FILE = "accounts.txt";
const std::string PROJECTS_FILE = "projects.txt";
const std::string DEFAULT_ADMIN_LOGIN = "admin";
const std::string DEFAULT_ADMIN_PASSWORD = "admin123";
const std::string MSG_LOGIN_EXISTS = "Логин уже существует!";
const std::string MSG_SUCCESS = "Операция выполнена успешно!";
const std::string MSG_CONFIRM_DELETE = "Вы действительно хотите удалить запись? (y/n): ";
const std::string MSG_INVALID_INPUT = "Неверный ввод!";
const std::string MSG_NOT_FOUND = "Данные не найдены!";
const int ROLE_ADMIN = 1;
const int ROLE_USER = 0;
const int ACCESS_PENDING = 0;
const int ACCESS_APPROVED = 1;
const int ACCESS_BLOCKED = 2;
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 24;

// Структура для учётной записи
struct Account {
    std::string login;
    std::string salted_hash_password;
    std::string salt;
    int role;
    int access;
};

// Структура для проекта
struct Project {
    std::string name;
    std::string work_type;
    std::string employee;
    int hours;
    double hour_cost;
};

// Глобальные переменные
Account* currentUser = nullptr;

// Функции для хэширования паролей
std::string generateSalt() {
    std::string salt = "salt" + std::to_string(rand() % 10000);
    return salt;
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    std::string input = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    std::string hashed;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        hashed += hex;
    }
    return hashed;
}

// Функции для работы с файлами
std::vector<Account> readAccounts() {
    std::vector<Account> accounts;
    std::ifstream file(ACCOUNTS_FILE);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> tokens;
            std::string token;
            for (char c : line) {
                if (c == ';') {
                    tokens.push_back(token);
                    token.clear();
                } else {
                    token += c;
                }
            }
            tokens.push_back(token);
            if (tokens.size() == 5) {
                Account acc{tokens[0], tokens[1], tokens[2], std::stoi(tokens[3]), std::stoi(tokens[4])};
                accounts.push_back(acc);
            }
        }
        file.close();
    }
    return accounts;
}

void writeAccounts(const std::vector<Account>& accounts) {
    std::ofstream file(ACCOUNTS_FILE);
    if (file.is_open()) {
        for (const auto& acc : accounts) {
            file << acc.login << ";" << acc.salted_hash_password << ";" << acc.salt << ";"
                 << acc.role << ";" << acc.access << "\n";
        }
        file.close();
    }
}

std::vector<Project> readProjects() {
    std::vector<Project> projects;
    std::ifstream file(PROJECTS_FILE);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> tokens;
            std::string token;
            for (char c : line) {
                if (c == ';') {
                    tokens.push_back(token);
                    token.clear();
                } else {
                    token += c;
                }
            }
            tokens.push_back(token);
            if (tokens.size() == 5) {
                Project proj{tokens[0], tokens[1], tokens[2], std::stoi(tokens[3]), std::stod(tokens[4])};
                projects.push_back(proj);
            }
        }
        file.close();
    }
    return projects;
}

void writeProjects(const std::vector<Project>& projects) {
    std::ofstream file(PROJECTS_FILE);
    if (file.is_open()) {
        for (const auto& proj : projects) {
            file << proj.name << ";" << proj.work_type << ";" << proj.employee << ";"
                 << proj.hours << ";" << proj.hour_cost << "\n";
        }
        file.close();
    }
}

// Функции авторизации и регистрации
bool authenticate(const std::string& login, const std::string& password) {
    auto accounts = readAccounts();
    for (auto& acc : accounts) {
        if (acc.login == login) {
            std::string hashed = hashPassword(password, acc.salt);
            if (hashed == acc.salted_hash_password && acc.access == ACCESS_APPROVED) {
                currentUser = new Account(acc);
                return true;
            }
        }
    }
    return false;
}

void registerUser(const std::string& login, const std::string& password, int role, int access) {
    auto accounts = readAccounts();
    for (const auto& acc : accounts) {
        if (acc.login == login) {
            mvprintw(SCREEN_HEIGHT - 1, 0, MSG_LOGIN_EXISTS.c_str());
            refresh();
            return;
        }
    }
    Account newAcc;
    newAcc.login = login;
    newAcc.salt = generateSalt();
    newAcc.salted_hash_password = hashPassword(password, newAcc.salt);
    newAcc.role = role;
    newAcc.access = access;
    accounts.push_back(newAcc);
    writeAccounts(accounts);
    mvprintw(SCREEN_HEIGHT - 1, 0, MSG_SUCCESS.c_str());
    refresh();
}

// Экран ввода строки с pdcurses
std::string inputString(int y, int x, const std::string& prompt, bool mask = false) {
    mvprintw(y, x, prompt.c_str());
    std::string input;
    int ch;
    while ((ch = getch()) != '\n') {
        if (ch == KEY_BACKSPACE || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
                mvprintw(y, x + prompt.length() + input.length(), " ");
                move(y, x + prompt.length() + input.length());
            }
        } else if (ch >= 32 && ch <= 126) {
            input += static_cast<char>(ch);
            if (mask) {
                mvprintw(y, x + prompt.length() + input.length() - 1, "*");
            } else {
                mvprintw(y, x + prompt.length() + input.length() - 1, "%c", ch);
            }
        }
        refresh();
    }
    return input;
}

int inputInt(int y, int x, const std::string& prompt) {
    while (true) {
        std::string input = inputString(y, x, prompt);
        try {
            return std::stoi(input);
        } catch (const std::exception& e) {
            mvprintw(y + 1, x, "Ошибка: введите целое число!");
            refresh();
            napms(1000);
            mvprintw(y + 1, x, "                            "); // Очистка сообщения об ошибке
            mvprintw(y, x + prompt.length(), "          "); // Очистка поля ввода
            move(y, x + prompt.length()); // Возврат курсора
            refresh();
        }
    }
}

double inputDouble(int y, int x, const std::string& prompt) {
    while (true) {
        std::string input = inputString(y, x, prompt);
        try {
            return std::stod(input);
        } catch (const std::exception& e) {
            mvprintw(y + 1, x, "Ошибка: введите число!");
            refresh();
            napms(1000);
            mvprintw(y + 1, x, "                            "); // Очистка сообщения об ошибке
            mvprintw(y, x + prompt.length(), "          "); // Очистка поля ввода
            move(y, x + prompt.length()); // Возврат курсора
            refresh();
        }
    }
}

// Экран авторизации
void loginScreen() {
    clear();
    mvprintw(0, 0, "=== Авторизация ===");
    std::string login = inputString(2, 0, "Логин: ");
    std::string password = inputString(4, 0, "Пароль: ", true);
    if (authenticate(login, password)) {
        mvprintw(6, 0, "Вход выполнен успешно!");
        refresh();
        napms(1000); // Задержка 1 секунда, чтобы пользователь увидел сообщение
    } else {
        mvprintw(6, 0, "Неверный логин или пароль!");
        refresh();
        napms(1000); // Задержка перед возвратом
    }
}

// Экран регистрации
void registerScreen(bool byAdmin = false) {
    clear();
    mvprintw(0, 0, "=== Регистрация ===");
    std::string login = inputString(2, 0, "Логин: ");
    std::string password = inputString(4, 0, "Пароль: ", true);
    int role = byAdmin ? ROLE_USER : ROLE_USER;
    int access = byAdmin ? ACCESS_APPROVED : ACCESS_APPROVED; // Изменено с ACCESS_PENDING на ACCESS_APPROVED
    registerUser(login, password, role, access);
    napms(1000);
}

// Экран просмотра проектов
void viewProjectsScreen() {
    clear();
    mvprintw(0, 0, "=== Просмотр проектов ===");
    auto projects = readProjects();
    int y = 2;
    for (size_t i = 0; i < projects.size() && y < SCREEN_HEIGHT - 1; ++i) {
        std::string line = std::to_string(i + 1) + ". " + projects[i].name + " - " + projects[i].work_type;
        mvprintw(y++, 0, line.c_str());
    }
    mvprintw(SCREEN_HEIGHT - 1, 0, "Нажмите любую клавишу для возврата...");
    refresh();
    getch();
}

// Экран добавления проекта
void addProjectScreen() {
    clear();
    mvprintw(0, 0, "=== Добавление проекта ===");
    Project proj;
    proj.name = inputString(2, 0, "Наименование проекта: ");
    proj.work_type = inputString(4, 0, "Вид работ: ");
    proj.employee = inputString(6, 0, "Ф.И.О. сотрудника: ");
    proj.hours = inputInt(8, 0, "Часы: "); // Используем inputInt
    proj.hour_cost = inputDouble(10, 0, "Стоимость часа: "); // Используем inputDouble
    auto projects = readProjects();
    projects.push_back(proj);
    writeProjects(projects);
    mvprintw(SCREEN_HEIGHT - 1, 0, MSG_SUCCESS.c_str());
    refresh();
    napms(1000);
}

// Экран редактирования проекта
void editProjectScreen() {
    clear();
    mvprintw(0, 0, "=== Редактирование проекта ===");
    int index = std::stoi(inputString(2, 0, "Введите номер проекта: ")) - 1;
    auto projects = readProjects();
    if (index >= 0 && index < projects.size()) {
        Project& proj = projects[index];
        proj.name = inputString(4, 0, "Новое наименование проекта: ");
        proj.work_type = inputString(6, 0, "Новый вид работ: ");
        proj.employee = inputString(8, 0, "Новое Ф.И.О. сотрудника: ");
        proj.hours = std::stoi(inputString(10, 0, "Новые часы: "));
        proj.hour_cost = std::stod(inputString(12, 0, "Новая стоимость часа: "));
        writeProjects(projects);
        mvprintw(SCREEN_HEIGHT - 1, 0, MSG_SUCCESS.c_str());
    } else {
        mvprintw(SCREEN_HEIGHT - 1, 0, MSG_NOT_FOUND.c_str());
    }
    refresh();
    napms(1000);
}

// Экран удаления проекта
void deleteProjectScreen() {
    clear();
    mvprintw(0, 0, "=== Удаление проекта =='");
    int index = std::stoi(inputString(2, 0, "Введите номер проекта: ")) - 1;
    auto projects = readProjects();
    if (index >= 0 && index < projects.size()) {
        mvprintw(4, 0, MSG_CONFIRM_DELETE.c_str());
        refresh();
        if (getch() == 'y') {
            projects.erase(projects.begin() + index);
            writeProjects(projects);
            mvprintw(SCREEN_HEIGHT - 1, 0, MSG_SUCCESS.c_str());
        }
    } else {
        mvprintw(SCREEN_HEIGHT - 1, 0, MSG_NOT_FOUND.c_str());
    }
    refresh();
    napms(1000);
}

// Экран поиска проектов (пример по имени)
void searchProjectsScreen() {
    clear();
    mvprintw(0, 0, "=== Поиск проектов ===");
    std::string query = inputString(2, 0, "Введите наименование проекта: ");
    auto projects = readProjects();
    int y = 4;
    for (size_t i = 0; i < projects.size() && y < SCREEN_HEIGHT - 1; ++i) {
        if (projects[i].name.find(query) != std::string::npos) {
            std::string line = std::to_string(i + 1) + ". " + projects[i].name + " - " + projects[i].work_type;
            mvprintw(y++, 0, line.c_str());
        }
    }
    if (y == 4) mvprintw(SCREEN_HEIGHT - 1, 0, MSG_NOT_FOUND.c_str());
    else mvprintw(SCREEN_HEIGHT - 1, 0, "Нажмите любую клавишу для возврата...");
    refresh();
    getch();
}

// Экран сортировки проектов (пример по имени)
void sortProjectsScreen() {
    clear();
    mvprintw(0, 0, "=== Сортировка проектов ===");
    auto projects = readProjects();
    std::sort(projects.begin(), projects.end(), [](const Project& a, const Project& b) {
        return a.name < b.name;
    });
    int y = 2;
    for (size_t i = 0; i < projects.size() && y < SCREEN_HEIGHT - 1; ++i) {
        std::string line = std::to_string(i + 1) + ". " + projects[i].name + " - " + projects[i].work_type;
        mvprintw(y++, 0, line.c_str());
    }
    mvprintw(SCREEN_HEIGHT - 1, 0, "Нажмите любую клавишу для возврата...");
    refresh();
    getch();
}

// Экран управления учётными записями
void manageAccountsScreen() {
    clear();
    mvprintw(0, 0, "=== Управление учётными записями ===");
    auto accounts = readAccounts();
    int y = 2;
    for (size_t i = 0; i < accounts.size() && y < SCREEN_HEIGHT - 3; ++i) {
        std::string line = std::to_string(i + 1) + ". " + accounts[i].login + " (Role: " +
                          std::to_string(accounts[i].role) + ", Access: " + std::to_string(accounts[i].access) + ")";
        mvprintw(y++, 0, line.c_str());
    }
    mvprintw(y++, 0, "1. Подтвердить | 2. Заблокировать | 3. Удалить | 0. Назад");
    refresh();
    int choice = getch() - '0';
    if (choice == 0) return;
    int index = std::stoi(inputString(y + 1, 0, "Введите номер записи: ")) - 1;
    if (index >= 0 && index < accounts.size() && accounts[index].login != currentUser->login) {
        if (choice == 1) accounts[index].access = ACCESS_APPROVED;
        else if (choice == 2) accounts[index].access = ACCESS_BLOCKED;
        else if (choice == 3) {
            mvprintw(y + 2, 0, MSG_CONFIRM_DELETE.c_str());
            refresh();
            if (getch() == 'y') accounts.erase(accounts.begin() + index);
        }
        writeAccounts(accounts);
        mvprintw(SCREEN_HEIGHT - 1, 0, MSG_SUCCESS.c_str());
    } else {
        mvprintw(SCREEN_HEIGHT - 1, 0, MSG_NOT_FOUND.c_str());
    }
    refresh();
    napms(1000);
}



// Экран главного меню
void mainMenu() {
    while (true) {
        clear();
        mvprintw(0, 0, "=== Главное меню ===");
        mvprintw(2, 0, "1. Просмотр данных");
        mvprintw(3, 0, "2. Поиск данных");
        mvprintw(4, 0, "3. Сортировка данных");
        if (currentUser->role == ROLE_ADMIN) {
            mvprintw(5, 0, "4. Управление учётными записями");
            mvprintw(6, 0, "5. Добавить проект");
            mvprintw(7, 0, "6. Редактировать проект");
            mvprintw(8, 0, "7. Удалить проект");
            mvprintw(9, 0, "8. Регистрация пользователя");
        }
        mvprintw(currentUser->role == ROLE_ADMIN ? 10 : 5, 0, "0. Выход");
        refresh();

        int choice = getch() - '0';
        if (choice == 0) break;
        switch (choice) {
            case 1: viewProjectsScreen(); break;
            case 2: searchProjectsScreen(); break;
            case 3: sortProjectsScreen(); break;
            case 4: if (currentUser->role == ROLE_ADMIN) manageAccountsScreen(); break;
            case 5: if (currentUser->role == ROLE_ADMIN) addProjectScreen(); break;
            case 6: if (currentUser->role == ROLE_ADMIN) editProjectScreen(); break;
            case 7: if (currentUser->role == ROLE_ADMIN) deleteProjectScreen(); break;
            case 8: if (currentUser->role == ROLE_ADMIN) registerScreen(true); break;
            default: mvprintw(SCREEN_HEIGHT - 1, 0, MSG_INVALID_INPUT.c_str()); refresh(); napms(1000);
        }
    }
}

int main() {
    // Инициализация файлов
    if (!std::ifstream(ACCOUNTS_FILE)) {
        Account admin{DEFAULT_ADMIN_LOGIN, "", generateSalt(), ROLE_ADMIN, ACCESS_APPROVED};
        admin.salted_hash_password = hashPassword(DEFAULT_ADMIN_PASSWORD, admin.salt);
        std::vector<Account> accounts = {admin};
        writeAccounts(accounts);
    }
    if (!std::ifstream(PROJECTS_FILE)) {
        std::ofstream(PROJECTS_FILE).close();
    }

    // Инициализация pdcurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Главный цикл
    while (true) {
        clear();
        mvprintw(0, 0, "1. Вход | 2. Регистрация | 0. Выход");
        refresh();
        int choice = getch() - '0';
        if (choice == 0) break;
        if (choice == 1) {
            loginScreen();
            if (currentUser) {
                mainMenu();
                delete currentUser;
                currentUser = nullptr;
            }
        } else if (choice == 2) {
            registerScreen();
        } else {
            mvprintw(2, 0, MSG_INVALID_INPUT.c_str());
            refresh();
            napms(1000);
        }
    }

    endwin();
    return 0;
}