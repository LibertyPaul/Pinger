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
	static constexpr uint32_t skip = 5;
	std::atomic<bool> stopFlag, readyFlag;
    double progress;
	static std::mutex sysCallMutex;
    mutable std::mutex runInstanceMutex, accessPropertyMutex, progressMutex;
	std::vector<double> result;
	std::unique_ptr<std::runtime_error> exception;


	std::vector<double> runPingProcessInstance(const uint16_t requestCount, const double delay);
#ifdef __linux__
	static std::pair<bool, double> extractPingTime_ms(const std::string &PingResult);
	std::string createCommand() const;
#endif

public:
    Pinger(const std::string &host);
	~Pinger();

	void run(const uint16_t requestCount, const double delay) noexcept;

	double getProgress() const noexcept;
	bool isReady() const noexcept;
	std::vector<double> getResult() const;
	std::runtime_error *getLastException();

    void stop();
};

#endif // PINGER_HPP
