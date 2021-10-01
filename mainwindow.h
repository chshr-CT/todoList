#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QCloseEvent>

#include <map>
#include <fstream>

#include "dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    static void writeToFile(bool* isWorking,
                            bool* readyForWork,
                            std::vector<std::pair<int, QString>>* data,
                            QSqlDatabase* db,
                            QSqlQueryModel* model,
                            std::mutex* mutex);
    void updateTable();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_clicked();

private:
    Ui::MainWindow* ui;
    QSqlDatabase db;
    QSqlQueryModel model;
    std::vector<std::pair<int, QString>> data;
    bool isWorking;
    bool readyForWork;
    std::mutex mutex;
    std::thread thread;
};
#endif // MAINWINDOW_H
