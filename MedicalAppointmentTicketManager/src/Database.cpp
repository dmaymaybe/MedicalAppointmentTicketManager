#include "Database.h"



// Проверка записан ли пациент на талон
bool TicketView::isFree() const {
    return !patientId.has_value();
}



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
void Database::removeDoctor(int id) {
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



// Общие JOIN для запросов талонов: врач подключается
// через JOIN (у талона всегда есть врач), пациент - через LEFT JOIN,
// так как у свободного талона patient_id может быть NULL
static const char* TICKET_VIEW_SELECT =
    "SELECT t.id, t.doctor_id, d.fio AS doctor_fio, d.speciality, "
    "t.date, t.time, t.patient_id, p.fio AS patient_fio "
    "FROM tickets t "
    "JOIN doctors d ON t.doctor_id = d.id "
    "LEFT JOIN patients p ON t.patient_id = p.id ";



// Поиск свободных талонов конкретного врача на конкретную дату
std::vector<TicketView> Database::findFreeSlots(int doctor_id, const std::string& date) const {
    SQLite::Statement q(db_, std::string(TICKET_VIEW_SELECT) +
        "WHERE t.doctor_id = ? AND t.date = ? AND t.patient_id IS NULL ORDER BY t.time");
    q.bind(1, doctor_id);
    q.bind(2, date);

    std::vector<TicketView> result;
    while (q.executeStep()) {
        result.push_back(ticketViewFromRow(q));
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



// Список записей пациента (с ФИО врача, специальностью и т.д. для вывода в таблицу)
std::vector<TicketView> Database::listPatientTickets(int patient_id) const {
    SQLite::Statement q(db_, std::string(TICKET_VIEW_SELECT) + "WHERE t.patient_id = ? ORDER BY t.date, t.time");
    q.bind(1, patient_id);

    std::vector<TicketView> result;
    while (q.executeStep()) {
        result.push_back(ticketViewFromRow(q));
    }
    return result;
}


// Поиск талона по ID (для бизнес-логики book/cancel)
std::optional<Ticket> Database::findTicketById(int id) const {
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



// Получение списка всех талонов (с ФИО врача, специальностью и ФИО пациента)
std::vector<TicketView> Database::listAllTickets() const {
    SQLite::Statement q(db_, std::string(TICKET_VIEW_SELECT) + "ORDER BY t.date, t.time");

    std::vector<TicketView> result;
    while (q.executeStep()) {
        result.push_back(ticketViewFromRow(q));
    }
    return result;
}



// Все свободные талоны врача, начиная с текущей даты
std::vector<TicketView> Database::findFreeSlotsByDoctor(int doctor_id) const {
    SQLite::Statement q(db_, std::string(TICKET_VIEW_SELECT) +
        "WHERE t.doctor_id = ? AND t.patient_id IS NULL AND t.date >= date('now','localtime') "
        "ORDER BY t.date, t.time");
    q.bind(1, doctor_id);

    std::vector<TicketView> result;
    while (q.executeStep()) {
        result.push_back(ticketViewFromRow(q));
    }
    return result;
}   



// Получение списка истории обращений
std::vector<HistoryRecord> Database::listHistory() const {
    SQLite::Statement q(db_, "SELECT h.id, h.ticket_id, p.fio, h.action_time, h.action_type FROM history h JOIN patients p ON h.patient_id = p.id ORDER BY h.action_time, h.id");
    std::vector<HistoryRecord> result;
    while (q.executeStep()) {
        HistoryRecord r;
        r.id = q.getColumn("id").getInt();
        r.ticket_id = q.getColumn("ticket_id").getInt();
        r.fio = q.getColumn("fio").getString();
        r.action_time = q.getColumn("action_time").getString();
        r.action_type = q.getColumn("action_type").getString();
        result.push_back(r);
    }
    return result;
}



// Логирование действий в таблицу history
void Database::logHistory(int ticket_id, int patient_id, const std::string& action) {
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



// Сборка обычного Ticket из текущей строки выборки для бизнес-логики
Ticket Database::ticketFromRow(SQLite::Statement& q) const {
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



// Сборка TicketView (с ФИО врача/пациента) из строки JOIN-запроса
TicketView Database::ticketViewFromRow(SQLite::Statement& q) const {
    TicketView v;
    v.id = q.getColumn("id").getInt();
    v.doctorId = q.getColumn("doctor_id").getInt();
    v.doctorFio = q.getColumn("doctor_fio").getString();
    v.speciality = q.getColumn("speciality").getString();
    v.date = q.getColumn("date").getString();
    v.time = q.getColumn("time").getString();
    if (!q.getColumn("patient_id").isNull()) {
        v.patientId = q.getColumn("patient_id").getInt();
        v.patientFio = q.getColumn("patient_fio").getString();
    }
    return v;
}
