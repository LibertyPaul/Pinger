#include "probabilitydensityplot.hpp"
#include "ui_probabilitydensityplot.h"
#include "colorgenerator.hpp"
#include <algorithm>
#include <set>

ProbabilityDensityPlot::ProbabilityDensityPlot(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ProbabilityDensityPlot)
{
	ui->setupUi(this);

	connect(ui->probabilityDensityPlotArea->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->probabilityDensityPlotArea->xAxis2, SLOT(setRange(QCPRange)));
	connect(ui->probabilityDensityPlotArea->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->probabilityDensityPlotArea->yAxis2, SLOT(setRange(QCPRange)));
	ui->probabilityDensityPlotArea->xAxis->setLabel("Time, ms");
	ui->probabilityDensityPlotArea->yAxis->setLabel("Ratio");
}

ProbabilityDensityPlot::~ProbabilityDensityPlot()
{
	delete ui;
}

void ProbabilityDensityPlot::show(const std::vector<double> &time){
	this->show(QVector<double>::fromStdVector(time));
}

void ProbabilityDensityPlot::show(const QVector<double> &time){
	QVector<double> x;
	QVector<double> y;

	std::set<double> timeList(time.cbegin(), time.cend());
	std::multiset<double> timeCounts(time.cbegin(), time.cend());

	for(const auto timeItem : timeList){
		x.push_back(timeItem);
		y.push_back(static_cast<double>(timeCounts.count(timeItem)) / time.size());
	}

	this->plot(x, y);
}


void ProbabilityDensityPlot::plot(const QVector<double> &x, const QVector<double> &y){
	double maxX = *std::max_element(x.cbegin(), x.cend());
	double minX = *std::min_element(x.cbegin(), x.cend());
	double maxY = *std::max_element(y.cbegin(), y.cend());


	QCPGraph *graph = ui->probabilityDensityPlotArea->addGraph();
	QPen pen;
	pen.setWidth(2);
	QColor color("black");
	pen.setColor(color);
	graph->setPen(pen);

	graph->setLineStyle(QCPGraph::lsImpulse);

	graph->setData(x, y);
	const int margin = 25;
	ui->probabilityDensityPlotArea->xAxis->setRange(std::max(0., minX - margin), maxX - 1 + margin);
	ui->probabilityDensityPlotArea->yAxis->setRange(0, maxY * 1.1);
	ui->probabilityDensityPlotArea->replot();
}
