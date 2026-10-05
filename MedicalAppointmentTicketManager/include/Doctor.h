#pragma once
#include <string>

class Doctor
{
public:
	Doctor(const std::string& d_fio, const std::string& d_speciality, int d_office, int d_id = -1);
	
	int getId() const;
	const std::string& getFio() const;
	const std::string& getSpeciality() const;
	int getOffice() const;

	void setId(int d_id);
private:
	int id;
	std::string fio;
	std::string speciality;
	int office;
};

