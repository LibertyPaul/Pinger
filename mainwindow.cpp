#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <stdexcept>
#include <QProgressBar>
#include <QMessageBox>
#include <atomic>
#include <future>
#include <thread>
#include <chrono>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <regex>

#include "pingresult.hpp"


enum class tablePosition{
	host = 0,
	run = 1,
	progress = 2,
	stop = 3,
	avg = 4,
	getSats = 5,
	probabilityDensity = 6,
	clear = 7,
	deleteBtn = 8
};


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	pingTimePlot(new PingTimePlot(this)){

	ui->setupUi(this);

    QIcon icon("stuff/pingerIcon.ico");
    this->setWindowIcon(icon);

	this->targetsTable = this->findChild<QTableWidget *>("targetsTable");
	if(this->targetsTable == nullptr)
		throw std::logic_error("targetsTable not found");

	if(this->ui->submitNewHost == nullptr)
		throw std::logic_error("submitNewHost not found");
	connect(this->ui->submitNewHost, &QPushButton::released, this, &MainWindow::addHost_);

	if(this->ui->newHost == nullptr)
		throw std::logic_error("newHost not found");
	connect(this->ui->newHost, &QLineEdit::returnPressed, this, &MainWindow::addHost_);

	if(this->ui->deleteAll == nullptr)
		throw std::logic_error("deleteAll not found");
	connect(this->ui->deleteAll, &QPushButton::released, [=](){
		while(this->targetsTable->rowCount() > 0)
			this->removeHost(0);
	});

	if(this->ui->startAllPings == nullptr)
		throw std::logic_error("startAllPings not found");
	connect(this->ui->startAllPings, &QPushButton::released, [=](){
		for(int i = 0; i < this->targetsTable->rowCount(); ++i){
			this->runPing(i);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));//чтобы пинги не конкурировали между собой
		}
	});

	if(this->ui->stopAllButton == nullptr)
		throw std::logic_error("stopAllButton not found");
	connect(this->ui->stopAllButton, &QPushButton::released, [=](){
		for(int i = 0; i < this->pingers.size(); ++i)
			this->stopPinger(i);
	});

	if(this->ui->clearAllButton == nullptr)
		throw std::logic_error("clearAllButton not found");
	connect(this->ui->clearAllButton, &QPushButton::released, [=](){
		for(int i = 0; i < this->targetsTable->rowCount(); ++i)
			this->clearHost(i);
	});


	std::ifstream hosts("hosts.txt");

	std::string currentLine;
	while(std::getline(hosts, currentLine)){
		currentLine = currentLine.substr(0, currentLine.find('#'));
		if(currentLine.length() == 0)
			break;
		currentLine = currentLine.substr(currentLine.find_first_not_of(' '));
		if(currentLine.length() == 0)
			break;
		currentLine = currentLine.substr(0, currentLine.find_first_of(' '));//TODO: remove not only spaces but TABs and other symbols those witch cann not apper in hostname or ip
		if(currentLine.length() == 0)
			break;
		this->addHost(currentLine);
	}

	if(this->ui->showAllResultsButton == nullptr)
		throw std::logic_error("showAllResultsButton not found");

	connect(this->ui->showAllResultsButton, &QPushButton::released, [=](){
		QVector<PingResult> pingResults(this->pingers.size());
		for(int i = 0; i < this->pingers.size(); ++i){
			PingResult currentResult;
			currentResult.hostName = this->targetsTable->item(i, static_cast<int>(tablePosition::host))->text();
			currentResult.time = QVector<double>::fromStdVector(this->pingers.at(i)->getResult());
			pingResults[i] = std::move(currentResult);
		}

		this->pingTimePlot->show(pingResults);
		this->pingTimePlot->open();
	});

	targetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	/*qRegisterMetaType<QVector<int>>();
	qRegisterMetaType<const QVector<double> &>();*/


	this->pingTimeTable = new PingTimeTable(this);
	this->pingTimePlot = new PingTimePlot(this);
	this->probabilityDensityPlot = new ProbabilityDensityPlot(this);
}

MainWindow::~MainWindow(){
	delete ui;
}


bool MainWindow::isCorrectHostname(const std::string &hostname) const{
	//return std::regex_match(hostname, std::regex("\w*"));
	return hostname.length() > 3 && hostname.find('.') != hostname.npos;//TODO: correct domain name check
}

void MainWindow::addHost_(){
	QLineEdit *newHostNameField = this->findChild<QLineEdit *>("newHost");
	if(newHostNameField == nullptr)
		throw std::logic_error("newHostNameField not found");

	QString host = newHostNameField->text();
	newHostNameField->setText("");

	this->addHost(host.toStdString());
}

