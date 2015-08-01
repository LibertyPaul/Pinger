#ifndef PINGER_HPP
#define PINGER_HPP

#include <QVector>
#include <QWaitCondition>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <memory>


class Pinger{
protected:
	std::string host;
	uint32_t requestCount;
	static constexpr uint32_t skip = 5;
	double delay;
	std::atomic<bool> stopFlag, readyFlag;
	std::atomic<double> progress;
	static std::mutex sysCallMutex;
	std::mutex runInstanceMutex, accessPropertyMutex;
	std::vector<double> result;
	std::shared_ptr<std::runtime_error> exception;


    std::vector<double> runPingProcessInstance();
#ifdef __linux__
	static double extractPingTime_ms(const std::string &PingResult);
	std::string createCommand() const;
#endif

public:
	Pinger(const std::string &host, const uint16_t requestCount, const double delay);
	~Pinger();

	void run() noexcept;

	double getProgress() const noexcept;
	bool isReady() const noexcept;
	std::vector<double> getResult() const;
	std::shared_ptr<std::runtime_error> getException() const;

    void stop();
};

#endif // PINGER_HPP
