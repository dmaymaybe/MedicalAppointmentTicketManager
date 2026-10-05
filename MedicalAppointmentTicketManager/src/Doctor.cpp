#include "Doctor.h"

Doctor::Doctor(const std::string& d_fio, const std::string& d_speciality, int d_office, int d_id) 
	: id(d_id), fio(d_fio), speciality(d_speciality), office(d_office) {}

int Doctor::getId() const {
	return id;
}

const std::string& Doctor::getFio() const {
	return fio;
}

const std::string& Doctor::getSpeciality() const {
	return speciality;
}

int Doctor::getOffice() const {
	return office;
}

void Doctor::setId(int d_id) {
	id = d_id;
}