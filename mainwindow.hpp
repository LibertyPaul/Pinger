#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTableWidget>
#include <utility>
#include <mutex>
#include "pinger.hpp"

#include "pingtimeplot.hpp"
#include "pingtimetable.hpp"
#include "probabilitydensityplot.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *ui;
	QTableWidget *targetsTable;

	PingTimePlot *pingTimePlot;
	PingTimeTable *pingTimeTable;
	ProbabilityDensityPlot *probabilityDensityPlot;

	QVector<std::shared_ptr<Pinger>> pingers;
	std::mutex mainWindowMutex;

	bool isCorrectHostname(const std::string &hostname) const;
	void addHost_();
	void addHost(const std::string &host);
	void removeHost(int row);
	void runPing(const int row);
	bool isHostEnabled(const int row) const;
	void clearHost(const int row);
	void stopPinger(const int row);
};
#endif // MAINWINDOW_HPP
