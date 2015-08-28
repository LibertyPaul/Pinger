#include "pingtimetable.hpp"
#include "ui_pingtimetable.h"

#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

PingTimeTable::PingTimeTable(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PingTimeTable){

	ui->setupUi(this);

	connect(this->ui->saveAsTextButton, &QPushButton::released, [=](){
		QString path = QFileDialog::getSaveFileName(this, "Save as text", "", "Text files (*.txt)");
		if(path.length() == 0)
			return;

		std::ofstream output(path.toStdString());
		if(output.is_open() == false){
			QMessageBox::critical(this, "Error", "File creating error");
			return;
		}

		for(int32_t row = 0; row < this->ui->pingTimeTable->rowCount(); ++row)
			output << this->ui->pingTimeTable->item(row, 0)->text().toStdString() << '\n';

		output.close();


	});
}

PingTimeTable::~PingTimeTable()
{
	delete ui;
}


void PingTimeTable::showTime(const QString &hostname, const QVector<double> &time){
	this->ui->hostname->setText(hostname);

	while(this->ui->pingTimeTable->rowCount() != 0)
		this->ui->pingTimeTable->removeRow(0);

	for(const auto value : time){
		int row = this->ui->pingTimeTable->rowCount();
		this->ui->pingTimeTable->insertRow(row);
		this->ui->pingTimeTable->setItem(row, 0, new QTableWidgetItem(QString::number(value)));
	}
}
