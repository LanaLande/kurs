#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <curses.h>
#include <openssl/sha.h>
#include <ctime>

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
        if (ch == '/') { // Проверка на символ "/"
            return "/"; // Возвращаем "/" для отмены
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
    std::string login = inputString(2, 0, "Логин: ");
    std::string password = inputString(4, 0, "Пароль: ", true);
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
    std::string login = inputString(2, 0, "Логин: ");
    std::string password = inputString(4, 0, "Пароль: ", true);
    int role = byAdmin ? ROLE_USER : ROLE_USER;
   int access = byAdmin ? ACCESS_APPROVED : ACCESS_PENDING; // Новый пользователь получает ACCESS_PENDING
    registerUser(login, password, role, access);
    
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
        mvprintw(2, 0, "Проекты отсутствуют!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу для возврата...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 4;
    int totalPages = (projects.size() + dataRows - 1) / dataRows;
    int currentPage = 0;
    int sortColumn = -1; // -1 означает отсутствие сортировки по умолчанию
    bool ascending = true; // Направление сортировки: true - по возрастанию, false - по убыванию

    while (true) {
        clear();
        mvprintw(0, 0, "=== Просмотр проектов ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(projects.size()));

        // Фиксированные ширины столбцов
        int colNumWidth = 4;      // "№"
        int colNameWidth = 15;    // "Название"
        int colWorkWidth = 15;    // "Работа"
        int colEmployeeWidth = 33;// "Сотрудник"
        int colHoursWidth = 6;    // "Часы"
        int colCostWidth = 10;    // "Стоимость"

        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5; // 5 разделителей "|"
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        // Заголовок таблицы с номерами столбцов
        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "1 Название" + std::string(colNameWidth - 10, ' ') + "|" +
                            "2 Работа" + std::string(colWorkWidth - 8, ' ') + "|" +
                            "3 Сотрудник" + std::string(colEmployeeWidth - 11, ' ') + "|" +
                            "4 Часы" + std::string(colHoursWidth - 6, ' ') + "|" +
                            "5 Стоимость";
        mvprintw(2, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        // Вывод данных
        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 2; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string name = projects[i].name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(projects[i].name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
            std::string work = projects[i].work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(projects[i].work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
            std::string employee = projects[i].employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(projects[i].employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
            std::string hours = std::to_string(projects[i].hours) + std::string(colHoursWidth - std::to_string(projects[i].hours).length() - 1, ' ') + "|";
            std::string cost = std::to_string(projects[i].hour_cost).substr(0, colCostWidth - 1);

            std::string line = num + name + work + employee + hours + cost;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string navigation = "Стрелки влево/вправо - страницы | 1-5 - сортировка | q - выход | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++; // Листаем только если не на последней странице
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--; // Листаем только если не на первой странице
        } else if (ch >= '1' && ch <= '5') {
            int newSortColumn = ch - '0';
            if (newSortColumn == sortColumn) {
                // Если тот же столбец, меняем направление сортировки
                ascending = !ascending;
            } else {
                // Новый столбец, начинаем с сортировки по возрастанию
                sortColumn = newSortColumn;
                ascending = true;
            }
            // Применяем сортировку один раз при выборе столбца или изменении направления
            std::sort(projects.begin(), projects.end(), [&](const Project& a, const Project& b) {
                switch (sortColumn) {
                    case 1: // Название
                        return ascending ? a.name < b.name : a.name > b.name;
                    case 2: // Работа
                        return ascending ? a.work_type < b.work_type : a.work_type > b.work_type;
                    case 3: // Сотрудник
                        return ascending ? a.employee < b.employee : a.employee > b.employee;
                    case 4: // Часы
                        return ascending ? a.hours < b.hours : a.hours > b.hours;
                    case 5: // Стоимость
                        return ascending ? a.hour_cost < b.hour_cost : a.hour_cost > b.hour_cost;
                    default:
                        return false;
                }
            });
            // Пересчитываем количество страниц после сортировки
            totalPages = (projects.size() + dataRows - 1) / dataRows;
            // Ограничиваем текущую страницу
            currentPage = std::min(currentPage, totalPages - 1);
            if (currentPage < 0) currentPage = 0;
        } else if (ch == 'q' || ch == 'Q') {
            break;
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

    // Ограничения длины
    const int MAX_NAME_LENGTH = 14;
    const int MAX_WORK_LENGTH = 14;
    const int MAX_EMPLOYEE_LENGTH = 32;
    const int MAX_HOURS_LENGTH = 5;   // Новое ограничение для часов
    const int MAX_COST_LENGTH = 10;   // Новое ограничение для стоимости

    proj.name = inputString(2, 0, "Наименование проекта (макс. 14): ", false, MAX_NAME_LENGTH);
    proj.work_type = inputString(4, 0, "Вид работ (макс. 14): ", false, MAX_WORK_LENGTH);
    proj.employee = inputString(6, 0, "Ф.И.О. сотрудника (макс. 32): ", false, MAX_EMPLOYEE_LENGTH);
    proj.hours = inputInt(8, 0, "Часы (макс. 5): ", MAX_HOURS_LENGTH);
    proj.hour_cost = inputDouble(10, 0, "Стоимость часа (макс. 10): ", MAX_COST_LENGTH);

    auto projects = readProjects();
    projects.push_back(proj);
    writeProjects(projects);
    mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    refresh();
    napms(1000);
}


// Экран редактирования проекта
void editProjectScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Редактирование проекта ===");
    int index = inputInt(2, 0, "Введите номер проекта: ") - 1;
    auto projects = readProjects();

    if (index >= 0 && index < projects.size()) {
        Project& proj = projects[index];
        Project originalProj = proj; // Сохраняем копию оригинальных данных для отмены

        // Очищаем строку ввода номера проекта полностью перед выводом новой надписи
        mvprintw(2, 0, std::string(width, ' ').c_str()); // Заполняем строку пробелами
        mvprintw(2, 0, "Текущие данные проекта:");       // Записываем новую надпись
        refresh();

        // Фиксированные ширины столбцов (как в viewProjectsScreen)
        int colNumWidth = 4;      // "№"
        int colNameWidth = 15;    // "Название"
        int colWorkWidth = 15;    // "Работа"
        int colEmployeeWidth = 33;// "Сотрудник"
        int colHoursWidth = 6;    // "Часы"
        int colCostWidth = 10;    // "Стоимость"

        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5; // 5 разделителей "|"
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        // Заголовок таблицы
        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            "Название" + std::string(colNameWidth - 8, ' ') + "|" +
                            "Работа" + std::string(colWorkWidth - 6, ' ') + "|" +
                            "Сотрудник" + std::string(colEmployeeWidth - 9, ' ') + "|" +
                            "Часы" + std::string(colHoursWidth - 4, ' ') + "|" +
                            "Стоимость";
        mvprintw(4, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(5, 0, separator.c_str());

        // Вывод данных выбранного проекта в виде таблицы
        std::string num = std::to_string(index + 1) + std::string(colNumWidth - std::to_string(index + 1).length() - 1, ' ') + "|";
        std::string name = proj.name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(proj.name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
        std::string work = proj.work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(proj.work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
        std::string employee = proj.employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(proj.employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
        std::string hours = std::to_string(proj.hours) + std::string(colHoursWidth - std::to_string(proj.hours).length() - 1, ' ') + "|";
        std::string cost = std::to_string(proj.hour_cost).substr(0, colCostWidth - 1);

        std::string line = num + name + work + employee + hours + cost;
        mvprintw(6, 0, line.substr(0, width).c_str());

        // Сообщение перед вводом новых данных
        mvprintw(8, 0, "Введите новые данные (Enter для сохранения текущего значения, / для отмены):");

        // Ограничения длины
        const int MAX_NAME_LENGTH = 14;
        const int MAX_WORK_LENGTH = 14;
        const int MAX_EMPLOYEE_LENGTH = 32;
        const int MAX_HOURS_LENGTH = 5;   // Ограничение для часов
        const int MAX_COST_LENGTH = 10;   // Ограничение для стоимости

        // Редактирование с возможностью отмены по "/"
        std::string newName = inputString(9, 0, "Новое наименование (макс. 14): ", false, MAX_NAME_LENGTH);
        if (newName == "/") { // Проверка на "/"
            proj = originalProj; // Восстанавливаем оригинальные данные
            mvprintw(height - 1, 0, "Редактирование отменено!");
            refresh();
            napms(1000);
            return;
        }
        proj.name = newName.empty() ? proj.name : newName;

        std::string newWork = inputString(10, 0, "Новый вид работ (макс. 14): ", false, MAX_WORK_LENGTH);
        if (newWork == "/") {
            proj = originalProj;
            mvprintw(height - 1, 0, "Редактирование отменено!");
            refresh();
            napms(1000);
            return;
        }
        proj.work_type = newWork.empty() ? proj.work_type : newWork;

        std::string newEmployee = inputString(11, 0, "Новое Ф.И.О. сотрудника (макс. 32): ", false, MAX_EMPLOYEE_LENGTH);
        if (newEmployee == "/") {
            proj = originalProj;
            mvprintw(height - 1, 0, "Редактирование отменено!");
            refresh();
            napms(1000);
            return;
        }
        proj.employee = newEmployee.empty() ? proj.employee : newEmployee;

        std::string hoursInput = inputString(12, 0, "Новые часы (макс. 5): ", false, MAX_HOURS_LENGTH);
        if (hoursInput == "/") {
            proj = originalProj;
            mvprintw(height - 1, 0, "Редактирование отменено!");
            refresh();
            napms(1000);
            return;
        }
        if (!hoursInput.empty()) {
            try {
                proj.hours = std::stoi(hoursInput);
            } catch (const std::exception& e) {
                mvprintw(height - 1, 0, "Ошибка: часы оставлены без изменений!");
                refresh();
                napms(1000);
            }
        }

        std::string costInput = inputString(13, 0, "Новая стоимость часа (макс. 10): ", false, MAX_COST_LENGTH);
        if (costInput == "/") {
            proj = originalProj;
            mvprintw(height - 1, 0, "Редактирование отменено!");
            refresh();
            napms(1000);
            return;
        }
        if (!costInput.empty()) {
            try {
                proj.hour_cost = std::stod(costInput);
            } catch (const std::exception& e) {
                mvprintw(height - 1, 0, "Ошибка: стоимость оставлена без изменений!");
                refresh();
                napms(1000);
            }
        }

        writeProjects(projects);
        mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    } else {
        mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
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
    int index = inputInt(2, 0, "Введите номер проекта: ") - 1;
    auto projects = readProjects();
    if (index >= 0 && index < projects.size()) {
        mvprintw(4, 0, MSG_CONFIRM_DELETE.c_str());
        refresh();
        if (getch() == 'y') {
            projects.erase(projects.begin() + index);
            writeProjects(projects);
            mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
        }
    } else {
        mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
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

    int choice = getch() - '0';
    if (choice < 1 || choice > 3) {
        mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str());
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
    auto projects = readProjects();

    // Фильтрация проектов
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
        mvprintw(4, 0, MSG_NOT_FOUND.c_str());
        mvprintw(height - 1, 0, "Нажмите любую клавишу для возврата...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 4;
    int totalPages = (filteredProjects.size() + dataRows - 1) / dataRows;
    int currentPage = 0;

    while (true) {
        clear();
        mvprintw(0, 0, "=== Поиск проектов ===");

        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(filteredProjects.size()));

        // Фиксированные ширины столбцов (как в viewProjectsScreen)
        int colNumWidth = 4;      // "№"
        int colNameWidth = 15;    // "Название"
        int colWorkWidth = 15;    // "Работа"
        int colEmployeeWidth = 33;// "Сотрудник"
        int colHoursWidth = 6;    // "Часы"
        int colCostWidth = 10;    // "Стоимость"

        // Проверка, чтобы таблица помещалась в ширину экрана
        int totalWidth = colNumWidth + colNameWidth + colWorkWidth + colEmployeeWidth + colHoursWidth + colCostWidth + 5; // 5 разделителей "|"
        if (totalWidth > width) {
            colEmployeeWidth = width - (colNumWidth + colNameWidth + colWorkWidth + colHoursWidth + colCostWidth + 5);
        }

        // Заголовок таблицы
        std::string header = std::string(colNumWidth - 1, ' ') + "№ |" +
                            std::string("Название").substr(0, colNameWidth - 1) + std::string(colNameWidth - 8, ' ') + "|" +
                            std::string("Работа").substr(0, colWorkWidth - 1) + std::string(colWorkWidth - 7, ' ') + "|" +
                            std::string("Сотрудник").substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - 10, ' ') + "|" +
                            "Часы |" +
                            "Стоимость";
        mvprintw(2, 0, header.substr(0, width).c_str());

        std::string separator(totalWidth, '-');
        mvprintw(3, 0, separator.c_str());

        // Вывод данных
        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 2; ++i) {
            std::string num = std::to_string(i + 1) + std::string(colNumWidth - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string name = filteredProjects[i].name.substr(0, colNameWidth - 1) + std::string(colNameWidth - std::min(filteredProjects[i].name.length(), static_cast<size_t>(colNameWidth - 1)), ' ') + "|";
            std::string work = filteredProjects[i].work_type.substr(0, colWorkWidth - 1) + std::string(colWorkWidth - std::min(filteredProjects[i].work_type.length(), static_cast<size_t>(colWorkWidth - 1)), ' ') + "|";
            std::string employee = filteredProjects[i].employee.substr(0, colEmployeeWidth - 1) + std::string(colEmployeeWidth - std::min(filteredProjects[i].employee.length(), static_cast<size_t>(colEmployeeWidth - 1)), ' ') + "|";
            std::string hours = std::to_string(filteredProjects[i].hours) + std::string(colHoursWidth - std::to_string(filteredProjects[i].hours).length() - 1, ' ') + "|";
            std::string cost = std::to_string(filteredProjects[i].hour_cost).substr(0, colCostWidth - 1);

            std::string line = num + name + work + employee + hours + cost;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        std::string navigation = "Стрелки влево/вправо - переключение страниц | q - выход | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages);
        mvprintw(height - 1, 0, navigation.substr(0, width).c_str());
        refresh();

        int ch = getch();
        if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch == 'q' || ch == 'Q') {
            break;
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
        
        // Отображение текущих данных
        mvprintw(4, 0, "Текущий логин: %s", acc.login.c_str());
        mvprintw(5, 0, "Текущая роль: %d (0 - Пользователь, 1 - Админ)", acc.role);
        mvprintw(6, 0, "Текущий статус: %d (0 - Ожидает, 1 - Одобрен, 2 - Заблокирован)", acc.access);

        // Ввод новых данных
        std::string newLogin = inputString(8, 0, "Новый логин (Enter для сохранения текущего): ");
        refresh();
        if (!newLogin.empty()) {
            // Проверка на уникальность нового логина
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

        int newRole = inputInt(10, 0, "Новая роль (0 - Пользователь, 1 - Админ): ");
        if (newRole == 0 || newRole == 1) {
            acc.role = newRole;
        } else {
            mvprintw(height - 1, 0, "Неверное значение роли!");
            refresh();
            napms(1000);
            return;
        }

        int newAccess = inputInt(12, 0, "Новый статус (0 - Ожидает, 1 - Одобрен, 2 - Заблокирован): ");
        if (newAccess >= 0 && newAccess <= 2) {
            acc.access = newAccess;
        } else {
            mvprintw(height - 1, 0, "Неверное значение статуса!");
            refresh();
            napms(1000);
            return;
        }

        // Сохранение изменений
        writeAccounts(accounts);
        mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    } else {
        mvprintw(height - 1, 0, "Запись не найдена или это текущий пользователь!");
    }
    refresh();
    napms(1000);
}

// Экран управления учётными записями
void manageAccountsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Управление учётными записями ===");
    auto accounts = readAccounts();
    int y = 2;
    for (size_t i = 0; i < accounts.size() && y < height - 3; ++i) {
        std::string line = std::to_string(i + 1) + ". " + accounts[i].login + " (Role: " +
                          std::to_string(accounts[i].role) + ", Access: " + std::to_string(accounts[i].access) + ")";
        mvprintw(y++, 0, line.substr(0, width).c_str());
    }
    mvprintw(y++, 0, "1. Подтвердить | 2. Заблокировать | 3. Удалить | 4. Редактировать | 0. Назад");
    refresh();
    int choice = getch() - '0';
    if (choice == 0) return;
    int index = inputInt(y + 1, 0, "Введите номер записи: ") - 1;
    if (index >= 0 && index < accounts.size() && accounts[index].login != currentUser->login) {
        if (choice == 1) accounts[index].access = ACCESS_APPROVED;
        else if (choice == 2) accounts[index].access = ACCESS_BLOCKED;
        else if (choice == 3) {
            mvprintw(y + 2, 0, MSG_CONFIRM_DELETE.c_str());
            refresh();
            if (getch() == 'y') accounts.erase(accounts.begin() + index);
        }
        else if (choice == 4) {
            editAccountScreen(index); 
            return; 
        }
        writeAccounts(accounts);
        mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    } else {
        mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
    }
    refresh();
    napms(1000);
}

void pendingRequestsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Запросы на подтверждение ===");
    auto accounts = readAccounts();
    
    // Фильтруем пользователей с ACCESS_PENDING
    std::vector<Account> pendingAccounts;
    for (const auto& acc : accounts) {
        if (acc.access == ACCESS_PENDING) {
            pendingAccounts.push_back(acc);
        }
    }

    if (pendingAccounts.empty()) {
        mvprintw(2, 0, "Нет запросов на подтверждение!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу для возврата...");
        refresh();
        getch();
        return;
    }

    int y = 2;
    for (size_t i = 0; i < pendingAccounts.size() && y < height - 3; ++i) {
        std::string line = std::to_string(i + 1) + ". " + pendingAccounts[i].login + " (Ожидает подтверждения)";
        mvprintw(y++, 0, line.substr(0, width).c_str());
    }
    mvprintw(y++, 0, "1. Подтвердить | 2. Заблокировать | 0. Назад");
    refresh();

    int choice = getch() - '0';
    if (choice == 0) return;

    int index = inputInt(y + 1, 0, "Введите номер записи: ") - 1;
    if (index >= 0 && index < pendingAccounts.size()) {
        // Находим индекс в полном списке аккаунтов
        int realIndex = -1;
        for (size_t i = 0; i < accounts.size(); ++i) {
            if (accounts[i].login == pendingAccounts[index].login) {
                realIndex = i;
                break;
            }
        }
        if (realIndex != -1) {
            if (choice == 1) {
                accounts[realIndex].access = ACCESS_APPROVED;
                mvprintw(height - 1, 0, "Пользователь подтверждён!");
            } else if (choice == 2) {
                accounts[realIndex].access = ACCESS_BLOCKED;
                mvprintw(height - 1, 0, "Пользователь заблокирован!");
            }
            writeAccounts(accounts);
        }
    } else {
        mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
    }
    refresh();
    napms(1000);
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
        if (currentUser->role == ROLE_ADMIN) {
            mvprintw(y++, 0, "3. Управление учётными записями"); // Было 4
            mvprintw(y++, 0, "4. Добавить проект");              // Было 5
            mvprintw(y++, 0, "5. Редактировать проект");         // Было 6
            mvprintw(y++, 0, "6. Удалить проект");               // Было 7
            mvprintw(y++, 0, "7. Регистрация пользователя");     // Было 8
            mvprintw(y++, 0, "8. Запросы на подтверждение");     // Было 9
        }
        mvprintw(y++, 0, "0. Выход");
        refresh();

        int choice = getch() - '0';
        if (choice == 0) break;
        switch (choice) {
            case 1: viewProjectsScreen(); break;
            case 2: searchProjectsScreen(); break;
            case 3: if (currentUser->role == ROLE_ADMIN) manageAccountsScreen(); break;
            case 4: if (currentUser->role == ROLE_ADMIN) addProjectScreen(); break;
            case 5: if (currentUser->role == ROLE_ADMIN) editProjectScreen(); break;
            case 6: if (currentUser->role == ROLE_ADMIN) deleteProjectScreen(); break;
            case 7: if (currentUser->role == ROLE_ADMIN) registerScreen(true); break;
            case 8: if (currentUser->role == ROLE_ADMIN) pendingRequestsScreen(); break;
            default: mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str()); refresh(); napms(1000);
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

    // Инициализация PDCurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    srand(time(nullptr));

    // Главный цикл
    while (true) {
        int height, width;
        getmaxyx(stdscr, height, width);
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
            mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str());
            refresh();
            napms(1000);
        }
    }
    endwin();
    return 0;
}