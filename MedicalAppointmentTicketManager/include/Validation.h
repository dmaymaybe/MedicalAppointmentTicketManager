#pragma once
#include <string>


class Validation
{
public:
	bool isValidFio(const std::string& fio) const;
	bool isValidPhone(const std::string& phone) const;
	bool isValidDate(const std::string& date) const;
	bool isValidTime(const std::string& time) const;
	bool isValidOffice(int office) const;
};

