#pragma once
#include <string>
#include <optional>

class Ticket
{
public:
	Ticket(int t_doctor_id, const std::string& t_date, const std::string& t_time, std::optional<int> t_patient_id = std::nullopt, int t_id = -1);

	int getId() const;
	int getDoctorId() const;
	const std::string& getDate() const;
	const std::string& getTime() const;
	std::optional<int> getPatientId() const;

	void setId(int t_id);

	bool isFree() const;
	void bookBy(int t_id);
	void cancelBooking();
private:
	int id;
	int doctor_id;
	std::string date;
	std::string time;
	std::optional<int> patient_id;
};

