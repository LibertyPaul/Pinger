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

#include "pingresult.hpp"
#include "probabilitydensityplot.hpp"


enum class tablePosition{
	host = 0,
	run = 1,
	progress = 2,
	stop = 3,
	avg = 4,
	probabilityDensity = 5,
	clear = 6,
	deleteBtn = 7
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


	this->addHost("libertypaul.ru");
	this->addHost("yandex.ru");
	this->addHost("github.org");
	this->addHost("habrahabr.ru");
	this->addHost("stackoverflow.com");
	this->addHost("twitter.com");
	this->addHost("vkb2010.ru");
	this->addHost("ec.dstu.edu.ru");
	this->addHost("ru.wikipedia.org");


	QPushButton *showAllResultsButton = this->findChild<QPushButton *>("showAllResultsButton");
	if(showAllResultsButton == nullptr)
		throw std::logic_error("showAllResultsButton not found");

	connect(showAllResultsButton, &QPushButton::released, [=](){
		PingTimePlot *plot = new PingTimePlot(this);
		QVector<PingResult> pingResults(this->results.size());
		for(int i = 0; i < this->results.size(); ++i){
			PingResult currentResult;
			currentResult.hostName = this->targetsTable->item(i, static_cast<int>(tablePosition::host))->text();
			currentResult.time = this->results.at(i);
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


void MainWindow::addHost_(){
	QLineEdit *newHost = this->findChild<QLineEdit *>("newHost");
	if(newHost == nullptr)
		throw std::logic_error("newHost not found");

	QString host = newHost->text();
	if(host.size() == 0){
		QMessageBox::warning(newHost, "Incorrect host", "Please, check and try again.");
		return;
	}
	newHost->setText("");

	this->addHost(host);
}

void MainWindow::addHost(const QString &host){
	int row = this->targetsTable->rowCount();
	this->targetsTable->insertRow(row);
	this->results.push_back(QVector<double>());

	this->targetsTable->setItem(row, static_cast<int>(tablePosition::host), new QTableWidgetItem(host));

	QPushButton *runButton = new QPushButton("Start");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::run), runButton);
	connect(runButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(runButton->pos()).row();
		this->runPing(row);
	});


	QProgressBar *pingProgress = new QProgressBar();
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::progress), pingProgress);
	pingProgress->setValue(0);

	QPushButton *stopButton = new QPushButton("Stop");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::stop), stopButton);
	stopButton->setEnabled(false);

	QTableWidgetItem *avgTime = new QTableWidgetItem;
	avgTime->setTextAlignment(Qt::AlignCenter);
	this->targetsTable->setItem(row, static_cast<int>(tablePosition::avg), avgTime);

	QPushButton *showPDPButton = new QPushButton("Probability density");
	this->targetsTable->setCellWidget(row, static_cast<int>(tablePosition::probabilityDensity), showPDPButton);
	showPDPButton->setEnabled(false);
	connect(showPDPButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(showPDPButton->pos()).row();
		ProbabilityDensityPlot *pdp = new ProbabilityDensityPlot(this);
		pdp->show(this->results.at(row));
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

	QPushButton *showPDPButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::probabilityDensity)));
	if(showPDPButton == nullptr)
		throw std::logic_error("showPDPButton not found");
	showPDPButton->setEnabled(false);

	QPushButton *getStopButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, static_cast<int>(tablePosition::stop)));
	if(getStopButton == nullptr)
		throw std::logic_error("getStopButton not found");
	getStopButton->setEnabled(false);

	this->results[row].clear();
}

void MainWindow::removeHost(int row){
	this->targetsTable->removeRow(row);
	this->results.remove(row);
}


void MainWindow::runPing(const int row){
	QTableWidgetItem *hostCell = this->targetsTable->item(row, static_cast<int>(tablePosition::host));
	QString qHost = hostCell->text();
	std::string host = qHost.toUtf8().constData();


	QDoubleSpinBox *pingDelay = this->findChild<QDoubleSpinBox *>("pingDelay");
	if(pingDelay == nullptr)
		throw std::logic_error("pingDelay not found");

	QSpinBox *requestCountInput = this->findChild<QSpinBox *>("pingCount");
	if(requestCountInput == nullptr)
		throw std::logic_error("pingCount not found");


	Pinger *pinger = new Pinger(host, requestCountInput->value(), pingDelay->value());

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
	updateTimer->start();

	connect(updateTimer, &QTimer::timeout, this, [=](){
		std::shared_ptr<std::runtime_error> exception = pinger->getException();
		if(exception != nullptr){
			QMessageBox::warning(nullptr, "Error", exception->what());
			updateTimer->stop();
			return;
		}

		double progress_d = pinger->getProgress();
		int progress = progress_d * 100;
		pingProgress->setValue(progress);

		if(pinger->isReady()){
			updateTimer->stop();
			QVector<double> result = QVector<double>::fromStdVector(pinger->getResult());
			this->results[row] = result;
			showPDPButton->setEnabled(true);
			stopButton->setEnabled(false);

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
		pinger->stop();
		this->clearHost(row);
	});


	std::thread t(&Pinger::run, pinger);
	t.detach();


}










