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
	int id;
	int ticket_id;
	int patient_id;
	std::string action_time;
	std::string action_type;
};



class Database
{
public:
	explicit Database(const std::string& path);
	
	std::vector<Doctor> listDoctors();
	std::optional<Doctor> findDoctorById(int id);
	int addDoctor(const Doctor& d);              // Возвращает новый id
	void removeDoctor(int id);
	bool hasActiveTickets(int doctor_id);

	int addPatient(const Patient& p);             // Возвращает новый id
	std::optional<Patient> findPatientByPhone(const std::string& phone);
	std::optional<Patient> findPatientById(int id);
	std::vector<Patient> listPatients();

	std::vector<Ticket> findFreeSlots(int doctor_id, const std::string& date);
	bool bookSlot(int ticket_id, int patient_id);
	bool cancelBooking(int ticket_id, int patient_id);
	std::vector<Ticket> listPatientTickets(int patient_id);
	std::optional<Ticket> findTicketById(int id);
	bool hasTicketWithDoctorOnDate(int patient_id, int doctor_id, const std::string& date);
	int addTicket(const Ticket& t);
	std::vector<Ticket> listAllTickets();
	std::vector<Ticket> findFreeSlotsByDoctor(int doctor_id);

	std::vector<HistoryRecord> listHistory();
	void logHistory(int ticket_id, int patient_id, const std::string& action);

private:
	SQLite::Database db_;
	void initSchema();
	Ticket ticketFromRow(SQLite::Statement& q);
};

