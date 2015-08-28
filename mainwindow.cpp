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
#include "probabilitydensityplot.hpp"


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
	pingTimePlot(new PingTimePlot(this))
{
	ui->setupUi(this);

    QIcon icon("stuff/pingerIcon.ico");
    this->setWindowIcon(icon);

	this->targetsTable = this->findChild<QTableWidget *>("targetsTable");
	if(this->targetsTable == nullptr)
		throw std::logic_error("targetsTable not found");

	QPushButton *submitNewHost = this->findChild<QPushButton *>("submitNewHost");
	if(submitNewHost == nullptr)
		throw std::logic_error("submitNewHost not found");
	connect(submitNewHost, &QPushButton::released, this, &MainWindow::addHost_);

	QLineEdit *newHost = this->findChild<QLineEdit *>("newHost");
	if(newHost == nullptr)
		throw std::logic_error("newHost not found");
	connect(newHost, &QLineEdit::returnPressed, this, &MainWindow::addHost_);

	QPushButton *deleteAll = this->findChild<QPushButton *>("deleteAll");
	if(deleteAll == nullptr)
		throw std::logic_error("deleteAll not found");

	connect(deleteAll, &QPushButton::released, [=](){
		while(this->targetsTable->rowCount() > 0)
			this->removeHost(0);
	});

	QPushButton *startAllPings = this->findChild<QPushButton *>("startAllPings");
	if(startAllPings == nullptr)
		throw std::logic_error("startAllPings not found");
	connect(startAllPings, &QPushButton::released, [=](){
		for(int i = 0; i < this->targetsTable->rowCount(); ++i){
			this->runPing(i);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));//чтобы пинги не конкурировали между собой
		}
	});

	QPushButton *clearAllButton = this->findChild<QPushButton *>("clearAllButton");
	if(clearAllButton == nullptr)
		throw std::logic_error("clearAllButton not found");

	connect(clearAllButton, &QPushButton::released, [=](){
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
		currentLine = currentLine.substr(0, currentLine.find_first_of(' '));
		if(currentLine.length() == 0)
			break;
		this->addHost(currentLine);
	}

	QPushButton *showAllResultsButton = this->findChild<QPushButton *>("showAllResultsButton");
	if(showAllResultsButton == nullptr)
		throw std::logic_error("showAllResultsButton not found");

	connect(showAllResultsButton, &QPushButton::released, [=](){
		PingTimePlot *plot = new PingTimePlot(this);
		QVector<PingResult> pingResults(this->pingers.size());
		for(int i = 0; i < this->pingers.size(); ++i){
			PingResult currentResult;
			currentResult.hostName = this->targetsTable->item(i, static_cast<int>(tablePosition::host))->text();
			currentResult.time = QVector<double>::fromStdVector(this->pingers.at(i)->getResult());
			pingResults[i] = std::move(currentResult);
		}


		plot->show(pingResults);
		plot->open();
	});

	targetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	qRegisterMetaType<QVector<int>>();
	qRegisterMetaType<const QVector<double> &>();
}

MainWindow::~MainWindow()

{
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
		//тут открываем окно с таблицей времени
	});

	QPushButton *showPDPButton = new QPushButton("Probability density");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::probabilityDensity), showPDPButton);
	connect(showPDPButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(showPDPButton->pos()).row();
		ProbabilityDensityPlot *pdp = new ProbabilityDensityPlot(this);
		pdp->show(this->pingers.at(row)->getResult());
		pdp->open();
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
	QTableWidgetItem *hostCell = this->targetsTable->item(row, static_cast<int>(tablePosition::host));
	QString qHost = hostCell->text();
	std::string host = qHost.toUtf8().constData();

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
		std::shared_ptr<std::runtime_error> exception = pingerPtr->getException();
		if(exception != nullptr){
			QMessageBox::warning(nullptr, "Error", exception->what());
			this->clearHost(row);
			updateTimer->stop();
			return;
		}

		//TODO replace 'row' everywhere!!!!!

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
		int row = this->targetsTable->indexAt(stopButton->pos()).row();
		pingerPtr->stop();
		this->clearHost(row);
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










