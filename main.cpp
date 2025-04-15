#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <curses.h>
#include <openssl/sha.h>
#include <ctime>
#include <map>
#include <set>


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
    int height, width;
    getmaxyx(stdscr, height, width);
    auto accounts = readAccounts();
    for (const auto& acc : accounts) {
        if (acc.login == login) {
            mvprintw(height - 1, 0, MSG_LOGIN_EXISTS.c_str());
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
    mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    refresh();
}

// Экран ввода строки с PDCurses
std::string inputString(int y, int x, const std::string& prompt, bool mask = false, int maxLength = -1) {
    mvprintw(y, x, prompt.c_str());
    std::string input;
    int ch;
    while ((ch = getch()) != '\n') {
        if (ch == '/') {
            return "/";
        }
        if (ch == KEY_BACKSPACE || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
                mvprintw(y, x + prompt.length() + input.length(), " ");
                move(y, x + prompt.length() + input.length());
            }
        } else if (ch >= 32 && ch <= 126 && (maxLength == -1 || input.length() < maxLength)) {
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

int inputInt(int y, int x, const std::string& prompt, int maxLength = -1) {
    while (true) {
        std::string input = inputString(y, x, prompt, false, maxLength);
        try {
            return std::stoi(input);
        } catch (const std::exception& e) {
            mvprintw(y, x, (prompt + std::string(20, ' ')).c_str()); // Очищаем строку приглашения
            mvprintw(y + 1, x, "Ошибка: введите целое число!     ");
            refresh();
            napms(1000);
            mvprintw(y + 1, x, std::string(40, ' ').c_str()); // Очищаем строку ошибки
            move(y, x + prompt.length());
            refresh();
        }
    }
}

double inputDouble(int y, int x, const std::string& prompt, int maxLength = -1) {
    while (true) {
        std::string input = inputString(y, x, prompt, false, maxLength);
        try {
            return std::stod(input);
        } catch (const std::exception& e) {
            mvprintw(y, x, (prompt + std::string(20, ' ')).c_str()); // Очищаем строку приглашения
            mvprintw(y + 1, x, "Ошибка: введите число!     ");
            refresh();
            napms(1000);
            mvprintw(y + 1, x, std::string(40, ' ').c_str()); // Очищаем строку ошибки
            move(y, x + prompt.length());
            refresh();
        }
    }
}

// Экран авторизации
void loginScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Авторизация ===");
    
    std::string login = inputString(2, 0, "Логин: ", false, 20); // Ограничение 20 символов
    if (login == "/") {
        mvprintw(height - 1, 0, "\"Вход отменен!\"");
        refresh();
        napms(1000);
        return;
    }
    
    std::string password = inputString(4, 0, "Пароль: ", true, 20); // Ограничение 20 символов
    auto accounts = readAccounts();
    bool found = false;
    for (auto& acc : accounts) {
        if (acc.login == login) {
            found = true;
            std::string hashed = hashPassword(password, acc.salt);
            if (hashed == acc.salted_hash_password) {
                if (acc.access == ACCESS_APPROVED) {
                    currentUser = new Account(acc);
                    mvprintw(height - 2, 0, "Вход выполнен успешно!");
                } else if (acc.access == ACCESS_PENDING) {
                    mvprintw(height - 2, 0, "Ваш запрос на регистрацию ожидает подтверждения.");
                } else if (acc.access == ACCESS_BLOCKED) {
                    mvprintw(height - 2, 0, "Ваш аккаунт заблокирован.");
                }
            } else {
                mvprintw(height - 2, 0, "Неверный пароль!");
            }
            break;
        }
    }
    if (!found) {
        mvprintw(height - 2, 0, "Логин не найден!");
    }
    mvprintw(height - 1, 0, "Нажмите любую клавишу...");
    refresh();
    getch();
}



// Экран регистрации
void registerScreen(bool byAdmin = false) {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Регистрация ===");
    
    std::string login = inputString(2, 0, "Логин: ", false, 20);
    if (login == "/") {
        mvprintw(height - 1, 0, "\"Регистрация отменена!\"");
        refresh();
        napms(1000);
        return;
    }
    
    std::string password = inputString(4, 0, "Пароль: ", true, 20);
    int role = byAdmin ? ROLE_USER : ROLE_USER;
    int access = byAdmin ? ACCESS_APPROVED : ACCESS_PENDING;
    registerUser(login, password, role, access);
    
    // Очищаем нижние строки перед выводом
    mvprintw(height - 2, 0, std::string(width, ' ').c_str()); // Очистка строки
    mvprintw(height - 1, 0, std::string(width, ' ').c_str()); // Очистка строки
    if (!byAdmin) {
        mvprintw(height - 2, 0, "Ваш запрос отправлен администратору на подтверждение.");
    }
    mvprintw(height - 1, 0, "Нажмите любую клавишу...");
    refresh();
    getch();
}

// Экран просмотра проектов
void viewProjectsScreen() {
    auto projects = readProjects();
    int height, width;
    getmaxyx(stdscr, height, width);

    if (projects.empty()) {
        clear();
        mvprintw(0, 0, "=== Просмотр проектов ===");
        mvprintw(2, 0, "\"Проекты отсутствуют!\"");
        mvprintw(height - 1, 0, "\"Нажмите любую клавишу для возврата...\"");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 5;
    int totalPages = (projects.size() + dataRows - 1) / dataRows;
    int currentPage = 0;
    int sortColumn = -1;
    bool ascending = true;

    while (true) {
        clear();
        mvprintw(0, 0, "=== Просмотр проектов ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(projects.size()));

        int colNumWidth = 4;
        int colNameWidth = 15;
        int colWorkWidth = 15;
        int colEmployeeWidth = 33;
        int colHoursWidth = 6;
        int colCostWidth = 10;

        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5;
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "1 Название" + std::string(colNameWidth - 10, ' ') + "|" +
                            "2 Работа" + std::string(colWorkWidth - 8, ' ') + "|" +
                            "3 Сотрудник" + std::string(colEmployeeWidth - 11, ' ') + "|" +
                            "4 Часы" + std::string(colHoursWidth - 6, ' ') + "|" +
                            "5 Стоимость";
        mvprintw(2, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string name = projects[i].name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(projects[i].name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
            std::string work = projects[i].work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(projects[i].work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
            std::string employee = projects[i].employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(projects[i].employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
            std::string hours = std::to_string(projects[i].hours) + std::string(colHoursWidth - std::to_string(projects[i].hours).length() - 1, ' ') + "|";
            std::string cost = std::to_string(projects[i].hour_cost).substr(0, colCostWidth - 1);

            std::string line = num + name + work + employee + hours + cost;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string navigation = "\"Стрелки влево/вправо - страницы | 1-5 - сортировка\" | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch >= '1' && ch <= '5') {
            int newSortColumn = ch - '0';
            if (newSortColumn == sortColumn) {
                ascending = !ascending;
            } else {
                sortColumn = newSortColumn;
                ascending = true;
            }
            std::sort(projects.begin(), projects.end(), [&](const Project& a, const Project& b) {
                switch (sortColumn) {
                    case 1: return ascending ? a.name < b.name : a.name > b.name;
                    case 2: return ascending ? a.work_type < b.work_type : a.work_type > b.work_type;
                    case 3: return ascending ? a.employee < b.employee : a.employee > b.employee;
                    case 4: return ascending ? a.hours < b.hours : a.hours > b.hours;
                    case 5: return ascending ? a.hour_cost < b.hour_cost : a.hour_cost > b.hour_cost;
                    default: return false;
                }
            });
            totalPages = (projects.size() + dataRows - 1) / dataRows;
            currentPage = std::min(currentPage, totalPages - 1);
            if (currentPage < 0) currentPage = 0;
        }
    }
}


// Экран добавления проекта
void addProjectScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Добавление проекта ===");
    Project proj;

    const int MAX_NAME_LENGTH = 14;
    const int MAX_WORK_LENGTH = 14;
    const int MAX_EMPLOYEE_LENGTH = 32;
    const int MAX_HOURS_LENGTH = 5;
    const int MAX_COST_LENGTH = 10;

    proj.name = inputString(2, 0, "Наименование проекта (макс. 14): ", false, MAX_NAME_LENGTH);
    if (proj.name == "/") {
        mvprintw(height - 1, 0, "\"Добавление отменено!\"");
        refresh();
        napms(1000);
        return;
    }

    proj.work_type = inputString(4, 0, "Вид работ (макс. 14): ", false, MAX_WORK_LENGTH);
    if (proj.work_type == "/") {
        mvprintw(height - 1, 0, "\"Добавление отменено!\"");
        refresh();
        napms(1000);
        return;
    }

    proj.employee = inputString(6, 0, "Ф.И.О. сотрудника (макс. 32): ", false, MAX_EMPLOYEE_LENGTH);
    if (proj.employee == "/") {
        mvprintw(height - 1, 0, "\"Добавление отменено!\"");
        refresh();
        napms(1000);
        return;
    }

    proj.hours = inputInt(8, 0, "Часы (макс. 5): ", MAX_HOURS_LENGTH);
    proj.hour_cost = inputDouble(10, 0, "Стоимость часа (макс. 10): ", MAX_COST_LENGTH);

    auto projects = readProjects();
    projects.push_back(proj);
    writeProjects(projects);
    mvprintw(height - 1, 0, "\"Операция выполнена успешно!\"");
    refresh();
    napms(1000);
}

// Экран редактирования проекта
void editProjectScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Редактирование проекта ===");
    
    // Используем inputString вместо inputInt для возможности ввода "/"
    std::string indexInput = inputString(2, 0, "Введите номер проекта: ");
    if (indexInput == "/") {
        mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
        refresh();
        napms(1000);
        return;
    }
    
    int index;
    try {
        index = std::stoi(indexInput) - 1;
    } catch (const std::exception& e) {
        mvprintw(height - 1, 0, "\"Ошибка: введите корректный номер!\"");
        refresh();
        napms(1000);
        return;
    }
    
    auto projects = readProjects();
    if (index >= 0 && index < projects.size()) {
        Project& proj = projects[index];
        Project originalProj = proj;

        mvprintw(2, 0, std::string(width, ' ').c_str());
        mvprintw(2, 0, "Текущие данные проекта:");
        refresh();

        int colNumWidth = 4;
        int colNameWidth = 15;
        int colWorkWidth = 15;
        int colEmployeeWidth = 33;
        int colHoursWidth = 6;
        int colCostWidth = 10;

        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5;
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "Название" + std::string(colNameWidth - 8, ' ') + "|" +
                            "Работа" + std::string(colWorkWidth - 6, ' ') + "|" +
                            "Сотрудник" + std::string(colEmployeeWidth - 9, ' ') + "|" +
                            "Часы" + std::string(colHoursWidth - 4, ' ') + "|" +
                            "Стоимость";
        mvprintw(4, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(5, 0, separator.c_str());

        std::string num = std::to_string(index + 1) + std::string(colNumWidth - std::to_string(index + 1).length() - 1, ' ') + "|";
        std::string name = proj.name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(proj.name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
        std::string work = proj.work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(proj.work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
        std::string employee = proj.employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(proj.employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
        std::string hours = std::to_string(proj.hours) + std::string(colHoursWidth - std::to_string(proj.hours).length() - 1, ' ') + "|";
        std::string cost = std::to_string(proj.hour_cost).substr(0, colCostWidth - 1);

        std::string line = num + name + work + employee + hours + cost;
        mvprintw(6, 0, line.substr(0, width).c_str());

        mvprintw(8, 0, "Введите новые данные (Enter для сохранения текущего значения):");

        const int MAX_NAME_LENGTH = 14;
        const int MAX_WORK_LENGTH = 14;
        const int MAX_EMPLOYEE_LENGTH = 32;
        const int MAX_HOURS_LENGTH = 5;
        const int MAX_COST_LENGTH = 10;

        std::string newName = inputString(9, 0, "Новое наименование (макс. 14): ", false, MAX_NAME_LENGTH);
        if (newName == "/") {
            mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
            refresh();
            napms(1000);
            return;
        }
        proj.name = newName.empty() ? proj.name : newName;

        std::string newWork = inputString(10, 0, "Новый вид работ (макс. 14): ", false, MAX_WORK_LENGTH);
        if (newWork == "/") {
            mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
            refresh();
            napms(1000);
            return;
        }
        proj.work_type = newWork.empty() ? proj.work_type : newWork;

        std::string newEmployee = inputString(11, 0, "Новое Ф.И.О. сотрудника (макс. 32): ", false, MAX_EMPLOYEE_LENGTH);
        if (newEmployee == "/") {
            mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
            refresh();
            napms(1000);
            return;
        }
        proj.employee = newEmployee.empty() ? proj.employee : newEmployee;

        std::string hoursInput = inputString(12, 0, "Новые часы (макс. 5): ", false, MAX_HOURS_LENGTH);
        if (hoursInput == "/") {
            mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
            refresh();
            napms(1000);
            return;
        }
        if (!hoursInput.empty()) {
            try {
                proj.hours = std::stoi(hoursInput);
            } catch (const std::exception& e) {
                mvprintw(height - 1, 0, "\"Ошибка: часы оставлены без изменений!\"");
                refresh();
                napms(1000);
            }
        }

        std::string costInput = inputString(13, 0, "Новая стоимость часа (макс. 10): ", false, MAX_COST_LENGTH);
        if (costInput == "/") {
            mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
            refresh();
            napms(1000);
            return;
        }
        if (!costInput.empty()) {
            try {
                proj.hour_cost = std::stod(costInput);
            } catch (const std::exception& e) {
                mvprintw(height - 1, 0, "\"Ошибка: стоимость оставлена без изменений!\"");
                refresh();
                napms(1000);
            }
        }

        writeProjects(projects);
        mvprintw(height - 1, 0, "\"Операция выполнена успешно!\"");
    } else {
        mvprintw(height - 1, 0, "\"Данные не найдены!\"");
    }
    refresh();
    napms(1000);
}








// Экран удаления проекта
void deleteProjectScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Удаление проекта ===");
    std::string indexInput = inputString(2, 0, "Введите номер проекта: ");
    if (indexInput == "/") {
        mvprintw(height - 1, 0, "\"Удаление отменено!\"");
        refresh();
        napms(1000);
        return;
    }
    int index = std::stoi(indexInput) - 1;
    auto projects = readProjects();
    if (index >= 0 && index < projects.size()) {
        mvprintw(4, 0, "\"Вы действительно хотите удалить запись? (y/n): \"");
        refresh();
        if (getch() == 'y') {
            projects.erase(projects.begin() + index);
            writeProjects(projects);
            mvprintw(height - 1, 0, "\"Операция выполнена успешно!\"");
        } else {
            mvprintw(height - 1, 0, "\"Удаление отменено!\"");
        }
    } else {
        mvprintw(height - 1, 0, "\"Данные не найдены!\"");
    }
    refresh();
    napms(1000);
}


// Экран поиска проектов
void searchProjectsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Поиск проектов ===");
    mvprintw(2, 0, "Выберите критерий поиска:");
    mvprintw(3, 0, "1. По наименованию проекта");
    mvprintw(4, 0, "2. По виду работ");
    mvprintw(5, 0, "3. По Ф.И.О. сотрудника");
    refresh();

    int ch = getch();
    if (ch == '/') {
        return;
    }
    int choice = ch - '0';
    if (choice < 1 || choice > 3) {
        mvprintw(height - 1, 0, "\"Неверный ввод!\"");
        refresh();
        napms(1000);
        return;
    }

    std::string prompt;
    switch (choice) {
        case 1: prompt = "Введите наименование проекта: "; break;
        case 2: prompt = "Введите вид работ: "; break;
        case 3: prompt = "Введите Ф.И.О. сотрудника: "; break;
    }

    clear();
    mvprintw(0, 0, "=== Поиск проектов ===");
    std::string query = inputString(2, 0, prompt);
    if (query == "/") {
        mvprintw(height - 1, 0, "\"Поиск отменен!\"");
        refresh();
        napms(1000);
        return;
    }
    auto projects = readProjects();

    std::vector<Project> filteredProjects;
    for (const auto& proj : projects) {
        if (choice == 1 && proj.name.find(query) != std::string::npos) {
            filteredProjects.push_back(proj);
        } else if (choice == 2 && proj.work_type.find(query) != std::string::npos) {
            filteredProjects.push_back(proj);
        } else if (choice == 3 && proj.employee.find(query) != std::string::npos) {
            filteredProjects.push_back(proj);
        }
    }

    if (filteredProjects.empty()) {
        mvprintw(4, 0, "\"Данные не найдены!\"");
        mvprintw(height - 1, 0, "\"Нажмите любую клавишу для возврата...\"");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 5;
    int totalPages = (filteredProjects.size() + dataRows - 1) / dataRows;
    int currentPage = 0;

    while (true) {
        clear();
        mvprintw(0, 0, "=== Поиск проектов ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(filteredProjects.size()));

        int colNumWidth = 4;
        int colNameWidth = 15;
        int colWorkWidth = 15;
        int colEmployeeWidth = 33;
        int colHoursWidth = 6;
        int colCostWidth = 10;

        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5;
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            std::string("Название").substr(0, colNameWidth - 1) + std::string(colNameWidth - 8, ' ') + "|" +
                            std::string("Работа").substr(0, colWorkWidth - 1) + std::string(colWorkWidth - 7, ' ') + "|" +
                            std::string("Сотрудник").substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - 10, ' ') + "|" +
                            "Часы |" +
                            "Стоимость";
        mvprintw(2, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string name = filteredProjects[i].name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(filteredProjects[i].name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
            std::string work = filteredProjects[i].work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(filteredProjects[i].work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
            std::string employee = filteredProjects[i].employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(filteredProjects[i].employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
            std::string hours = std::to_string(filteredProjects[i].hours) + std::string(colHoursWidth - std::to_string(filteredProjects[i].hours).length() - 1, ' ') + "|";
            std::string cost = std::to_string(filteredProjects[i].hour_cost).substr(0, colCostWidth - 1);

            std::string line = num + name + work + employee + hours + cost;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string navigation = "Стрелки влево/вправо - переключение страниц | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        }
    }
}



void editAccountScreen(int index) {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Редактирование учетной записи ===");
   
    auto accounts = readAccounts();
    if (index >= 0 && index < accounts.size() && accounts[index].login != currentUser->login) {
        Account& acc = accounts[index];

        mvprintw(4, 0, "Текущий логин: %s", acc.login.c_str());
        mvprintw(5, 0, "Админ: %s", acc.role ? "1 - да" : "0 - нет");
        mvprintw(6, 0, "Заблокирован: %s", 
                 acc.access == ACCESS_BLOCKED ? "1 - да" : "0 - нет");

        const int MAX_LOGIN_LENGTH = 20;
        std::string newLogin = inputString(8, 0, "Новый логин (Enter - сохранить текущий): ", false, MAX_LOGIN_LENGTH);
        if (newLogin == "/") return;
        if (!newLogin.empty()) {
            bool loginExists = false;
            for (const auto& existingAcc : accounts) {
                if (existingAcc.login == newLogin && existingAcc.login != acc.login) {
                    loginExists = true;
                    break;
                }
            }
            if (loginExists) {
                mvprintw(height - 1, 0, "Логин уже существует!");
                refresh();
                napms(1000);
                return;
            }
            acc.login = newLogin;
        }

        std::string newRoleStr = inputString(10, 0, "Админ (0 - нет, 1 - да, Enter - сохранить текущий): ");
        if (newRoleStr == "/") return;
        if (!newRoleStr.empty()) {
            int newRole = std::stoi(newRoleStr);
            if (newRole == 0 || newRole == 1) {
                acc.role = newRole;
            } else {
                mvprintw(height - 1, 0, "Неверное значение роли!");
                refresh();
                napms(1000);
                return;
            }
        }

        std::string newBlockedStr = inputString(12, 0, "Заблокирован (0 - нет, 1 - да, Enter - сохранить текущий): ");
        if (newBlockedStr == "/") return;
        if (!newBlockedStr.empty()) {
            int newBlocked = std::stoi(newBlockedStr);
            if (newBlocked == 0 || newBlocked == 1) {
                // Преобразуем интерфейсные 0 и 1 во внутренние статусы
                if (newBlocked == 0) {
                    acc.access = (acc.access == ACCESS_PENDING) ? ACCESS_PENDING : ACCESS_APPROVED;
                } else {
                    acc.access = ACCESS_BLOCKED;
                }
            } else {
                mvprintw(height - 1, 0, "Неверное значение! Используйте 0 или 1.");
                refresh();
                napms(1000);
                return;
            }
        }

        writeAccounts(accounts);
        mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
        refresh();
        napms(1000);
    }
}



void confirmDeleteAccountScreen(int index, std::vector<Account>& accounts) {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Подтверждение удаления ===");
    mvprintw(2, 0, "Информация об учетной записи:");

    int colNumWidth = 4;
    int colLoginWidth = 20;
    int colRoleWidth = 12; // Фиксируем ширину для "Админ"
    int colAccessWidth = 14;
    int totalWidth = colNumWidth + colLoginWidth + colRoleWidth + colAccessWidth + 3;
    if (totalWidth > width) {
        colLoginWidth = width - (colNumWidth + colRoleWidth + colAccessWidth + 3);
    }

    std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                        "Логин" + std::string(colLoginWidth - 5, ' ') + "|" +
                        "Админ" + std::string(colRoleWidth - 5, ' ') + "|" +
                        "Заблокирован";
    mvprintw(4, 0, header.substr(0, width).c_str());

    std::string separator(totalWidth, '-');
    mvprintw(5, 0, separator.c_str());

    std::string num = std::to_string(index + 1) + std::string(colNumWidth - std::to_string(index + 1).length() - 1, ' ') + "|";
    std::string login = accounts[index].login.substr(0, colLoginWidth - 1) + 
                        std::string(colLoginWidth - std::min(accounts[index].login.length(), static_cast<size_t>(colLoginWidth - 1)), ' ') + "|";
    std::string role = (accounts[index].role ? "1 - да" : "0 - нет") + 
                       std::string(colRoleWidth - 7, ' ') + "|"; // Фиксируем длину 7 символов
    std::string blocked = (accounts[index].access == ACCESS_BLOCKED ? "1 - да" : "0 - нет") + 
                          std::string(colAccessWidth - 7, ' ');

    std::string line = num + login + role + blocked;
    mvprintw(6, 0, line.substr(0, width).c_str());

    mvprintw(height - 2, 0, "Вы действительно хотите удалить запись? (y/n): ");
    refresh();

    int ch = getch();
    if (ch == 'y') {
        accounts.erase(accounts.begin() + index);
        writeAccounts(accounts);
        mvprintw(height - 1, 0, "\"Операция выполнена успешно!\"");
    } else {
        mvprintw(height - 1, 0, "\"Удаление отменено!\"");
    }
    refresh();
    napms(1000);
}






void manageAccountsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    auto accounts = readAccounts();

    if (accounts.empty()) {
        clear();
        mvprintw(0, 0, "=== Управление учётными записями ===");
        mvprintw(2, 0, "\"Учётные записи отсутствуют!\"");
        mvprintw(height - 1, 0, "\"Нажмите любую клавишу для возврата...\"");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 6;
    int totalPages = (accounts.size() + dataRows - 1) / dataRows;
    int currentPage = 0;

    while (true) {
        clear();
        mvprintw(0, 0, "=== Управление учётными записями ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(accounts.size()));

        int colNumWidth = 4;    // Ширина столбца "№"
        int colLoginWidth = 20; // Ширина столбца "Логин"
        int colRoleWidth = 12;  // Ширина столбца "Админ" (должна вмещать "1 - да" или "0 - нет")
        int colAccessWidth = 14;// Ширина столбца "Заблокирован"
        int totalWidth = colNumWidth + colLoginWidth + colRoleWidth + colAccessWidth + 3;
        if (totalWidth > width) {
            colLoginWidth = width - (colNumWidth + colRoleWidth + colAccessWidth + 3);
        }

        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "Логин" + std::string(colLoginWidth - 5, ' ') + "|" +
                            "Админ" + std::string(colRoleWidth - 5, ' ') + "|" +
                            "Заблокирован";
        mvprintw(2, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 2; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string login = accounts[i].login.substr(0, colLoginWidth - 1) + 
                              std::string(colLoginWidth - std::min(accounts[i].login.length(), static_cast<size_t>(colLoginWidth - 1)), ' ') + "|";
            // Выравниваем "Админ" (7 символов для "0 - нет" или "1 - да" + пробелы до colRoleWidth)
            std::string role = (accounts[i].role ? "1 - да " : "0 - нет") + 
                              std::string(colRoleWidth - 7, ' ') + "|";
            // Выравниваем "Заблокирован" (7 символов для "0 - нет" или "1 - да" + пробелы до colAccessWidth)
            std::string blocked = (accounts[i].access == ACCESS_BLOCKED ? "1 - да " : "0 - нет") + 
                                  std::string(colAccessWidth - 7, ' ');

            std::string line = num + login + role + blocked;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string options = "1. Заблокировать/Разблокировать | 2. Дать/Снять админку | 3. Удалить | 4. Редактировать";
        mvprintw(height - 2, 0, options.substr(0, width).c_str());
        
        std::string navigation = "Стрелки влево/вправо - страницы | Страница " +
                                std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch >= '1' && ch <= '4') {
            std::string indexInput = inputString(height - 3, 0, "Введите номер записи: ");
            if (indexInput == "/") {
                mvprintw(height - 1, 0, "\"Операция отменена!\"");
                refresh();
                napms(1000);
                continue;
            }
            if (indexInput.empty()) {
                mvprintw(height - 1, 0, "\"Ошибка: введите номер записи!\"");
                refresh();
                napms(1000);
                continue;
            }
            int index;
            try {
                index = std::stoi(indexInput) - 1;
            } catch (const std::exception& e) {
                mvprintw(height - 1, 0, "\"Ошибка: введите корректный номер!\"");
                refresh();
                napms(1000);
                continue;
            }
            if (index >= 0 && index < accounts.size()) {
                if (accounts[index].login == currentUser->login) {
                    clear();
                    mvprintw(height / 2, 0, "\"Нельзя изменить или удалить текущего пользователя!\"");
                    mvprintw(height - 1, 0, "\"Нажмите любую клавишу для продолжения...\"");
                    refresh();
                    getch();
                    continue;
                }
                switch (ch) {
                    case '1':
                        if (accounts[index].access == ACCESS_BLOCKED) {
                            accounts[index].access = (accounts[index].access == ACCESS_PENDING) ? ACCESS_PENDING : ACCESS_APPROVED;
                            mvprintw(height - 1, 0, "\"Пользователь разблокирован!\"");
                        } else {
                            accounts[index].access = ACCESS_BLOCKED;
                            mvprintw(height - 1, 0, "\"Пользователь заблокирован!\"");
                        }
                        writeAccounts(accounts);
                        break;
                    case '2':
                        accounts[index].role = !accounts[index].role;
                        writeAccounts(accounts);
                        mvprintw(height - 1, 0, "\"Операция выполнена успешно!\"");
                        break;
                    case '3':
                        confirmDeleteAccountScreen(index, accounts);
                        accounts = readAccounts();
                        totalPages = (accounts.size() + dataRows - 1) / dataRows;
                        currentPage = std::min(currentPage, totalPages - 1);
                        if (currentPage < 0) currentPage = 0;
                        break;
                    case '4':
                        editAccountScreen(index);
                        accounts = readAccounts();
                        totalPages = (accounts.size() + dataRows - 1) / dataRows;
                        currentPage = std::min(currentPage, totalPages - 1);
                        if (currentPage < 0) currentPage = 0;
                        break;
                }
                refresh();
                napms(1000);
            } else {
                mvprintw(height - 1, 0, "\"Данные не найдены!\"");
                refresh();
                napms(1000);
            }
        }
    }
}


void pendingRequestsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Запросы на подтверждение ===");
    auto accounts = readAccounts();

    std::vector<Account> pendingAccounts;
    for (const auto& acc : accounts) {
        if (acc.access == ACCESS_PENDING) {
            pendingAccounts.push_back(acc);
        }
    }

    if (pendingAccounts.empty()) {
        mvprintw(2, 0, "\"Нет запросов на подтверждение!\"");
        mvprintw(height - 1, 0, "\"Нажмите любую клавишу для возврата...\"");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 5;
    int totalPages = (pendingAccounts.size() + dataRows - 1) / dataRows;
    int currentPage = 0;

    while (true) {
        clear();
        mvprintw(0, 0, "=== Запросы на подтверждение ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(pendingAccounts.size()));

        int colNumWidth = 4;
        int colLoginWidth = 20;
        int totalWidth = colNumWidth + colLoginWidth + 1;
        if (totalWidth > width) {
            colLoginWidth = width - (colNumWidth + 1);
        }

        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "Логин" + std::string(colLoginWidth - 5, ' ');
        mvprintw(2, 0, header.substr(0, width).c_str());
        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string login = pendingAccounts[i].login.substr(0, colLoginWidth - 1) + 
                                std::string(colLoginWidth - std::min(pendingAccounts[i].login.length(), static_cast<size_t>(colLoginWidth - 1)), ' ');
            std::string line = num + login;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string navigation = "\"Стрелки влево/вправо - страницы | 1. Подтвердить | 2. Заблокировать\" | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch == '1' || ch == '2') {
            std::string indexInput = inputString(height - 2, 0, "Введите номер записи: ");
            if (indexInput == "/") {
                mvprintw(height - 1, 0, "\"Операция отменена!\"");
                refresh();
                napms(1000);
                continue;
            }
            int index = std::stoi(indexInput) - 1;
            if (index >= 0 && index < pendingAccounts.size()) {
                int realIndex = -1;
                for (size_t i = 0; i < accounts.size(); ++i) {
                    if (accounts[i].login == pendingAccounts[index].login) {
                        realIndex = i;
                        break;
                    }
                }
                if (realIndex != -1) {
                    if (ch == '1') {
                        accounts[realIndex].access = ACCESS_APPROVED;
                        mvprintw(height - 1, 0, "\"Пользователь подтверждён!\"");
                    } else if (ch == '2') {
                        accounts[realIndex].access = ACCESS_BLOCKED;
                        mvprintw(height - 1, 0, "\"Пользователь заблокирован!\"");
                    }
                    writeAccounts(accounts);
                    refresh();
                    napms(1000);
                    pendingAccounts.clear();
                    for (const auto& acc : accounts) {
                        if (acc.access == ACCESS_PENDING) {
                            pendingAccounts.push_back(acc);
                        }
                    }
                    totalPages = (pendingAccounts.size() + dataRows - 1) / dataRows;
                    currentPage = std::min(currentPage, totalPages - 1);
                    if (currentPage < 0) currentPage = 0;
                }
            } else {
                mvprintw(height - 1, 0, "\"Данные не найдены!\"");
                refresh();
                napms(1000);
            }
        }
    }
}

void projectSummaryScreen() {
    int height, width;
    getmaxyx(stdscr, height, width); // Получаем размеры экрана
    auto projects = readProjects();   // Читаем все проекты

    if (projects.empty()) {
        clear();
        mvprintw(0, 0, "=== Сводка по проектам ===");
        mvprintw(2, 0, "Проекты отсутствуют!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу для возврата...");
        refresh();
        getch();
        return;
    }

    // Группируем проекты по названию
    std::map<std::string, std::vector<Project>> projectMap;
    for (const auto& proj : projects) {
        projectMap[proj.name].push_back(proj);
    }

    // Создаем вектор уникальных названий проектов для пагинации
    std::vector<std::string> projectNames;
    for (const auto& pair : projectMap) {
        projectNames.push_back(pair.first);
    }

    int totalPages = projectNames.size(); // Количество страниц = количество уникальных проектов
    int currentPage = 0;                  // Текущая страница

    while (true) {
        clear();
        mvprintw(0, 0, "=== Сводка по проектам ===");

        // Текущий проект
        const std::string& projectName = projectNames[currentPage];
        const std::vector<Project>& projList = projectMap[projectName];

        // Вычисляем итоговую стоимость проекта
        double totalCost = 0.0;
        for (const auto& p : projList) {
            totalCost += p.hours * p.hour_cost;
        }

        // Выводим информацию о проекте
        int y = 2;
        mvprintw(y++, 0, "Проект: %s", projectName.c_str());
        mvprintw(y++, 0, "Итоговая стоимость: %.2f", totalCost);

        // Группируем записи по виду работ
        std::map<std::string, std::vector<Project>> workTypeMap;
        for (const auto& p : projList) {
            workTypeMap[p.work_type].push_back(p);
        }

        // Выводим информацию по каждому виду работ
        for (const auto& workPair : workTypeMap) {
            const std::string& workType = workPair.first;
            const std::vector<Project>& workList = workPair.second;

            // Подсчитываем уникальных сотрудников и стоимость вида работ
            std::set<std::string> employees;
            double workCost = 0.0;
            for (const auto& w : workList) {
                employees.insert(w.employee); // Уникальные сотрудники
                workCost += w.hours * w.hour_cost; // Стоимость этапа
            }

            if (y < height - 2) { // Проверяем, помещается ли на экран
                mvprintw(y++, 0, "  Вид работ: %s", workType.c_str());
                mvprintw(y++, 0, "    Количество специалистов: %d", employees.size());
                mvprintw(y++, 0, "    Стоимость: %.2f", workCost);
            }
        }

        // Навигация
        std::string navigation = "Стрелки влево/вправо - переключение | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        // Обработка ввода
        int ch = getch();
        if (ch == '/') break;                    // Выход
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;                       // Следующая страница
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;                       // Предыдущая страница
        }
    }
}

// Экран главного меню
void mainMenu() {
    while (true) {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "=== Главное меню ===");
        int y = 2;
        mvprintw(y++, 0, "1. Просмотр данных");
        mvprintw(y++, 0, "2. Поиск данных");
        mvprintw(y++, 0, "3. Сводка по проектам");
        if (currentUser->role == ROLE_ADMIN) {
            mvprintw(y++, 0, "4. Управление учётными записями");
            mvprintw(y++, 0, "5. Добавить проект");
            mvprintw(y++, 0, "6. Редактировать проект");
            mvprintw(y++, 0, "7. Удалить проект");
            mvprintw(y++, 0, "8. Регистрация пользователя");
            mvprintw(y++, 0, "9. Запросы на подтверждение");
        }
        mvprintw(y++, 0, "/. Выход");
        refresh();

        int ch = getch();
        if (ch == '/') break;
        int choice = ch - '0';
        switch (choice) {
            case 1: viewProjectsScreen(); break;
            case 2: searchProjectsScreen(); break;
            case 3: projectSummaryScreen(); break;
            case 4: if (currentUser->role == ROLE_ADMIN) manageAccountsScreen(); break;
            case 5: if (currentUser->role == ROLE_ADMIN) addProjectScreen(); break;
            case 6: if (currentUser->role == ROLE_ADMIN) editProjectScreen(); break;
            case 7: if (currentUser->role == ROLE_ADMIN) deleteProjectScreen(); break;
            case 8: if (currentUser->role == ROLE_ADMIN) registerScreen(true); break;
            case 9: if (currentUser->role == ROLE_ADMIN) pendingRequestsScreen(); break;
            default: mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str()); refresh(); napms(1000);
        }
    }
}

int main() {
    if (!std::ifstream(ACCOUNTS_FILE)) {
        Account admin{DEFAULT_ADMIN_LOGIN, "", generateSalt(), ROLE_ADMIN, ACCESS_APPROVED};
        admin.salted_hash_password = hashPassword(DEFAULT_ADMIN_PASSWORD, admin.salt);
        std::vector<Account> accounts = {admin};
        writeAccounts(accounts);
    }
    if (!std::ifstream(PROJECTS_FILE)) {
        std::ofstream(PROJECTS_FILE).close();
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    srand(time(nullptr));

    while (true) {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "1. Вход | 2. Регистрация | / Выход");
        refresh();
        int ch = getch();
        if (ch == '/') break;
        int choice = ch - '0';
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
            mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str());
            refresh();
            napms(1000);
        }
    }
    endwin();
    return 0;
}