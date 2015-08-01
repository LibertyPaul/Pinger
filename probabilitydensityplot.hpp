#ifndef PROBABILITYDENSITYPLOT_HPP
#define PROBABILITYDENSITYPLOT_HPP

#include <QDialog>
#include <QVector>
#include "QCustomPlot/qcustomplot.h"

namespace Ui {
class ProbabilityDensityPlot;
}

class ProbabilityDensityPlot : public QDialog
{
	Q_OBJECT

public:
	explicit ProbabilityDensityPlot(QWidget *parent = 0);
	~ProbabilityDensityPlot();

	void show(QVector<double> time);

private:
	void plot(const QVector<double> &x, const QVector<double> &y);
	Ui::ProbabilityDensityPlot *ui;
};

#endif // PROBABILITYDENSITYPLOT_HPP
