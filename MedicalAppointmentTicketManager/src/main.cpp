#define NOMINMAX
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <Windows.h>
#include "Database.h"
#include "Service.h"
#include "Validation.h"



// Проверки пользовательского ввода.
Validation validation;



// Ввод
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        // Больше нечего читать — завершаем программу
        std::cout << "\nВвод завершён. Выход из программы.\n";
        std::exit(0);
    }
    return line;
}



// Вывод таблиц

// Считает символы UTF-8 для ширины строки
size_t utf8DisplayWidth(const std::string& s) {
    size_t width = 0;
    for (unsigned char c : s) {
        // Байты-продолжения не считаем отдельными символами
        if ((c & 0xC0) != 0x80) {
            ++width;
        }
    }
    return width;
}



// Дополняет строку пробелами до нужной ширины
std::string utf8PadRight(const std::string& s, size_t targetWidth) {
    size_t width = utf8DisplayWidth(s);
    if (width >= targetWidth) return s;
    return s + std::string(targetWidth - width, ' ');
}



// Выводит таблицу с выравниванием столбцов
void printTable(const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows) {

    // Ширина колонки берется сначала по длине заголовка
    size_t cols = headers.size();
    std::vector<size_t> widths(cols);

    // Расширяем ширину колонки, если в данных есть значение длиннее заголовка
    for (size_t c = 0; c < cols; ++c)
        widths[c] = utf8DisplayWidth(headers[c]);

    for (const auto& row : rows)
        for (size_t c = 0; c < cols && c < row.size(); ++c)
            widths[c] = std::max(widths[c], utf8DisplayWidth(row[c]));

    // Печать разделительной линии
    auto separator = [&]() {
        for (size_t c = 0; c < cols; ++c)
            std::cout << "+" << std::string(widths[c] + 2, '-');
        std::cout << "+\n";
        };

    // Печать одной строки таблицы с выравниванием по ширине колонок
    auto printRow = [&](const std::vector<std::string>& row) {
        std::cout << "|";
        for (size_t c = 0; c < cols; ++c) {
            std::string val = (c < row.size()) ? row[c] : "";
            std::cout << " " << utf8PadRight(val, widths[c]) << " |";
        }
        std::cout << "\n";
        };

    // Печать таблицы
    separator();
    printRow(headers); // строка заголовков
    separator();
    if (rows.empty()) {
        std::cout << "| (нет данных)\n"; // пустой список
    }
    else {
        for (const auto& row : rows)
            printRow(row); // печать каждой строки данных
    }
    separator();
}



// Заголовки таблицы талонов
const std::vector<std::string> TICKET_HEADERS = { "ID", "ФИО врача", "Специальность", "Дата", "Время", "Пациент" };



// Подготавливает данные врачей для таблицы
std::vector<std::vector<std::string>> doctorsToRows(const std::vector<Doctor>& doctors) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& d : doctors)
        rows.push_back({ std::to_string(d.getId()), d.getFio(), d.getSpeciality(), std::to_string(d.getOffice()) });
    return rows;
}



// Подготавливает данные пациентов для таблицы
std::vector<std::vector<std::string>> patientsToRows(const std::vector<Patient>& patients) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& p : patients)
        rows.push_back({ std::to_string(p.getId()), p.getFio(), p.getPhone() });
    return rows;
}



// Подготавливает данные талонов для таблицы
std::vector<std::vector<std::string>> ticketsToRows(const std::vector<TicketView>& tickets) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& t : tickets) {
        std::string patientCell = t.patientFio ? *t.patientFio : "свободен";
        rows.push_back({ std::to_string(t.id), t.doctorFio, t.speciality, t.date, t.time, patientCell });
    }
    return rows;
}



// Подготавливает историю обращений для таблицы
std::vector<std::vector<std::string>> historyToRows(const std::vector<HistoryRecord>& history) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& h : history)
        rows.push_back({ std::to_string(h.id), std::to_string(h.ticket_id), std::to_string(h.patient_id),
                          h.action_time, h.action_type });
    return rows;
}



