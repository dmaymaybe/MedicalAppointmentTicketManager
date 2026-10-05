#pragma once
#include <string>

class Patient
{
public:
	Patient(const std::string& p_fio, const std::string& p_phone, int p_id = -1);

	int getId() const;
	const std::string& getFio() const;
	const std::string& getPhone() const;

	void setId(int p_id);
private:
	int id;
	std::string fio;
	std::string phone;
};

