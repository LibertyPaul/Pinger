#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <stdexcept>
#include <QCheckBox>
#include <QProgressBar>
#include <QMessageBox>
#include <atomic>
#include <future>
#include <thread>
#include <chrono>
#include <numeric>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

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
			this->targetsTable->removeRow(0);
	});

	this->addHost("libertypaul.ru");
	this->addHost("yandex.ru");
	this->addHost("yahoo.com");


	targetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	qRegisterMetaType<PingResult>("PingResult");
	qRegisterMetaType<QVector<int>>("QVector<int>");
}

MainWindow::~MainWindow()
{
	delete ui;
}

/*
   table columns:
   0: host
   1: isActive
   2: run
   3: progress
   4: avg
   5: result
   6: clear
   7: delete
 */

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

	this->targetsTable->setItem(row, 0, new QTableWidgetItem(host));

	QCheckBox *isActive = new QCheckBox();
	isActive->setChecked(true);
	this->targetsTable->setCellWidget(row, 1, isActive);

	QPushButton *runButton = new QPushButton("Start");
	this->targetsTable->setCellWidget(row, 2, runButton);
	runButton->setEnabled(isActive->isChecked());
	connect(isActive, &QCheckBox::stateChanged, runButton, &QPushButton::setEnabled);
	connect(runButton, &QPushButton::released, [=](){
		auto pos = runButton->pos();
		auto index = this->targetsTable->indexAt(pos);
		int row = index.row();
		this->runPing(row);
	});


	QProgressBar *pingProgress = new QProgressBar();
	this->targetsTable->setCellWidget(row, 3, pingProgress);
	pingProgress->setValue(0);

	QTableWidgetItem *avgTime = new QTableWidgetItem;
	avgTime->setTextAlignment(Qt::AlignCenter);
	this->targetsTable->setItem(row, 4, avgTime);

	QPushButton *getResultButton = new QPushButton("Result");
	this->targetsTable->setCellWidget(row, 5, getResultButton);
	getResultButton->setEnabled(false);
	connect(getResultButton, &QPushButton::released, [=](){
		//TODO: show result
	});

	QPushButton *clearButton = new QPushButton("Clear");
	connect(clearButton, &QPushButton::released, [=](){
		pingProgress->setValue(0);
		avgTime->setText("");
		getResultButton->setEnabled(false);
		//тут надо обнулить все значения об этой строке
	});
	this->targetsTable->setCellWidget(row, 6, clearButton);

	QPushButton *deleteButton = new QPushButton("Delete");
	this->targetsTable->setCellWidget(row, 7, deleteButton);
	connect(deleteButton, &QPushButton::released, [=](){
		int row = this->targetsTable->indexAt(deleteButton->pos()).row();//я потратил 2 часа чтоб понять как это сделать
		this->targetsTable->removeRow(row);
	});


}




void MainWindow::runPing(const int row){
	QCheckBox *isActive = dynamic_cast<QCheckBox *>(this->targetsTable->cellWidget(row, 1));
	if(isActive == nullptr)
		throw std::runtime_error("isActive QCheckBox not found");

	if(isActive->isChecked() == false){
		QMessageBox::warning(isActive, "Host is disabled", "How the hell did you push disabled start button???");
		return;
	}

	QTableWidgetItem *hostCell = this->targetsTable->item(row, 0);
	QString qHost = hostCell->text();
	std::string host = qHost.toUtf8().constData();


	Pinger *pinger = new Pinger(host, 10, 200000);

	QProgressBar *pingProgress = dynamic_cast<QProgressBar *>(this->targetsTable->cellWidget(row, 3));
	if(pingProgress == nullptr)
		throw std::logic_error("pingProgress not found");

	connect(pinger, &Pinger::progressChanged, pingProgress, &QProgressBar::setValue);
	connect(pinger, &Pinger::exception, [&](std::string what){
		QMessageBox::warning(nullptr, "Exception", what.c_str());
	});

	QMetaObject::Connection pingDoneConnection = connect(pinger, &Pinger::done, [&](PingResult pingResult){
		//TODO: store pingResult somewhere
		double avg = std::accumulate(pingResult.cbegin(), pingResult.cend(), 0.0) / pingResult.size() / 1000;

		QTableWidgetItem *avgTime = this->targetsTable->item(row, 4);
		if(avgTime == nullptr)
			throw std::logic_error("avgTime not found");

		std::stringstream ss;
		ss << std::setprecision(4) << avg << "ms";
		std::string avg_s;
		ss >> avg_s;
		avgTime->setText(avg_s.c_str());

		QPushButton *getResultButton = dynamic_cast<QPushButton *>(this->targetsTable->cellWidget(row, 5));
		if(getResultButton == nullptr)
			throw std::logic_error("getResultButton not found");

		getResultButton->setEnabled(true);

		//disconnect(pingDoneConnection);
	});

	std::thread t(&Pinger::run, pinger);
	t.detach();


}