// Печатает свободные талоны или сообщение, если их нет
void printFreeSlots(const std::vector<TicketView>& tickets) {
    if (tickets.empty()) {
        std::cout << "Свободные талоны не найдены.\n";
    }
    else {
        printTable(TICKET_HEADERS, ticketsToRows(tickets));
    }
}



// Преобразует строку в int или возвращает nullopt
std::optional<int> safeStoi(const std::string& s) {
    try {
        size_t pos = 0;
        int val = std::stoi(s, &pos);
        if (pos != s.size()) return std::nullopt;
        return val;
    }
    catch (...) {
        return std::nullopt;
    }
}



// Поиск пациента по ID или телефону
std::optional<Patient> findPatientByIdOrPhone(Service& service, const std::string& input) {
    if (!input.empty() && input[0] == '+') {
        auto resp = service.loginPatient(input);
        if (resp.status == LoginPatientResult::Ok) {
            return resp.patient;
        }
        return std::nullopt;
    }

    bool allDigits = !input.empty() &&
        std::all_of(input.begin(), input.end(), [](unsigned char c) { return std::isdigit(c); });
    if (!allDigits) {
        return std::nullopt;
    }
    auto idOpt = safeStoi(input);
    if (!idOpt) {
        return std::nullopt;
    }
    for (const auto& p : service.getPatients()) {
        if (p.getId() == *idOpt) return p;
    }
    return std::nullopt;
}



// Поиск талона по ID в уже загруженном списке
const TicketView* findInList(const std::vector<TicketView>& tickets, int id) {
    for (const auto& t : tickets)
        if (t.id == id) return &t;
    return nullptr;
}



// Повторяет ввод, пока значение не пройдет проверку
template <typename Predicate>
std::string readUntilValid(const std::string& prompt, Predicate isValid,
    const std::string& errorMsg = "Неверный формат ввода. Повторите ввод.\n") {
    while (true) {
        std::string value = readLine(prompt);
        if (isValid(value)) return value;
        std::cout << errorMsg;
    }
}



// Проверяет, есть ли врач с таким ID
bool doctorIdExists(const std::vector<Doctor>& doctors, const std::string& idStr) {
    auto idOpt = safeStoi(idStr);
    if (!idOpt) return false;
    for (const auto& d : doctors)
        if (d.getId() == *idOpt) return true;
    return false;
}



// Проверяет, есть ли талон с таким ID
bool ticketIdExistsInList(const std::vector<TicketView>& tickets, const std::string& idStr) {
    auto idOpt = safeStoi(idStr);
    if (!idOpt) return false;
    return findInList(tickets, *idOpt) != nullptr;
}



// Сообщения по результатам

