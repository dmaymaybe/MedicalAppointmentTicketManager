#define NOMINMAX
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <optional>
#include <cstdlib>
#include <filesystem>
#include <Windows.h>
#include "Database.h"
#include "Service.h"
#include "Validation.h"



// Проверки пользовательского ввода.
Validation validation;



// Удаляет пробельные символы только в начале и конце строки.
// Пробелы между словами (например, в ФИО) сохраняются.
std::string trimWhitespace(const std::string& value) {
    const std::string whitespace = " \t\r\n\f\v";
    const size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }

    const size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

// Ввод: сразу обрезаем пробелы по краям для всех полей программы.
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        // Больше нечего читать — завершаем программу
        std::cout << "\nВвод завершён. Выход из программы.\n";
        std::exit(0);
    }
    return trimWhitespace(line);
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
        rows.push_back({ std::to_string(h.id), std::to_string(h.ticket_id), h.fio,
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


// Отбирает из общего списка талоны конкретного врача.
std::vector<TicketView> ticketsByDoctor(const std::vector<TicketView>& tickets, int doctorId) {
    std::vector<TicketView> result;
    for (const auto& ticket : tickets) {
        if (ticket.doctorId == doctorId) result.push_back(ticket);
    }
    return result;
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


// Ввод поля с возможностью отменить текущую операцию нулём.
std::optional<std::string> readCancellableLine(const std::string& prompt) {
    std::string value = readLine(prompt + " (введите 0 для отмены): ");
    if (value == "0") {
        std::cout << "Операция отменена.\n";
        return std::nullopt;
    }
    return value;
}

// Как readUntilValid, но пользователь может отменить ввод значением 0.
template <typename Predicate>
std::optional<std::string> readUntilValidOrCancel(
    const std::string& prompt,
    Predicate isValid,
    const std::string& errorMsg = "Некорректное значение. Проверьте формат и повторите ввод.\n") {
    while (true) {
        auto value = readCancellableLine(prompt);
        if (!value) return std::nullopt;
        if (isValid(*value)) return value;
        std::cout << errorMsg;
    }
}


// Время записи допускается только с минутами 00 или 30.
bool isValidAppointmentTime(const std::string& value) {
    if (!validation.isValidTime(value) || value.size() != 5 || value[2] != ':') {
        return false;
    }
    const std::string minutes = value.substr(3, 2);
    return minutes == "00" || minutes == "30";
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



// Понятные сообщения по результатам операций.
void printResult(AddDoctorResult r) {
    switch (r) {
    case AddDoctorResult::Ok: std::cout << "Врач успешно добавлен.\n"; break;
    case AddDoctorResult::InvalidFio: std::cout << "Ошибка: ФИО врача должно содержать корректные имя и фамилию.\n"; break;
    case AddDoctorResult::EmptySpeciality: std::cout << "Ошибка: укажите специальность врача.\n"; break;
    case AddDoctorResult::InvalidOffice: std::cout << "Ошибка: номер кабинета должен быть положительным числом.\n"; break;
    case AddDoctorResult::DatabaseError: std::cout << "Не удалось добавить врача: ошибка базы данных.\n"; break;
    }
}

void printResult(RemoveDoctorResult r) {
    switch (r) {
    case RemoveDoctorResult::Ok: std::cout << "Врач успешно удалён.\n"; break;
    case RemoveDoctorResult::DoctorNotFound: std::cout << "Ошибка: врач с таким ID не найден.\n"; break;
    case RemoveDoctorResult::HasTickets: std::cout << "Удаление невозможно: у врача есть талоны. Сначала удалите его талоны.\n"; break;
    case RemoveDoctorResult::DatabaseError: std::cout << "Не удалось удалить врача: ошибка базы данных.\n"; break;
    }
}

void printResult(CreateTicketResult r) {
    switch (r) {
    case CreateTicketResult::Ok: std::cout << "Талон успешно создан.\n"; break;
    case CreateTicketResult::DoctorNotFound: std::cout << "Ошибка: врач с таким ID не найден.\n"; break;
    case CreateTicketResult::InvalidDate: std::cout << "Ошибка: дата должна быть в формате ГГГГ-ММ-ДД и существовать в календаре.\n"; break;
    case CreateTicketResult::InvalidTime: std::cout << "Ошибка: время должно быть в формате ЧЧ:ММ, минуты — только 00 или 30 (например, 14:00 или 14:30).\n"; break;
    case CreateTicketResult::TicketInPast: std::cout << "Ошибка: нельзя создать талон на прошедшие дату или время.\n"; break;
    case CreateTicketResult::Duplicate: std::cout << "Ошибка: талон на это время у данного врача уже существует.\n"; break;
    case CreateTicketResult::DatabaseError: std::cout << "Не удалось создать талон: ошибка базы данных.\n"; break;
    }
}

void printResult(RegisterPatientResult r) {
    switch (r) {
    case RegisterPatientResult::Ok: std::cout << "Пациент успешно зарегистрирован.\n"; break;
    case RegisterPatientResult::InvalidFio: std::cout << "Ошибка: проверьте правильность ввода ФИО.\n"; break;
    case RegisterPatientResult::InvalidPhone: std::cout << "Ошибка: телефон должен быть в формате +7XXXXXXXXXX.\n"; break;
    case RegisterPatientResult::PhoneTaken: std::cout << "Ошибка: пациент с таким номером телефона уже зарегистрирован.\n"; break;
    case RegisterPatientResult::DatabaseError: std::cout << "Не удалось зарегистрировать пациента: ошибка базы данных.\n"; break;
    }
}

void printResult(BookResult r) {
    switch (r) {
    case BookResult::Ok: std::cout << "Запись успешно оформлена.\n"; break;
    case BookResult::TicketNotFound: std::cout << "Ошибка: талон не найден.\n"; break;
    case BookResult::TicketAlreadyBooked: std::cout << "Ошибка: этот талон уже занят. Выберите свободный талон.\n"; break;
    case BookResult::PatientNotFound: std::cout << "Ошибка: пациент не найден. Проверьте ID пациента.\n"; break;
    case BookResult::AlreadyBookedWithDoctorOnDate: std::cout << "Пациент уже записан к этому врачу на выбранную дату.\n"; break;
    case BookResult::TicketInPast: std::cout << "Ошибка: нельзя записаться на прошедшие дату или время.\n"; break;
    case BookResult::DatabaseError: std::cout << "Не удалось оформить запись: ошибка базы данных.\n"; break;
    }
}

void printResult(CancelResult r) {
    switch (r) {
    case CancelResult::Ok: std::cout << "Запись успешно отменена.\n"; break;
    case CancelResult::TicketNotFound: std::cout << "Ошибка: талон не найден.\n"; break;
    case CancelResult::TicketAlreadyFree: std::cout << "Ошибка: выбранный талон уже свободен.\n"; break;
    case CancelResult::NotPatientTicket: std::cout << "Ошибка: этот талон не принадлежит текущему пациенту.\n"; break;
    case CancelResult::DatabaseError: std::cout << "Не удалось отменить запись: ошибка базы данных.\n"; break;
    }
}


// Меню администратора

// Запускает меню администратора.
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
            << "12. Просмотр всех талонов конкретного врача (свободных/занятых)\n"
            << "13. Добавить пациента\n"
            << "0. Выход\n";
        std::string choice = readLine("Выберите пункт меню (номер от 0 до 13; 0 — выход): ");
        // Дополнительно очищаем номер пункта меню перед сравнением.
        choice = trimWhitespace(choice);

        if (choice == "1") {
            printTable(TICKET_HEADERS, ticketsToRows(service.getAllTickets()));
        }
        else if (choice == "2") {
            auto doctors = service.getDoctors();
            if (!doctors.empty()) {
                std::cout << "Список врачей (для точного ввода ФИО в фильтре ниже):\n";
                printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            }

            auto speciality = readCancellableLine("Специальность (например, Кардиология; Enter — пропустить)");
            if (!speciality) continue;
            auto doctorFio = readCancellableLine("ФИО врача (скопируйте из списка выше, например Иванов И. И.; Enter — пропустить)");
            if (!doctorFio) continue;
            auto date = readCancellableLine("Дата ГГГГ-ММ-ДД (например, 2026-10-20; Enter — пропустить)");
            if (!date) continue;

            auto resp = service.getFreeSlots(*speciality, *doctorFio, *date);
            if (resp.status == FreeSlotsResult::Ok) printFreeSlots(resp.tickets);
            else std::cout << "Не удалось получить свободные талоны. Проверьте фильтры и формат даты.\n";
        }
        else if (choice == "3") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            if (doctors.empty()) {
                std::cout << "Список врачей пуст. Сначала добавьте врача.\n";
                continue;
            }

            auto doctorId = readUntilValidOrCancel("ID врача (целое число из списка)", [&](const std::string& value) {
                return doctorIdExists(doctors, value);
                }, "Врач с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!doctorId) continue;
            auto date = readUntilValidOrCancel("Дата приёма (формат ГГГГ-ММ-ДД, например 2026-10-20)",
                [](const std::string& value) { return validation.isValidDate(value); },
                "Некорректная дата. Используйте формат ГГГГ-ММ-ДД, например 2026-10-20. Введите 0 для отмены.\n");
            if (!date) continue;
            auto time = readUntilValidOrCancel("Время приёма (ЧЧ:ММ; запись только на :00 или :30, например 14:00 или 14:30)",
                [](const std::string& value) { return isValidAppointmentTime(value); },
                "Некорректное время. Используйте ЧЧ:00 или ЧЧ:30 (например, 14:00 или 14:30).\n");
            if (!time) continue;
            printResult(service.createTicket(*doctorId, *date, *time));
        }
        else if (choice == "4") {
            auto freeResp = service.getFreeSlots("", "", "");
            if (freeResp.status != FreeSlotsResult::Ok) {
                std::cout << "Не удалось загрузить свободные талоны.\n";
                continue;
            }
            printFreeSlots(freeResp.tickets);
            if (freeResp.tickets.empty()) continue;

            auto ticketId = readUntilValidOrCancel("ID талона (целое число из списка свободных талонов)", [&](const std::string& value) {
                return ticketIdExistsInList(freeResp.tickets, value);
                }, "Свободный талон с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!ticketId) continue;
            auto patientRef = readCancellableLine("ID пациента (целое число) или телефон в формате +7XXXXXXXXXX, например +77001234567");
            if (!patientRef) continue;

            auto patient = findPatientByIdOrPhone(service, *patientRef);
            if (!patient) {
                std::cout << "Пациент не найден. Проверьте ID или номер телефона.\n";
                continue;
            }
            printResult(service.book(*ticketId, patient->getId()));
        }
        else if (choice == "5") {
            auto allTickets = service.getAllTickets();
            std::vector<TicketView> booked;
            for (const auto& ticket : allTickets)
                if (!ticket.isFree()) booked.push_back(ticket);
            printTable(TICKET_HEADERS, ticketsToRows(booked));
            if (booked.empty()) {
                std::cout << "Нет занятых талонов для отмены записи.\n";
                continue;
            }

            auto ticketIdStr = readUntilValidOrCancel("ID занятого талона (целое число из списка)", [&](const std::string& value) {
                return ticketIdExistsInList(booked, value);
                }, "Занятый талон с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!ticketIdStr) continue;
            const TicketView* found = findInList(booked, *safeStoi(*ticketIdStr));
            if (!found || !found->patientId) {
                std::cout << "Не удалось определить пациента по выбранному талону.\n";
                continue;
            }
            printResult(service.cancel(*ticketIdStr, *found->patientId));
        }
        else if (choice == "6") {
            auto fio = readUntilValidOrCancel("ФИО врача (например, Иванов Иван Иванович)", [](const std::string& value) {
                return validation.isValidFio(value);
                }, "Некорректное ФИО. Повторите ввод или введите 0 для отмены.\n");
            if (!fio) continue;
            auto speciality = readUntilValidOrCancel("Специальность врача (например, Кардиология)", [](const std::string& value) {
                return !value.empty() && value.find_first_not_of(" \t") != std::string::npos;
                }, "Специальность не должна быть пустой. Повторите ввод или введите 0 для отмены.\n");
            if (!speciality) continue;
            auto office = readUntilValidOrCancel("Номер кабинета (положительное целое число, например 12)", [](const std::string& value) {
                auto number = safeStoi(value);
                return number.has_value() && validation.isValidOffice(*number);
                }, "Номер кабинета должен быть корректным положительным числом. Повторите ввод или введите 0 для отмены.\n");
            if (!office) continue;
            printResult(service.addDoctor(*fio, *speciality, *office));
        }
        else if (choice == "7") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            if (doctors.empty()) {
                std::cout << "Список врачей пуст. Удалять нечего.\n";
                continue;
            }
            auto id = readUntilValidOrCancel("ID врача для удаления (целое число из списка)", [&](const std::string& value) {
                return doctorIdExists(doctors, value);
                }, "Врач с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!id) continue;
            printResult(service.removeDoctor(*id));
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
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            if (doctors.empty()) {
                std::cout << "Список врачей пуст.\n";
                continue;
            }
            auto doctorId = readUntilValidOrCancel("ID врача (целое число из списка)", [&](const std::string& value) {
                return doctorIdExists(doctors, value);
                }, "Врач с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!doctorId) continue;
            auto date = readUntilValidOrCancel("Дата (формат ГГГГ-ММ-ДД, например 2026-10-20)",
                [](const std::string& value) { return validation.isValidDate(value); },
                "Некорректная дата. Используйте формат ГГГГ-ММ-ДД или 0 для отмены.\n");
            if (!date) continue;
            auto resp = service.getFreeSlotsByDoctor(*doctorId, *date);
            if (resp.status == FreeSlotsResult::Ok) printFreeSlots(resp.tickets);
            else std::cout << "Не удалось получить свободные талоны врача. Проверьте ID и дату.\n";
        }
        else if (choice == "12") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            if (doctors.empty()) {
                std::cout << "Список врачей пуст.\n";
                continue;
            }
            auto doctorId = readUntilValidOrCancel("ID врача (целое число из списка)", [&](const std::string& value) {
                return doctorIdExists(doctors, value);
                }, "Врач с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!doctorId) continue;
            auto doctorIdValue = safeStoi(*doctorId);
            if (!doctorIdValue) {
                std::cout << "Некорректный ID врача.\n";
                continue;
            }
            auto doctorTickets = ticketsByDoctor(service.getAllTickets(), *doctorIdValue);
            std::cout << "Все талоны выбранного врача (свободные и занятые):\n";
            if (doctorTickets.empty()) std::cout << "У этого врача пока нет талонов.\n";
            else printTable(TICKET_HEADERS, ticketsToRows(doctorTickets));
        }
        else if (choice == "13") {
            auto fio = readUntilValidOrCancel("ФИО пациента (например, Иванов Иван Иванович)", [](const std::string& value) {
                return validation.isValidFio(value);
                }, "Некорректное ФИО. Повторите ввод или введите 0 для отмены.\n");
            if (!fio) continue;
            auto phone = readUntilValidOrCancel("Телефон пациента в формате +7XXXXXXXXXX (например, +77001234567)",
                [](const std::string& value) { return validation.isValidPhone(value); },
                "Некорректный телефон. Используйте формат +7XXXXXXXXXX или введите 0 для отмены.\n");
            if (!phone) continue;
            printResult(service.registerPatient(*fio, *phone));
        }
        else if (choice == "0") {
            break;
        }
        else {
            std::cout << "Неизвестный пункт меню. Выберите число от 0 до 13.\n";
        }
    }
}


// Меню пациента

// Выполняет вход пациента или предлагает регистрацию.
std::optional<int> loginOrRegisterPatient(Service& service) {
    auto phone = readUntilValidOrCancel("Введите телефон в формате +7XXXXXXXXXX (например, +77001234567)",
        [](const std::string& value) { return validation.isValidPhone(value); },
        "Некорректный телефон. Используйте формат +7XXXXXXXXXX или введите 0 для отмены.\n");
    if (!phone) return std::nullopt;

    auto loginResp = service.loginPatient(*phone);
    if (loginResp.status == LoginPatientResult::Ok) {
        std::cout << "Добро пожаловать, " << loginResp.patient->getFio() << "!\n";
        return loginResp.patient->getId();
    }
    if (loginResp.status == LoginPatientResult::DatabaseError) {
        std::cout << "Не удалось выполнить вход: ошибка базы данных.\n";
        return std::nullopt;
    }

    auto answer = readCancellableLine(
        "Пациент с таким телефоном не найден. Зарегистрировать нового? (1 — да, 2 — нет)");
    if (!answer || *answer != "1") {
        if (answer && *answer != "2") std::cout << "Регистрация отменена. Для подтверждения используйте 1, для отказа — 2.\n";
        return std::nullopt;
    }

    auto fio = readUntilValidOrCancel("ФИО (например, Иванов Иван Иванович)", [](const std::string& value) {
        return validation.isValidFio(value);
        }, "Некорректное ФИО. Повторите ввод или введите 0 для отмены.\n");
    if (!fio) return std::nullopt;

    auto regResult = service.registerPatient(*fio, *phone);
    printResult(regResult);
    if (regResult != RegisterPatientResult::Ok) return std::nullopt;

    auto newLogin = service.loginPatient(*phone);
    if (newLogin.status == LoginPatientResult::Ok) return newLogin.patient->getId();
    std::cout << "Регистрация выполнена, но выполнить вход не удалось. Попробуйте войти снова.\n";
    return std::nullopt;
}

// Запускает меню пациента.
void runPatientMenu(Service& service) {
    auto patientIdOpt = loginOrRegisterPatient(service);
    if (!patientIdOpt) return;
    int patientId = *patientIdOpt;

    while (true) {
        std::cout << "\n--- Меню пациента ---\n"
            << "1. Просмотр свободных талонов\n"
            << "2. Записаться на талон\n"
            << "3. Отменить свою запись\n"
            << "4. Просмотр своих талонов\n"
            << "5. Просмотр свободных талонов конкретного врача\n"
            << "0. Выход\n";
        std::string choice = readLine("Выберите пункт меню (номер от 0 до 5; 0 — выход): ");
        // Дополнительно очищаем номер пункта меню перед сравнением.
        choice = trimWhitespace(choice);

        if (choice == "1") {
            auto speciality = readCancellableLine("Специальность (например, Кардиология; Enter — пропустить)");
            if (!speciality) continue;
            auto doctorFio = readCancellableLine("ФИО врача (например, Иванов Иван Иванович; Enter — пропустить)");
            if (!doctorFio) continue;
            auto date = readCancellableLine("Дата ГГГГ-ММ-ДД (например, 2026-10-20; Enter — пропустить)");
            if (!date) continue;
            auto resp = service.getFreeSlots(*speciality, *doctorFio, *date);
            if (resp.status == FreeSlotsResult::Ok) printFreeSlots(resp.tickets);
            else std::cout << "Не удалось получить свободные талоны. Проверьте фильтры и формат даты.\n";
        }
        if (choice == "1") {
            auto doctors = service.getDoctors();
            if (!doctors.empty()) {
                std::cout << "Список врачей (для точного ввода ФИО в фильтре ниже):\n";
                printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            }

            auto speciality = readCancellableLine("Специальность (например, Кардиология; Enter — пропустить)");
            if (!speciality) continue;
            auto doctorFio = readCancellableLine("ФИО врача (скопируйте из списка выше, например Иванов И. И.; Enter — пропустить)");
            if (!doctorFio) continue;
            auto date = readCancellableLine("Дата ГГГГ-ММ-ДД (например, 2026-10-20; Enter — пропустить)");
            if (!date) continue;
            auto resp = service.getFreeSlots(*speciality, *doctorFio, *date);
            if (resp.status == FreeSlotsResult::Ok) printFreeSlots(resp.tickets);
            else std::cout << "Не удалось получить свободные талоны. Проверьте фильтры и формат даты.\n";
        }
        else if (choice == "3") {
            auto myTickets = service.getPatientTickets(patientId);
            printTable(TICKET_HEADERS, ticketsToRows(myTickets));
            if (myTickets.empty()) {
                std::cout << "У вас нет записей для отмены.\n";
                continue;
            }
            auto ticketId = readUntilValidOrCancel("ID талона для отмены (целое число из списка ваших записей)", [&](const std::string& value) {
                return ticketIdExistsInList(myTickets, value);
                }, "Ваш талон с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!ticketId) continue;
            printResult(service.cancel(*ticketId, patientId));
        }
        else if (choice == "4") {
            auto myTickets = service.getPatientTickets(patientId);
            if (myTickets.empty()) std::cout << "У вас пока нет записей.\n";
            else printTable(TICKET_HEADERS, ticketsToRows(myTickets));
        }
        else if (choice == "5") {
            auto doctors = service.getDoctors();
            printTable({ "ID", "ФИО", "Специальность", "Кабинет" }, doctorsToRows(doctors));
            if (doctors.empty()) {
                std::cout << "Список врачей пуст.\n";
                continue;
            }
            auto doctorId = readUntilValidOrCancel("ID врача (целое число из списка)", [&](const std::string& value) {
                return doctorIdExists(doctors, value);
                }, "Врач с таким ID не найден. Введите ID из списка или 0 для отмены.\n");
            if (!doctorId) continue;
            auto date = readUntilValidOrCancel("Дата (формат ГГГГ-ММ-ДД, например 2026-10-20)",
                [](const std::string& value) { return validation.isValidDate(value); },
                "Некорректная дата. Используйте формат ГГГГ-ММ-ДД или 0 для отмены.\n");
            if (!date) continue;
            auto resp = service.getFreeSlotsByDoctor(*doctorId, *date);
            if (resp.status == FreeSlotsResult::Ok) printFreeSlots(resp.tickets);
            else std::cout << "Не удалось получить свободные талоны врача. Проверьте ID и дату.\n";
        }
        else if (choice == "0") {
            break;
        }
        else {
            std::cout << "Неизвестный пункт меню. Выберите число от 0 до 5.\n";
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

            std::string choice = readUntilValid("Выберите роль (1 — администратор, 2 — пациент, 0 — выход): ",
                [](const std::string& s) { return s == "0" || s == "1" || s == "2"; },
                "Введите 1, 2 или 0.\n");

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