void MainWindow::addHost(const std::string &host){
	if(this->isCorrectHostname(host) == false){
		if(host.length() > 0)
			QMessageBox::warning(this, QString::fromStdString("\"" + host + "\" is an incorrect hostname"), "Please, check and try again.");
		return;
	}



	int row = this->targetsTable->rowCount();
	this->targetsTable->insertRow(row);
	std::shared_ptr<Pinger> pingerPtr(new Pinger(host));
	this->pingers.push_back(pingerPtr);

	this->targetsTable->setItem(row, static_cast<int>(tablePosition::host), new QTableWidgetItem(QString::fromStdString(host)));

	QPushButton *runButton = new QPushButton("Start");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::run), runButton);
	connect(runButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(runButton->pos()).row();
		this->runPing(row);
	});


	QProgressBar *pingProgress = new QProgressBar();
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::progress), pingProgress);

	QPushButton *stopButton = new QPushButton("Stop");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::stop), stopButton);

	QTableWidgetItem *avgTime = new QTableWidgetItem;
	avgTime->setTextAlignment(Qt::AlignCenter);
	this->targetsTable->setItem(row, static_cast<int>(tablePosition::avg), avgTime);

	QPushButton *showStats = new QPushButton("Get stats");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::getSats), showStats);

	connect(showStats, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(showStats->pos()).row();
		QTableWidgetItem *hostField = this->targetsTable->item(row, static_cast<int>(tablePosition::host));
		this->pingTimeTable->showTime(hostField->text(), QVector<double>::fromStdVector(this->pingers.at(row)->getResult()));
		this->pingTimeTable->open();
	});

	QPushButton *showPDPButton = new QPushButton("Probability density");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::probabilityDensity), showPDPButton);
	connect(showPDPButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(showPDPButton->pos()).row();
		//ProbabilityDensityPlot *pdp = new ProbabilityDensityPlot(this);
		//pdp->show(this->pingers.at(row)->getResult());
		//pdp->open();

		this->probabilityDensityPlot->show(this->pingers.at(row)->getResult());
		this->probabilityDensityPlot->open();
	});

	QPushButton *clearButton = new QPushButton("Clear");
	connect(clearButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(clearButton->pos()).row();
		this->clearHost(row);
	});
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::clear), clearButton);

	QPushButton *deleteButton = new QPushButton("Delete");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::deleteBtn), deleteButton);
	connect(deleteButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(deleteButton->pos()).row();//я потратил 2 часа чтоб понять как это сделать
		this->removeHost(row);
	});

	this->clearHost(row);
}

void MainWindow::clearHost(const int row){
	this->stopPinger(row);

	QProgressBar *pingProgress = dynamic_cast<QProgressBar *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::progress)));
	if(pingProgress == nullptr)
		throw std::logic_error("pingProgress not found");
	pingProgress->setValue(0);

	QTableWidgetItem *avgTime = this->targetsTable->item(row, static_cast<int>(tablePosition::avg));
	if(avgTime == nullptr)
		throw std::logic_error("avgTime not found");
	avgTime->setText("");

	QPushButton *getStats = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::getSats)));
	if(getStats == nullptr)
		throw std::logic_error("getStats not found");
	getStats->setEnabled(false);

	QPushButton *showPDPButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::probabilityDensity)));
	if(showPDPButton == nullptr)
		throw std::logic_error("showPDPButton not found");
	showPDPButton->setEnabled(false);

	QPushButton *stopButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::stop)));
	if(stopButton == nullptr)
		throw std::logic_error("stopButton not found");
	stopButton->setEnabled(false);
}

void MainWindow::removeHost(int row){
	this->targetsTable->removeRow(row);
	this->pingers.remove(row);
}


void MainWindow::runPing(const int row){
	QProgressBar *pingProgress = dynamic_cast<QProgressBar *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::progress)));
	if(pingProgress == nullptr)
		throw std::logic_error("pingProgress not found");

	QTableWidgetItem *avgTime = this->targetsTable->item(row, static_cast<int>(tablePosition::avg));
	if(avgTime == nullptr)
		throw std::logic_error("avgTime not found");

	QPushButton *showPDPButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::probabilityDensity)));
	if(showPDPButton == nullptr)
		throw std::logic_error("showPDPButton not found");

	QPushButton *stopButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::stop)));
	if(stopButton == nullptr)
		throw std::logic_error("stopButton not found");
	stopButton->setEnabled(true);

	QTimer *updateTimer = new QTimer(this);
	updateTimer->setInterval(100);

	std::shared_ptr<Pinger> pingerPtr(this->pingers[row]);

	connect(updateTimer, &QTimer::timeout, this, [=](){
		int row = this->targetsTable->indexAt(stopButton->pos()).row();//небольшой костыль
		std::unique_ptr<std::runtime_error> exception = std::move(std::unique_ptr<std::runtime_error>(pingerPtr->getLastException()));
		if(exception != nullptr){
			QMessageBox::warning(nullptr, "Error", exception->what());
			this->clearHost(row);
			updateTimer->stop();
			return;
		}

		double progress_d = pingerPtr->getProgress();
		int progress = progress_d * 100;
		pingProgress->setValue(progress);

		if(pingerPtr->isReady()){
			updateTimer->stop();
			showPDPButton->setEnabled(true);
			stopButton->setEnabled(false);

			QPushButton *getStats = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::getSats)));
			if(stopButton == nullptr)
				throw std::logic_error("getStats not found");
			getStats->setEnabled(true);


			std::vector<double> result = pingerPtr->getResult();
			double avg = std::accumulate(result.cbegin(), result.cend(), 0.0) / result.size();
			std::stringstream ss;
			ss << std::setprecision(4) << avg << "ms";
			std::string avg_s;
			ss >> avg_s;

			avgTime->setText(avg_s.c_str());


		}
	});



	connect(stopButton, &QPushButton::released, [=](){
		pingerPtr->stop();
	});


	QDoubleSpinBox *pingDelay = this->findChild<QDoubleSpinBox *>("pingDelay");
	if(pingDelay == nullptr)
		throw std::logic_error("pingDelay not found");

	QSpinBox *requestCountInput = this->findChild<QSpinBox *>("pingCount");
	if(requestCountInput == nullptr)
		throw std::logic_error("pingCount not found");

	std::thread t(&Pinger::run, pingerPtr, requestCountInput->value(), pingDelay->value());
	t.detach();


	updateTimer->start();


}


void MainWindow::stopPinger(const int row){
	this->pingers[row]->stop();
}






