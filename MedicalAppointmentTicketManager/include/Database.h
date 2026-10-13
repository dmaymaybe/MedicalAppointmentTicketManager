#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <string>
#include <vector>
#include "Doctor.h"
#include "Patient.h"
#include "Ticket.h"



// Запись из таблицы истории обращений
struct HistoryRecord
{
	int id = 0;
	int ticket_id = 0;
	int patient_id = 0;
	std::string action_time;
	std::string action_type;
};



// Данные для таблицы с ФИО врача и пациента вместо их ID
// Для записи и отмены используется обычный Ticket
struct TicketView
{
	int id = 0;
	int doctorId = 0;
	std::string doctorFio;
	std::string speciality;
	std::string date;
	std::string time;
	std::optional<int> patientId;
	std::optional<std::string> patientFio; // nullopt - талон свободен

	bool isFree() const { return !patientId.has_value(); }
};



class Database
{
public:
	explicit Database(const std::string& path);
	
	std::vector<Doctor> listDoctors() const;
	std::optional<Doctor> findDoctorById(int id) const;
	int addDoctor(const Doctor& d); // Возвращает новый id
	void removeDoctor(int id);
	bool hasActiveTickets(int doctor_id) const;

	int addPatient(const Patient& p); // Возвращает новый id
	std::optional<Patient> findPatientByPhone(const std::string& phone) const;
	std::optional<Patient> findPatientById(int id) const;
	std::vector<Patient> listPatients() const;

	std::vector<TicketView> findFreeSlots(int doctor_id, const std::string& date) const;
	bool bookSlot(int ticket_id, int patient_id);
	bool cancelBooking(int ticket_id, int patient_id);
	std::vector<TicketView> listPatientTickets(int patient_id) const;
	std::optional<Ticket> findTicketById(int id) const;
	bool hasTicketWithDoctorOnDate(int patient_id, int doctor_id, const std::string& date) const;
	int addTicket(const Ticket& t);
	std::vector<TicketView> listAllTickets() const;
	std::vector<TicketView> findFreeSlotsByDoctor(int doctor_id) const;

	std::vector<HistoryRecord> listHistory() const;
	void logHistory(int ticket_id, int patient_id, const std::string& action);

private:
	SQLite::Database db_;
	void initSchema();
	Ticket ticketFromRow(SQLite::Statement& q) const;
	TicketView ticketViewFromRow(SQLite::Statement& q) const;
};
