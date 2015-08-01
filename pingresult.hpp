#ifndef PINGRESULT_HPP
#define PINGRESULT_HPP

#include <QString>
#include <QVector>

struct PingResult{
	QString hostName;
	QVector<double> time;
};

#endif // PINGRESULT_HPP

