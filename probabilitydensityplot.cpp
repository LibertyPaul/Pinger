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

}

ProbabilityDensityPlot::~ProbabilityDensityPlot()
{
	delete ui;
}

void ProbabilityDensityPlot::show(QVector<double> time){
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
	//double minY = *std::min_element(y.cbegin(), y.cend());


	QCPGraph *graph = ui->probabilityDensityPlotArea->addGraph();
	graph->setName("Распределение вероятности");

	QPen pen;
	pen.setWidth(1);
	//ColorGenerator cg;
	//QColor color = cg.generate();
	QColor color("black");
	pen.setColor(color);
	graph->setPen(pen);

	graph->setLineStyle(QCPGraph::lsImpulse);

	graph->setData(x, y);
	//ui->probabilityDensityPlotArea->xAxis->setTickStep(1);
	const int margin = 25;
	ui->probabilityDensityPlotArea->xAxis->setRange(minX - margin, maxX - 1 + margin);
	ui->probabilityDensityPlotArea->yAxis->setRange(0, maxY * 1.1);
	ui->probabilityDensityPlotArea->repaint();
}
