#include "Patient.h"

Patient::Patient(const std::string& p_fio, const std::string& p_phone, int p_id)
	: id(p_id), fio(p_fio), phone(p_phone) {}

int Patient::getId() const {
	return id;
}

const std::string& Patient::getFio() const {
	return fio;
}

const std::string& Patient::getPhone() const {
	return phone;
}

void Patient::setId(int p_id) {
	id = p_id;
}