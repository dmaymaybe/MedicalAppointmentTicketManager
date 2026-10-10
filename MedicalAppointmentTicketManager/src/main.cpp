#include <iostream>
#include <filesystem>
#include <Windows.h>
#include "Database.h"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::cout << std::boolalpha;   // bool выводится как true/false

    const std::string path = "data/test.db";
    std::filesystem::remove(path);   // чистая БД на каждый запуск

    try
    {
        Database db(path);

        // ---------- врач и талоны ----------
        Doctor doc("Иванов И.И.", "Терапевт", 101);
        doc.setId(db.addDoctor(doc));
        std::cout << "Врач добавлен, id = " << doc.getId() << "\n";

        Ticket t1(doc.getId(), "2026-12-01", "09:00");
        t1.setId(db.addTicket(t1));
        std::cout << "Талон добавлен, id = " << t1.getId() << "\n";

        // дубль: ожидаем исключение
        try
        {
            Ticket t2(doc.getId(), "2026-12-01", "09:00");
            db.addTicket(t2);
            std::cout << "ОШИБКА: дубль прошел без исключения\n";
        }
        catch (const SQLite::Exception& e)
        {
            std::cout << "Дубль отклонен, как и должно быть: " << e.what() << "\n";
        }

        // несуществующий врач: ожидаем исключение из-за внешнего ключа
        try
        {
            Ticket t3(999, "2026-12-01", "09:30");
            db.addTicket(t3);
            std::cout << "ОШИБКА: талон для несуществующего врача прошел\n";
        }
        catch (const SQLite::Exception& e)
        {
            std::cout << "Несуществующий врач отклонен: " << e.what() << "\n";
        }

        // ---------- пациент ----------
        Patient pat("Петров П.П.", "+79991234567");
        pat.setId(db.addPatient(pat));
        std::cout << "Пациент добавлен, id = " << pat.getId() << "\n";

        auto found = db.findPatientByPhone("+79991234567");
        std::cout << "Поиск по телефону: " << (found ? "найден" : "не найден") << "\n";   // найден

        // ---------- запись и отмена ----------
        std::cout << "Свободных слотов: "
            << db.findFreeSlots(doc.getId(), "2026-12-01").size() << "\n";           // 1

        std::cout << "Запись: " << db.bookSlot(t1.getId(), pat.getId()) << "\n";            // true
        std::cout << "Повторная запись: " << db.bookSlot(t1.getId(), pat.getId()) << "\n";  // false

        std::cout << "Свободных слотов: "
            << db.findFreeSlots(doc.getId(), "2026-12-01").size() << "\n";           // 0
        std::cout << "Талонов у пациента: "
            << db.listPatientTickets(pat.getId()).size() << "\n";                    // 1

        auto booked = db.findTicketById(t1.getId());
        std::cout << "Талон свободен: " << (booked && booked->isFree()) << "\n";            // false

        std::cout << "Запись к врачу в этот день: "
            << db.hasTicketWithDoctorOnDate(pat.getId(), doc.getId(), "2026-12-01") << "\n"; // true

        std::cout << "Отмена: " << db.cancelBooking(t1.getId(), pat.getId()) << "\n";           // true
        std::cout << "Повторная отмена: " << db.cancelBooking(t1.getId(), pat.getId()) << "\n"; // false

        // ---------- удаление врача ----------
        std::cout << "У врача есть талоны: " << db.hasActiveTickets(doc.getId()) << "\n";   // true

        std::cout << "Врачей в списке: " << db.listDoctors().size() << "\n";                // 1

        std::cout << "\nТест завершен. Проверь таблицу history в DB Browser: должно быть "
            "две строки, запись и отмена.\n";
    }
    catch (const SQLite::Exception& e)
    {
        std::cerr << "Ошибка SQLite: " << e.what() << "\n";
        return 1;
    }
    return 0;
}