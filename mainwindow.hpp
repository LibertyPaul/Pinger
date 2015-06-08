#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTableWidget>
#include "pinger.hpp"

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

	void addHost_();
	void addHost(const QString &host);
	void runPing(const int row);

};
#endif // MAINWINDOW_HPP
