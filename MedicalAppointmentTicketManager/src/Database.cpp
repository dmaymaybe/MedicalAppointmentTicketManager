#include "Database.h"



Database::Database(const std::string& path) : db_(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
	initSchema();
}



// Получение списка всех врачей
std::vector<Doctor> Database::listDoctors() const {
    SQLite::Statement q(db_, "SELECT id, fio, speciality, office FROM doctors ORDER BY fio");
    std::vector<Doctor> result;
    while (q.executeStep()) {
        Doctor d(
            q.getColumn("fio").getString(),
            q.getColumn("speciality").getString(),
            q.getColumn("office").getInt(),
            q.getColumn("id").getInt()
        );
        result.push_back(d);
    }
    return result;
}



// Поиск врача по ID
std::optional<Doctor> Database::findDoctorById(int id) const {
    SQLite::Statement q(db_, "SELECT id, fio, speciality, office FROM doctors WHERE id = ?");
    q.bind(1, id);
    if (q.executeStep()) {
        return Doctor(
            q.getColumn("fio").getString(),
            q.getColumn("speciality").getString(),
            q.getColumn("office").getInt(),
            q.getColumn("id").getInt()
        );
    }
    return std::nullopt;
}



// Добавление нового врача
int Database::addDoctor(const Doctor& d) {
	SQLite::Statement q(db_, "INSERT INTO doctors (fio, speciality, office) VALUES (?, ?, ?)");

	q.bind(1, d.getFio());
    q.bind(2, d.getSpeciality());
	q.bind(3, d.getOffice());

	q.exec();

	return static_cast<int>(db_.getLastInsertRowid());
}



// Удаление врача
void Database::removeDoctor(int id) const {
    SQLite::Statement q(db_, "DELETE FROM doctors WHERE id = ?");
    q.bind(1, id);
    q.exec();
}



// Проверка занятости врача
bool Database::hasActiveTickets(int doctor_id) const {
    SQLite::Statement q(db_, "SELECT 1 FROM tickets WHERE doctor_id = ?");
    q.bind(1, doctor_id);

    return static_cast<bool>(q.executeStep());
}



// Добавление нового пациента
int Database::addPatient(const Patient& p) {
    SQLite::Statement q(db_, "INSERT INTO patients (fio, phone) VALUES (?, ?)");
    q.bind(1, p.getFio());
    q.bind(2, p.getPhone());
    q.exec();
    return static_cast<int>(db_.getLastInsertRowid());
}



// Поиск пациента по телефону
std::optional<Patient> Database::findPatientByPhone(const std::string& phone) const {
    SQLite::Statement q(db_, "SELECT id, fio, phone FROM patients WHERE phone = ?");
    q.bind(1, phone);
    if (q.executeStep()) {
        return Patient(
            q.getColumn("fio").getString(),
            q.getColumn("phone").getString(),
            q.getColumn("id").getInt()
        );
    }
    return std::nullopt;
}



// Поиск пациента по ID
std::optional<Patient> Database::findPatientById(int id) const {
    SQLite::Statement q(db_, "SELECT id, fio, phone FROM patients WHERE id = ?");
    q.bind(1, id);
    if (q.executeStep()) {
        return Patient(
            q.getColumn("fio").getString(),
            q.getColumn("phone").getString(),
            q.getColumn("id").getInt()
        );
    }
    return std::nullopt;
}



// Получение списка всех пациентов
std::vector<Patient> Database::listPatients() const {
    SQLite::Statement q(db_, "SELECT id, fio, phone FROM patients ORDER BY fio");
    std::vector<Patient> result;
    while (q.executeStep()) {
        result.emplace_back(
            q.getColumn("fio").getString(),
            q.getColumn("phone").getString(),
            q.getColumn("id").getInt()
        );
    }
    return result;
}



// Поиск свободных талонов
std::vector<Ticket> Database::findFreeSlots(int doctor_id, const std::string& date) {
    SQLite::Statement q(db_, "SELECT id, doctor_id, date, time, patient_id FROM tickets WHERE doctor_id = ? AND date = ? AND patient_id IS NULL ORDER BY time");
    q.bind(1, doctor_id);
    q.bind(2, date);

    std::vector<Ticket> result;
    while (q.executeStep()) {
        result.push_back(ticketFromRow(q));
    }
    return result;
}



// Бронирование приема
bool Database::bookSlot(int ticket_id, int patient_id) {
    SQLite::Transaction transaction(db_);

    SQLite::Statement q(db_, "UPDATE tickets SET patient_id = ? WHERE id = ? AND patient_id IS NULL");
    q.bind(1, patient_id);
    q.bind(2, ticket_id);

    if (q.exec() == 0) {
        return false;
    }
    logHistory(ticket_id, patient_id, "запись");
    transaction.commit();
    return true;
}



// Отмена брони
bool Database::cancelBooking(int ticket_id, int patient_id) {
    SQLite::Transaction transaction(db_);

    SQLite::Statement q(db_, "UPDATE tickets SET patient_id = NULL WHERE id = ? AND patient_id = ?");
    q.bind(1, ticket_id);
    q.bind(2, patient_id);

    if (q.exec() == 0) {
        return false;
    }
    logHistory(ticket_id, patient_id, "отмена");
    transaction.commit();
    return true;
}



// Список записей пациента
std::vector<Ticket> Database::listPatientTickets(int patient_id) {
    SQLite::Statement q(db_, "SELECT id, doctor_id, date, time, patient_id FROM tickets WHERE patient_id = ? ORDER BY date, time");
    q.bind(1, patient_id);

    std::vector<Ticket> result;
    while (q.executeStep()) {
        result.push_back(ticketFromRow(q));
    }
    return result;
}


