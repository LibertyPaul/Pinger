#include "pinger.hpp"
#include <thread>
#include <cstring>
#include <algorithm>

#ifdef __linux__
    #include <unistd.h>
#endif

#ifdef Q_OS_WIN
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>
#endif

using namespace std;

std::mutex Pinger::sysCallMutex;

Pinger::Pinger(const string &host, const uint16_t requestCount, const double delay):
	host(host),
	requestCount(requestCount),
    delay(delay),
	progress(0){

#ifdef __linux__
	this->sysCallMutex.lock();
	int uid = getuid();
	this->sysCallMutex.unlock();
	if(delay < 0.2 && uid != 0){
		throw logic_error("dalay less than 0.2 seconds is only allowed to super user");
	}
#endif

}

Pinger::~Pinger(){
}


#ifdef __linux__
string Pinger::createCommand() const{
    string interval = to_string(this->delay);
	replace(interval.begin(), interval.end(), ',', '.');
	return "ping -c " + to_string(this->requestCount + this->skip) + " -i " + interval + " " + this->host;
}


double Pinger::extractPingTime_ms(const string &pingResult){
	//ordinary output of ping is: 64 bytes from xx.xx.xx.xx: icmp_seq=1 ttl=54 time=89.2 ms
	//we need to get '89.2' substring, turn it to double d = 89.2 and finally to uint32_t = 89
	const string before = "time=";
	const string after = " ms";

	size_t startPos = pingResult.find(before);
	size_t endPos = pingResult.find(after);
	if(startPos == string::npos || endPos == string::npos)
		return -1;

	startPos += before.length();

	string result_s = pingResult.substr(startPos, endPos - startPos);
	//костыль заменяющий '.' на ',' из за того, что stod не конвертирует число с '.'
	std::replace(result_s.begin(), result_s.end(), '.', ',');

	double result = stod(result_s);
	return result;
}

std::vector<double> Pinger::runPingProcessInstance(){
	this->progress = 0;

	string command = this->createCommand();

    FILE *ping_descriptor = popen(command.c_str(), "r");

	if(ping_descriptor == nullptr){
		lock_guard<mutex> lg(this->sysCallMutex);
		throw runtime_error(strerror(errno));
	}


	const uint32_t bufferSize = 1000;
	unique_ptr<char> buffer(new char[bufferSize]);
	vector<double> lags;

	uint32_t skipCounter = this->skip;//пропускаем первый результат
	double progressStep = 1.0 / (this->requestCount + skipCounter);
	do{
		if(fgets(buffer.get(), bufferSize, ping_descriptor) != buffer.get() && ferror(ping_descriptor) != 0){
			lock_guard<mutex> lg(this->sysCallMutex);
			throw runtime_error(strerror(errno));
		}
		if(skipCounter > 0){
			--skipCounter;
			continue;
		}


		if(this->stopFlag)
			break;

		string currentLine(buffer.get());
		double lagTime = extractPingTime_ms(currentLine);

		if(lagTime != -1){//если в строке было время - записываем его и посылаем сигнал
			if(this->progress + progressStep <= 1.0){//если шагов больше чем requestCount - не переполняем счетчик
				this->progress.store(this->progress + progressStep);
			}

			lags.push_back(lagTime);
		}
	}while(feof(ping_descriptor) == 0 && this->stopFlag == false);


	this->sysCallMutex.lock();
    int status = pclose(ping_descriptor);
	int exitStatus = WEXITSTATUS(status);
	this->sysCallMutex.unlock();
	if(exitStatus != 0){
		throw std::runtime_error("Ping failed");
    }

	if(this->stopFlag == false)
		this->progress = 1.0;

	return lags;
}
#endif

#ifdef Q_OS_WIN
std::vector<double> Pinger::runPingProcessInstance(){

    std::vector<double> result;
    this->progress = 0;
    double progressStep = 1.0 / this->requestCount;

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0)
        throw std::runtime_error("WSAStartup failed");


    addrinfo *addr;
    int res = getaddrinfo(this->host.c_str(), nullptr, nullptr, &addr);
    if(res != 0)
        throw std::runtime_error("getaddrinfo error");

    char ip_s[128];
    res = getnameinfo(addr->ai_addr, sizeof(*addr->ai_addr), ip_s, 256, nullptr, 0, 0);
    if(res != 0)
        throw std::runtime_error("getnameinfo error");

    UINT ip = inet_addr(ip_s);


    HANDLE hIcmp = IcmpCreateFile();
    if(hIcmp == INVALID_HANDLE_VALUE)
        throw std::runtime_error("IcmpCreateFile error");

    char SendData[32];
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = reinterpret_cast<LPVOID>(malloc(ReplySize));
    if(ReplyBuffer == nullptr)
        throw std::runtime_error("malloc error");

    std::chrono::duration<int, std::milli> pingDelay(static_cast<int>(this->delay * 1000));

	for(int i = 0; i < this->requestCount && this->stopFlag == false; ++i){
        auto lastTime = std::chrono::high_resolution_clock::now();

        DWORD resIcmp = IcmpSendEcho(hIcmp, ip, SendData, sizeof(SendData), nullptr, ReplyBuffer, ReplySize, 1000);
        if(resIcmp == 0)
            throw std::runtime_error("IcmpSendEcho error");

        PICMP_ECHO_REPLY pReply = reinterpret_cast<PICMP_ECHO_REPLY>(ReplyBuffer);

        result.push_back(pReply->RoundTripTime);

        if(this->progress + progressStep <= 1.0){//если шагов больше чем requestCount - не переполняем счетчик
            this->progress += progressStep;
        }

        if(i < this->requestCount)
            std::this_thread::sleep_until(lastTime + pingDelay);
    }

    return result;
}
#endif




void Pinger::run() noexcept{
	try{
		std::lock_guard<mutex> lg(this->runInstanceMutex);

		this->stopFlag = false;
		this->readyFlag = false;

		this->result = std::move(this->runPingProcessInstance());

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











