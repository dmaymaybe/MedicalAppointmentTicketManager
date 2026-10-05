#include "Ticket.h"

Ticket::Ticket(int t_doctor_id, const std::string& t_date, const std::string& t_time, std::optional<int> t_patient_id, int t_id) 
	: id(t_id), doctor_id(t_doctor_id), date(t_date), time(t_time), patient_id(t_patient_id) {}

int Ticket::getId() const {
	return id;
}

int Ticket::getDoctorId() const {
	return doctor_id;
}

const std::string& Ticket::getDate() const {
	return date;
}

const std::string& Ticket::getTime() const {
	return time;
}

std::optional<int> Ticket::getPatientId() const {
	return patient_id;
}

void Ticket::setId(int t_id) {
	id = t_id;
}

bool Ticket::isFree() const {
	return !patient_id.has_value();
}

void Ticket::bookBy(int p_id) {
	patient_id = p_id;
}

void Ticket::cancelBooking() {
	patient_id = std::nullopt;
}