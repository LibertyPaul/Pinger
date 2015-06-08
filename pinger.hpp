#ifndef PINGER_HPP
#define PINGER_HPP

#include <QObject>
#include <mutex>
#include <vector>

typedef std::vector<double> PingResult;

class Pinger : public QObject{
	Q_OBJECT
protected:
	std::string host;
	uint32_t requestCount;
	uint32_t delay_microseconds;
	double progress;//0 --> 1
	mutable std::mutex pingerMutex;


	PingResult runPingProcessInstance();
	static double extractPingTime_ms(const std::string &PingResult);
	std::string createCommand() const;
public:
	Pinger(const std::string &host, const uint16_t requestCount, const uint32_t delay_microseconds);
	~Pinger();

	void setRequestCount(const uint32_t requestCount);
	void setDelay(const uint32_t delay_microseconds);

public slots:
	void run();

signals:
	void progressChanged(int value);
	void done(PingResult result);
	void pingAverageTime(double time_ms);
	void exception(std::string what);
};

#endif // PINGER_HPP