// Поиск талона по ID
std::optional<Ticket> Database::findTicketById(int id) {
    SQLite::Statement q(db_, "SELECT id, doctor_id, date, time, patient_id FROM tickets WHERE id = ?");
    q.bind(1, id);

    if (q.executeStep()) {
        return ticketFromRow(q);
    }
    return std::nullopt;
}



// Проверка повторной записи
bool Database::hasTicketWithDoctorOnDate(int patient_id, int doctor_id, const std::string& date) const {
    SQLite::Statement q(db_, "SELECT 1 FROM tickets WHERE patient_id = ? AND doctor_id = ? AND date = ?");
    q.bind(1, patient_id);
    q.bind(2, doctor_id);
    q.bind(3, date);

    return q.executeStep();
}



// Создание нового талона
int Database::addTicket(const Ticket& t) {
    SQLite::Statement q(db_, "INSERT INTO tickets (doctor_id, date, time) VALUES (?, ?, ?)");
    q.bind(1, t.getDoctorId());
    q.bind(2, t.getDate());
    q.bind(3, t.getTime());
    q.exec();

    return static_cast<int>(db_.getLastInsertRowid());
}



// Получение списка всех талонов
std::vector<Ticket> Database::listAllTickets() {
    SQLite::Statement q(db_, "SELECT id, doctor_id, date, time, patient_id FROM tickets ORDER BY date, time");
    std::vector<Ticket> result;
    while (q.executeStep()) {
        result.push_back(ticketFromRow(q));
    }
    return result;
}



// Все свободные талоны врача, начиная с текущей даты
std::vector<Ticket> Database::findFreeSlotsByDoctor(int doctor_id) {
    SQLite::Statement q(db_,
        "SELECT id, doctor_id, date, time, patient_id FROM tickets "
        "WHERE doctor_id = ? AND patient_id IS NULL AND date >= date('now','localtime') "
        "ORDER BY date, time");
    q.bind(1, doctor_id);

    std::vector<Ticket> result;
    while (q.executeStep()) {
        result.push_back(ticketFromRow(q));
    }
    return result;
}



// Получение списка истории обращений
std::vector<HistoryRecord> Database::listHistory() const {
    SQLite::Statement q(db_, "SELECT id, ticket_id, patient_id, action_time, action_type FROM history ORDER BY action_time, id");
    std::vector<HistoryRecord> result;
    while (q.executeStep()) {
        HistoryRecord r;
        r.id = q.getColumn("id").getInt();
        r.ticket_id = q.getColumn("ticket_id").getInt();
        r.patient_id = q.getColumn("patient_id").getInt();
        r.action_time = q.getColumn("action_time").getString();
        r.action_type = q.getColumn("action_type").getString();
        result.push_back(r);
    }
    return result;
}



// Логирование действий
void Database::logHistory(int ticket_id, int patient_id, const std::string& action) const {
    SQLite::Statement q(db_, "INSERT INTO history (ticket_id, patient_id, action_time, action_type) VALUES (?, ?, datetime('now', 'localtime'), ?)");
    q.bind(1, ticket_id);
    q.bind(2, patient_id);
    q.bind(3, action);
    q.exec();
}



// Инициализация схемы БД
void Database::initSchema() {
        db_.exec("PRAGMA foreign_keys = ON");

        db_.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS "doctors" (
	    "id" INTEGER PRIMARY KEY,
	    "fio"	TEXT NOT NULL,
	    "speciality"	TEXT NOT NULL,
	    "office"	INTEGER NOT NULL,
	    CHECK("office" > 0)
    )
    )SQL");

        db_.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS "patients" (
	    "id" INTEGER PRIMARY KEY,
	    "fio"	TEXT NOT NULL,
	    "phone"	TEXT NOT NULL UNIQUE
    )
    )SQL");

        db_.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS "tickets" (
	    "id" INTEGER PRIMARY KEY,
	    "doctor_id"	INTEGER NOT NULL,
	    "date"	TEXT NOT NULL,
	    "time"	TEXT NOT NULL,
	    "patient_id"	INTEGER,
	    FOREIGN KEY("doctor_id") REFERENCES "doctors"("id"),
	    FOREIGN KEY("patient_id") REFERENCES "patients"("id"),
        UNIQUE("doctor_id", "date", "time")
    )
    )SQL");

        db_.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS "history" (
	    "id" INTEGER PRIMARY KEY,
	    "ticket_id"	INTEGER NOT NULL,
	    "patient_id"	INTEGER NOT NULL,
	    "action_time"	TEXT NOT NULL,
	    "action_type"	TEXT NOT NULL CHECK("action_type" IN ('запись', 'отмена')),
	    FOREIGN KEY("patient_id") REFERENCES "patients"("id"),
	    FOREIGN KEY("ticket_id") REFERENCES "tickets"("id")
    )
    )SQL");
}



// Сборка объекта Ticket из текущей строки выборки
Ticket Database::ticketFromRow(SQLite::Statement& q) {
    std::optional<int> patientId;
    if (!q.getColumn("patient_id").isNull()) {
        patientId = q.getColumn("patient_id").getInt();
    }
    return Ticket(
        q.getColumn("doctor_id").getInt(),
        q.getColumn("date").getString(),
        q.getColumn("time").getString(),
        patientId,
        q.getColumn("id").getInt()
    );
}