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



class Database
{
public:
	explicit Database(const std::string& path);
	
	std::vector<Doctor> listDoctors() const;
	std::optional<Doctor> findDoctorById(int id) const;
	int addDoctor(const Doctor& d);              // Возвращает новый id
	void removeDoctor(int id) const;
	bool hasActiveTickets(int doctor_id) const;

	int addPatient(const Patient& p);             // Возвращает новый id
	std::optional<Patient> findPatientByPhone(const std::string& phone) const;
	std::optional<Patient> findPatientById(int id) const;
	std::vector<Patient> listPatients() const;

	std::vector<Ticket> findFreeSlots(int doctor_id, const std::string& date);
	bool bookSlot(int ticket_id, int patient_id);
	bool cancelBooking(int ticket_id, int patient_id);
	std::vector<Ticket> listPatientTickets(int patient_id);
	std::optional<Ticket> findTicketById(int id);
	bool hasTicketWithDoctorOnDate(int patient_id, int doctor_id, const std::string& date) const;
	int addTicket(const Ticket& t);
	std::vector<Ticket> listAllTickets();
	std::vector<Ticket> findFreeSlotsByDoctor(int doctor_id);

	std::vector<HistoryRecord> listHistory() const;
	void logHistory(int ticket_id, int patient_id, const std::string& action) const;

private:
	SQLite::Database db_;
	void initSchema();
	Ticket ticketFromRow(SQLite::Statement& q);
};

