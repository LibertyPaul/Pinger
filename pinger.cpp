#include "pinger.hpp"
#include <thread>
#include <cstring>
#include <algorithm>
#include <utility>
#include <cstdlib>

#ifdef __linux__
    #include <unistd.h>
#endif

#ifdef Q_OS_WIN
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>
    #include <windns.h>
#endif

using namespace std;

std::mutex Pinger::sysCallMutex;

Pinger::Pinger(const std::string &host): host(host){
}

Pinger::~Pinger(){
}


#ifdef __linux__
std::pair<bool, double> Pinger::extractPingTime_ms(const string &pingResult){
	//ordinary output of ping is: 64 bytes from xx.xx.xx.xx: icmp_seq=1 ttl=54 time=89.2 ms
	//we need to get '89.2' substring, turn it to double d = 89.2 and finally to uint32_t = 89
	const string before = "time=";
	const string after = " ms";

	size_t startPos = pingResult.find(before);
	size_t endPos = pingResult.find(after);
	if(startPos == string::npos || endPos == string::npos)
		return std::make_pair(false, 0);

	startPos += before.length();

	string result_s = pingResult.substr(startPos, endPos - startPos);
	//костыль заменяющий '.' на ',' из за того, что stod не конвертирует число с '.'
	std::replace(result_s.begin(), result_s.end(), '.', ',');

	double result = stod(result_s);
	return std::make_pair(true, result);

}

std::vector<double> Pinger::runPingProcessInstance(const uint16_t requestCount, const double delay){
    this->progressMutex.lock();
	this->progress = 0;
    this->progressMutex.unlock();

	/*
	if(delay < 0.2){
		delay = 0.2;//on UNIX only root can run ping with delay less than 0.2
	}
	*/

	string interval = to_string(delay);
	replace(interval.begin(), interval.end(), ',', '.');
	string command =  "ping -c " + to_string(requestCount + this->skip) + " -i " + interval + " " + this->host;
	command += " 2>&1";//redirect stderr to stdout

    FILE *ping_descriptor = popen(command.c_str(), "r");

	if(ping_descriptor == nullptr){
		lock_guard<mutex> lg(this->sysCallMutex);
		throw runtime_error(strerror(errno));
	}


	const uint32_t bufferSize = 1000;
	unique_ptr<char> buffer(new char[bufferSize]);
	vector<double> lags;

	int32_t skipCounter = this->skip;//пропускаем первый результат
	double progressStep = 1.0 / (requestCount + skipCounter);
	do{
		if(fgets(buffer.get(), bufferSize, ping_descriptor) != buffer.get() && ferror(ping_descriptor) != 0){
			lock_guard<mutex> lg(this->sysCallMutex);
			throw runtime_error(strerror(errno));
		}


		if(this->stopFlag)
			break;

		string currentLine(buffer.get());
		if(currentLine.find("ping: ") == 0)//ping notified us about an error
			throw std::runtime_error(currentLine.substr(6));

		std::pair<bool, double> lagTime = extractPingTime_ms(currentLine);

        if(lagTime.first){//если в строке было время - записываем его и посылаем сигнал
            this->progressMutex.lock();
			if(this->progress + progressStep <= 1.0){//если шагов больше чем requestCount - не переполняем счетчик
				this->progress += progressStep;
			}
            this->progressMutex.unlock();

            //skip, if required
			if(skipCounter --> 0)//brand new '-->' operator: "Goes to ..."
				continue;

			lags.push_back(lagTime.second);
		}
	}while(feof(ping_descriptor) == 0 && this->stopFlag == false);


	this->sysCallMutex.lock();
    int status = pclose(ping_descriptor);
	int exitStatus = WEXITSTATUS(status);
	this->sysCallMutex.unlock();
	if(exitStatus != 0){
		throw std::runtime_error("Ping failed");
    }

    if(this->stopFlag == false){
        this->progressMutex.lock();
		this->progress = 1.0;
        this->progressMutex.unlock();
    }

	return lags;
}
#endif

#ifdef Q_OS_WIN
std::vector<double> Pinger::runPingProcessInstance(const uint16_t requestCount, const double delay){
    std::vector<double> result;
    this->progressMutex.lock();
    this->progress = 0;
    this->progressMutex.unlock();
    double progressStep = 1.0 / requestCount;

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0)
        throw std::runtime_error("WSAStartup failed");

    PDNS_RECORD dnsResultPtr;
    DNS_STATUS dnsStatus = DnsQuery_UTF8(this->host.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD, nullptr, &dnsResultPtr, nullptr);
    if(dnsStatus != DNS_RCODE_NOERROR)
        throw std::runtime_error("DnsQuery error");

    IP4_ADDRESS ip = dnsResultPtr->Data.A.IpAddress;

    HANDLE hIcmp = IcmpCreateFile();
    if(hIcmp == INVALID_HANDLE_VALUE)
        throw std::runtime_error("IcmpCreateFile error");

    char SendData[32];
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = reinterpret_cast<LPVOID>(malloc(ReplySize));
    if(ReplyBuffer == nullptr)
        throw std::runtime_error("malloc error");

    std::chrono::duration<int, std::milli> pingDelay(static_cast<int>(delay * 1000));

    for(uint32_t i = 0; i < requestCount && this->stopFlag == false; ++i){
        auto lastTime = std::chrono::high_resolution_clock::now();

        DWORD resIcmp = IcmpSendEcho(hIcmp, ip, SendData, sizeof(SendData), nullptr, ReplyBuffer, ReplySize, 1000);
        if(resIcmp == 0)
            throw std::runtime_error("IcmpSendEcho error");

        PICMP_ECHO_REPLY pReply = reinterpret_cast<PICMP_ECHO_REPLY>(ReplyBuffer);

        result.push_back(pReply->RoundTripTime);

        this->progressMutex.lock();
        if(this->progress + progressStep <= 1.0){//если шагов больше чем requestCount - не переполняем счетчик
            this->progress += progressStep;
        }
        this->progressMutex.unlock();

        if(i < requestCount)
            std::this_thread::sleep_until(lastTime + pingDelay);
    }

    if(this->stopFlag == false){
        this->progressMutex.lock();
        this->progress = 1.0;
        this->progressMutex.unlock();
    }

    return result;
}
#endif




void Pinger::run(const uint16_t requestCount, const double delay) noexcept{
	try{
		std::lock_guard<mutex> lg(this->runInstanceMutex);

		this->stopFlag = false;
		this->readyFlag = false;

		this->result = std::move(this->runPingProcessInstance(requestCount, delay));

		this->readyFlag = true;

	}catch(std::runtime_error &re){
		this->exception = std::make_shared<runtime_error>(re);
	}catch(...){
		this->exception = std::make_shared<runtime_error>(std::runtime_error("Unknown exception"));
	}
}

void Pinger::stop(){
	this->stopFlag = true;
}

double Pinger::getProgress() const noexcept{
    std::lock_guard<mutex> lg(this->progressMutex);
    return this->progress;
}

bool Pinger::isReady() const noexcept{
	return this->readyFlag;
}

std::vector<double> Pinger::getResult() const{
	return this->result;
}

std::shared_ptr<std::runtime_error> Pinger::getException() const{
	return this->exception;
}