// Выводит результат добавления врача
void printResult(AddDoctorResult r) {
    switch (r) {
    case AddDoctorResult::Ok: std::cout << "Врач успешно добавлен.\n"; break;
    case AddDoctorResult::InvalidFio: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case AddDoctorResult::EmptySpeciality: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case AddDoctorResult::InvalidOffice: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case AddDoctorResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Выводит результат удаления врача
void printResult(RemoveDoctorResult r) {
    switch (r) {
    case RemoveDoctorResult::Ok: std::cout << "Врач успешно удалён.\n"; break;
    case RemoveDoctorResult::DoctorNotFound: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case RemoveDoctorResult::HasTickets: std::cout << "Удаление невозможно: у врача есть талоны.\n"; break;
    case RemoveDoctorResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Выводит результат создания талона
void printResult(CreateTicketResult r) {
    switch (r) {
    case CreateTicketResult::Ok: std::cout << "Талон успешно создан.\n"; break;
    case CreateTicketResult::DoctorNotFound: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CreateTicketResult::InvalidDate: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CreateTicketResult::InvalidTime: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CreateTicketResult::TicketInPast: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CreateTicketResult::Duplicate: std::cout << "Талон на это время у данного врача уже существует.\n"; break;
    case CreateTicketResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Выводит результат регистрации пациента
void printResult(RegisterPatientResult r) {
    switch (r) {
    case RegisterPatientResult::Ok: std::cout << "Пациент успешно зарегистрирован.\n"; break;
    case RegisterPatientResult::InvalidFio: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case RegisterPatientResult::InvalidPhone: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case RegisterPatientResult::PhoneTaken: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case RegisterPatientResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Выводит результат записи пациента на талон
void printResult(BookResult r) {
    switch (r) {
    case BookResult::Ok: std::cout << "Запись успешно оформлена.\n"; break;
    case BookResult::TicketNotFound: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case BookResult::TicketAlreadyBooked: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case BookResult::PatientNotFound: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case BookResult::AlreadyBookedWithDoctorOnDate: std::cout << "Пациент уже записан к этому врачу на выбранную дату.\n"; break;
    case BookResult::TicketInPast: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case BookResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Выводит результат отмены записи
void printResult(CancelResult r) {
    switch (r) {
    case CancelResult::Ok: std::cout << "Запись успешно отменена.\n"; break;
    case CancelResult::TicketNotFound: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CancelResult::TicketAlreadyFree: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CancelResult::NotPatientTicket: std::cout << "Неверный формат ввода. Повторите ввод.\n"; break;
    case CancelResult::DatabaseError: std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n"; break;
    }
}



// Меню администратора

// Запускает меню администратора
void runAdminMenu(Service& service) {
    while (true) {
        std::cout << "\n--- Меню администратора ---\n"
            << "1. Просмотр всех талонов\n"
            << "2. Просмотр свободных талонов\n"
            << "3. Создать талон\n"
            << "4. Записать пациента на талон\n"
            << "5. Отменить запись\n"
            << "6. Добавить врача\n"
            << "7. Удалить врача\n"
            << "8. Просмотр списка врачей\n"
            << "9. Просмотр списка пациентов\n"
            << "10. Просмотр истории обращений\n"
            << "11. Просмотр свободных талонов конкретного врача\n"
            << "0. Выход\n";
        std::string choice = readLine("Выберите пункт меню: ");

        if (choice == "1") {
            printTable(TICKET_HEADERS, ticketsToRows(service.getAllTickets()));
        }
        else if (choice == "2") {
            std::string speciality = readLine("Специальность (Enter — пропустить): ");
            std::string doctorFio = readLine("ФИО врача (Enter — пропустить): ");
            std::string date = readLine("Дата ГГГГ-ММ-ДД (Enter — пропустить): ");
            auto resp = service.getFreeSlots(speciality, doctorFio, date);
            if (resp.status == FreeSlotsResult::Ok) {
                printFreeSlots(resp.tickets);
            }
            else {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
        }
        else if (choice == "3") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));

            if (doctors.empty()) {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
            else {
                std::string doctorId = readUntilValid("ID врача: ",
                    [&](const std::string& s) { return doctorIdExists(doctors, s); });

                std::string date = readUntilValid("Дата приёма (ГГГГ-ММ-ДД): ",
                    [](const std::string& s) { return validation.isValidDate(s); });

                std::string time = readUntilValid("Время приёма (ЧЧ:ММ): ",
                    [](const std::string& s) { return validation.isValidTime(s); });

                printResult(service.createTicket(doctorId, date, time));
            }
        }
        else if (choice == "4") {
            auto freeResp = service.getFreeSlots("", "", "");
            printFreeSlots(freeResp.tickets);

            std::string ticketId = freeResp.tickets.empty()
                ? readLine("ID талона: ")
                : readUntilValid("ID талона: ",
                    [&](const std::string& s) { return ticketIdExistsInList(freeResp.tickets, s); });

            std::string patientRef = readLine("ID пациента или телефон (+7XXXXXXXXXX): ");

            auto patient = findPatientByIdOrPhone(service, patientRef);
            if (!patient) {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
            else {
                printResult(service.book(ticketId, patient->getId()));
            }
        }
        else if (choice == "5") {
            auto allTickets = service.getAllTickets();
            std::vector<TicketView> booked;
            for (const auto& t : allTickets)
                if (!t.isFree()) booked.push_back(t);
            printTable(TICKET_HEADERS, ticketsToRows(booked));

            if (booked.empty()) {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
            else {
                std::string ticketIdStr = readUntilValid("ID талона: ",
                    [&](const std::string& s) { return ticketIdExistsInList(booked, s); });
                const TicketView* found = findInList(booked, *safeStoi(ticketIdStr));
                printResult(service.cancel(ticketIdStr, *found->patientId));
            }
        }
        else if (choice == "6") {
            std::string fio = readUntilValid("ФИО врача: ",
                [](const std::string& s) { return validation.isValidFio(s); });

            std::string speciality = readLine("Специальность: ");

            std::string office = readUntilValid("Номер кабинета: ",
                [](const std::string& s) {
                    auto n = safeStoi(s);
                    return n.has_value() && validation.isValidOffice(*n);
                });

            printResult(service.addDoctor(fio, speciality, office));
        }
        else if (choice == "7") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));

            if (doctors.empty()) {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
            else {
                std::string id = readUntilValid("ID врача: ",
                    [&](const std::string& s) { return doctorIdExists(doctors, s); });
                printResult(service.removeDoctor(id));
            }
        }
        else if (choice == "8") {
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(service.getDoctors()));
        }
        else if (choice == "9") {
            printTable({ "ID", "ФИО", "Телефон" }, patientsToRows(service.getPatients()));
        }
        else if (choice == "10") {
            printTable({ "ID", "Талон", "Пациент", "Время действия", "Действие" }, historyToRows(service.getHistory()));
        }
        else if (choice == "11") {
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(service.getDoctors()));
            std::string doctorId = readLine("ID врача: ");
            std::string date = readLine("Дата (ГГГГ-ММ-ДД): ");
            auto resp = service.getFreeSlotsByDoctor(doctorId, date);
            if (resp.status == FreeSlotsResult::Ok) {
                printFreeSlots(resp.tickets);
            }
            else {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
        }
        else if (choice == "0") {
            break;
        }
        else {
            std::cout << "Неверный формат ввода. Повторите ввод.\n";
        }
    }
}



// Меню пациента

// Выполняет вход пациента или предлагает регистрацию
std::optional<int> loginOrRegisterPatient(Service& service) {
    std::string phone = readUntilValid("Введите номер телефона (+7XXXXXXXXXX): ",
        [](const std::string& s) { return validation.isValidPhone(s); });

    auto loginResp = service.loginPatient(phone);

    if (loginResp.status == LoginPatientResult::Ok) {
        std::cout << "Добро пожаловать, " << loginResp.patient->getFio() << "!\n";
        return loginResp.patient->getId();
    }

    if (loginResp.status == LoginPatientResult::DatabaseError) {
        std::cout << "Ошибка базы данных. Попробуйте ещё раз.\n";
        return std::nullopt;
    }

    // Пациент не найден — предлагаем регистрацию
    std::string answer = readLine(
        "Пациент с таким телефоном не найден. Зарегистрировать нового пациента? (1 - да, 2 - нет): ");
    if (answer != "1") {
        return std::nullopt;
    }

    std::string fio = readLine("ФИО: ");
    auto regResult = service.registerPatient(fio, phone);
    printResult(regResult);
    if (regResult != RegisterPatientResult::Ok) {
        return std::nullopt;
    }

    auto newLogin = service.loginPatient(phone);
    if (newLogin.status == LoginPatientResult::Ok) {
        return newLogin.patient->getId();
    }
    return std::nullopt;
}



// Запускает меню пациента
void runPatientMenu(Service& service) {
    auto patientIdOpt = loginOrRegisterPatient(service);
    if (!patientIdOpt) {
        return;
    }
    int patientId = *patientIdOpt;

    while (true) {
        std::cout << "\n--- Меню пациента ---\n"
            << "1. Просмотр свободных талонов\n"
            << "2. Записаться на талон\n"
            << "3. Отменить свою запись\n"
            << "4. Просмотр своих талонов\n"
            << "5. Просмотр свободных талонов конкретного врача\n"
            << "0. Выход\n";
        std::string choice = readLine("Выберите пункт меню: ");

        if (choice == "1") {
            std::string speciality = readLine("Специальность (Enter — пропустить): ");
            std::string doctorFio = readLine("ФИО врача (Enter — пропустить): ");
            std::string date = readLine("Дата ГГГГ-ММ-ДД (Enter — пропустить): ");
            auto resp = service.getFreeSlots(speciality, doctorFio, date);
            if (resp.status == FreeSlotsResult::Ok) {
                printFreeSlots(resp.tickets);
            }
            else {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
        }
        else if (choice == "2") {
            auto freeResp = service.getFreeSlots("", "", "");
            printFreeSlots(freeResp.tickets);

            std::string ticketId = freeResp.tickets.empty()
                ? readLine("ID талона: ")
                : readUntilValid("ID талона: ",
                    [&](const std::string& s) { return ticketIdExistsInList(freeResp.tickets, s); });

            printResult(service.book(ticketId, patientId));
        }
        else if (choice == "3") {
            auto myTickets = service.getPatientTickets(patientId);
            printTable(TICKET_HEADERS, ticketsToRows(myTickets));

            std::string ticketId = myTickets.empty()
                ? readLine("ID талона: ")
                : readUntilValid("ID талона: ",
                    [&](const std::string& s) { return ticketIdExistsInList(myTickets, s); });

            printResult(service.cancel(ticketId, patientId));
        }
        else if (choice == "4") {
            printTable(TICKET_HEADERS, ticketsToRows(service.getPatientTickets(patientId)));
        }
        else if (choice == "5") {
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(service.getDoctors()));
            std::string doctorId = readLine("ID врача: ");
            std::string date = readLine("Дата (ГГГГ-ММ-ДД): ");
            auto resp = service.getFreeSlotsByDoctor(doctorId, date);
            if (resp.status == FreeSlotsResult::Ok) {
                printFreeSlots(resp.tickets);
            }
            else {
                std::cout << "Неверный формат ввода. Повторите ввод.\n";
            }
        }
        else if (choice == "0") {
            break;
        }
        else {
            std::cout << "Неверный формат ввода. Повторите ввод.\n";
        }
    }
}



int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::filesystem::create_directories("data");
    const std::string path = "data/tickets.db";

    try {
        Database db(path);
        Service service(db);

        while (true) {
            std::cout << "\n=== Система записи к врачам ===\n"
                << "1. Администратор\n"
                << "2. Пациент\n"
                << "0. Выход\n";

            std::string choice = readUntilValid("Выберите роль: ",
                [](const std::string& s) { return s == "0" || s == "1" || s == "2"; });

            if (choice == "1") {
                try { runAdminMenu(service); }
                catch (const SQLite::Exception& e) {
                    std::cout << "Ошибка базы данных: " << e.what() << "\n";
                }
            }
            else if (choice == "2") {
                try { runPatientMenu(service); }
                catch (const SQLite::Exception& e) {
                    std::cout << "Ошибка базы данных: " << e.what() << "\n";
                }
            }
            else {
                break;
            }
        }
    }
    catch (const SQLite::Exception& e) {
        std::cerr << "Критическая ошибка базы данных: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
