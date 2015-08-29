#include "pingtimeplot.hpp"
#include "ui_pingtimeplot.h"
#include "colorgenerator.hpp"
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <random>
#include <cmath>


PingTimePlot::PingTimePlot(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PingTimePlot){

	ui->setupUi(this);

	connect(ui->pingResultPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->pingResultPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(ui->pingResultPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->pingResultPlot->yAxis2, SLOT(setRange(QCPRange)));

	ui->pingResultPlot->legend->setVisible(true);
	ui->pingResultPlot->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
	ui->pingResultPlot->xAxis->setLabel("Request no.");
	ui->pingResultPlot->yAxis->setLabel("Lag, ms");
	ui->pingResultPlot->xAxis->setTickStep(1);

}

PingTimePlot::~PingTimePlot(){
	delete ui;
}

void PingTimePlot::clear(){
	ui->pingResultPlot->clearGraphs();
}

void PingTimePlot::show(const QVector<PingResult> &data){
	this->clear();
	double maxY = 0;
	int maxX = 0;

	ColorGenerator cg;

	for(const auto graphData : data){
		QCPGraph *graph = ui->pingResultPlot->addGraph();
		graph->setName(graphData.hostName);

		QPen pen;
		pen.setWidth(2);
		QColor color = cg.generate();
		color.setAlphaF(0.8);
		pen.setColor(color);
		graph->setPen(pen);

		uint32_t pos = 0;

		QVector<double> x(graphData.time.size());
		for(auto &xValue : x)
			xValue = pos++;
		ui->pingResultPlot->graph()->setData(x, graphData.time);

		maxX = std::max(graphData.time.size(), maxX);
		maxY = std::max(*std::max_element(graphData.time.cbegin(), graphData.time.cend()), maxY);

	}

	ui->pingResultPlot->xAxis->setRange(0, maxX - 1);
	ui->pingResultPlot->yAxis->setRange(0, maxY);
	ui->pingResultPlot->xAxis->setTickStep(1);
	ui->pingResultPlot->replot();
}







