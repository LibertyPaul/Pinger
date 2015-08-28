#ifndef PINGTIMETABLE_HPP
#define PINGTIMETABLE_HPP

#include <QString>
#include <QVector>
#include <QDialog>

namespace Ui {
class PingTimeTable;
}

class PingTimeTable : public QDialog
{
	Q_OBJECT

public:
	explicit PingTimeTable(QWidget *parent = 0);
	~PingTimeTable();

public slots:
	void showTime(const QString &hostname, const QVector<double> &time);

private:
	Ui::PingTimeTable *ui;
};

#endif // PINGTIMETABLE_HPP
