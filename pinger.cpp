#include "pinger.hpp"
#include <unistd.h>
#include <thread>
#include <stdexcept>
#include <cstring>
#include <algorithm>

using namespace std;

Pinger::Pinger(const string &host, const uint16_t requestCount, const uint32_t delay_microseconds):
	host(host),
	requestCount(requestCount),
	delay_microseconds(delay_microseconds),
	progress(0),
	pingerMutex(){

	if(delay_microseconds < 200000 && geteuid() != 0){
		throw logic_error("dalay less than 200000 microseconds is only allowed to super user");
	}

}

Pinger::~Pinger(){
}



void Pinger::setRequestCount(const uint32_t requestCount){
	this->requestCount = requestCount;
}

void Pinger::setDelay(const uint32_t delay_microseconds){
	this->delay_microseconds = delay_microseconds;
}

string Pinger::createCommand() const{
	lock_guard<mutex> lg(this->pingerMutex);
	string interval = to_string(static_cast<double>(this->delay_microseconds) / 1000000);
	replace(interval.begin(), interval.end(), ',', '.');
	return "ping -c " + to_string(this->requestCount) + " -i " + interval + " " + this->host;
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

	string result_s = pingResult.substr(startPos, endPos);
	return stod(result_s);
}

PingResult Pinger::runPingProcessInstance(){
	this->progress = 0;
	emit progressChanged(progress * 100);
	string command = this->createCommand();

	this->pingerMutex.lock();
	FILE *ping_descriptor = popen(command.c_str(), "r");
	this->pingerMutex.unlock();

	if(ping_descriptor == nullptr){
		lock_guard<mutex> lg(this->pingerMutex);
		throw runtime_error(strerror(errno));
	}


	const uint32_t bufferSize = 1000;
	unique_ptr<char> buffer(new char[bufferSize]);
	vector<std::string> output;
	vector<double> lags;


	double progressStep = 1.0 / this->requestCount;
	do{
		this->pingerMutex.lock();
		if(fgets(buffer.get(), bufferSize, ping_descriptor) != buffer.get() && ferror(ping_descriptor) != 0){
			this->pingerMutex.unlock();
			lock_guard<mutex> lg(this->pingerMutex);
			throw runtime_error(strerror(errno));
		}
		this->pingerMutex.unlock();

		string currentLine(buffer.get());
		output.push_back(currentLine);
		int32_t lagTime = extractPingTime_ms(currentLine);

		if(lagTime != -1){//если в строке было время - записываем его и посылаем сигнал
			lags.push_back(lagTime);
			if(progress + progressStep <= 1.0){//если шагов больше чем requestCount - не переполняем счетчик
				progress += progressStep;
				emit progressChanged(progress * 100);
			}
		}
	}while(feof(ping_descriptor) == 0);


	this->pingerMutex.lock();
	int status = pclose(ping_descriptor);
	int exitStatus = WEXITSTATUS(status);
	this->pingerMutex.unlock();

	if(exitStatus != 0){
		throw std::runtime_error("Ping failed");
	}

	progress = 1.0;
	emit progressChanged(progress * 100);

	double avg = std::accumulate(lags.cbegin(), lags.cend(), 0.0) / lags.size();
	emit pingAverageTime(avg);
	return lags;
}

void Pinger::run(){
	PingResult result;
	try{
		result = this->runPingProcessInstance();
	}catch(std::runtime_error &re){
		emit exception(re.what());
	}catch(...){
		emit exception("Unknown exception");
	}

	emit done(result);
}




















