#ifndef PINGTIMEPLOT_HPP
#define PINGTIMEPLOT_HPP

#include <QDialog>
#include "pingresult.hpp"
#include "QCustomPlot/qcustomplot.h"

namespace Ui {
class PingTimePlot;
}

class PingTimePlot : public QDialog
{
	Q_OBJECT

public:
	explicit PingTimePlot(QWidget *parent = 0);
	~PingTimePlot();

	void clear();
	void show(const QVector<PingResult> &data);

private:
	Ui::PingTimePlot *ui;

};

#endif // PINGTIMEPLOT_HPP
