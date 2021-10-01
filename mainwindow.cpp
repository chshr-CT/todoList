#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          db(QSqlDatabase::addDatabase("QSQLITE")),
                                          model(QSqlQueryModel()),
                                          isWorking(true),
                                          readyForWork(false),
                                          thread(writeToFile, &isWorking, &readyForWork, &data, &db, &model, &mutex)
{
    ui->setupUi(this);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setModel(&model);

    db.setDatabaseName("tasks.db");

    if(!db.open()) {
        qDebug() << "can't open db";
        readyForWork = false;
        isWorking = false;
        QMessageBox::warning(this, tr("Error"), tr("You cannot run this app (can't open database)"));
        QTimer::singleShot(0, this, SLOT(close()));
    }else {
        qDebug() << "opened db";
        mutex.lock();
        QSqlQuery query(db);
        query.exec("CREATE TABLE tasks ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "date VARCHAR(20), "
                       "size INTEGER, "
                       "status VARCHAR(32), "
                       "data VARCHAR(128)"
                   ");");

        query.exec("update tasks "
                   "set status='в очереди'"
                   "where status='идет запись'");
        mutex.unlock();
        updateTable();
        readyForWork = true;
    }
}

void MainWindow::writeToFile(bool* isWorking,
                             bool* readyForWork,
                             std::vector<std::pair<int, QString>>* data,
                             QSqlDatabase* db, QSqlQueryModel* model,
                             std::mutex* mutex)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while (*isWorking) {
        if(*readyForWork) {
            mutex->lock();
            QSqlQuery query(*db);
            query.exec("select id, data, count(id) over() "
                       "from tasks "
                       "where status='в очереди' or status='идет запись';");
            mutex->unlock();

            if(query.next()) {
                data->reserve(query.value(2).toUInt());
                query.previous();
            }

            mutex->lock();
            while(query.next()) {
                if(!query.value(1).toString().isEmpty()) {
                    data->emplace_back(query.value(0).toInt(), query.value(1).toString());
                }
            }
            mutex->unlock();

            if(!data->empty()) {
                for(std::pair<int, QString>& element : *data) {
                    std::string filePath = std::to_string(element.first) + ".txt";
                    std::ofstream file;
                    QString tempStr;
                    tempStr.reserve(10);

                    mutex->lock();
                    query.exec("update tasks "
                               "set status='идет запись'"
                               "where id=" + QString::number(element.first));

                    query.exec("select id, date, size, status "
                               "from tasks;");
                    model->setQuery(query);
                    mutex->unlock();

                    mutex->lock();
                    if(!element.second.isEmpty())
                        file.open(filePath);
                    for(int i = 0; i < element.second.size() && *isWorking; i++, mutex->lock()) {
                        if(element.second[i] != '\n' && i != element.second.size() - 1) {
                            tempStr += element.second[i];
                            mutex->unlock();
                        }else {
                            tempStr += element.second[i];
                            if(file.is_open()) {
                                file.write(tempStr.toStdString().c_str(), tempStr.size());
                                file.close();
                                tempStr.clear();
                                if(*isWorking && i == element.second.size() - 1) {
                                    query.exec("update tasks "
                                               "set status='завершено'"
                                               "where id=" + QString::number(element.first));

                                    query.exec("select id, date, size, status "
                                               "from tasks;");
                                    model->setQuery(query);
                                }
                                mutex->unlock();
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                            }else {
                                file.open(filePath, std::ios_base::app);
                                if(file.is_open()) {
                                    file.write(tempStr.toStdString().c_str(), tempStr.size());
                                    file.close();
                                    tempStr.clear();
                                    if(*isWorking && i == element.second.size() - 1) {
                                        query.exec("update tasks "
                                                   "set status='завершено'"
                                                   "where id=" + QString::number(element.first));

                                        query.exec("select id, date, size, status "
                                                   "from tasks;");
                                        model->setQuery(query);
                                    }
                                    mutex->unlock();
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                                }
                            }
                        }
                    }
                    mutex->unlock();
                }
                data->clear();
            }else {
                qDebug() << "sleep";
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
}

MainWindow::~MainWindow() {
    isWorking = false;
    thread.join();
    db.close();
    delete ui;
}


void MainWindow::on_pushButton_2_clicked() {
    Dialog dialog(db,mutex);
    dialog.setModal(true);
    dialog.exec();

    updateTable();
}

void MainWindow::on_pushButton_3_clicked() {
    QModelIndexList rowIndex = ui->tableView->selectionModel()->selectedIndexes();
    if(!rowIndex.isEmpty()) {
        mutex.lock();
        QSqlQuery query(db);
        query.exec("select data "
                   "from tasks "
                   "where id=" + rowIndex.value(0).data().toString());
        query.next();
        QString data = query.value(0).toString();

        query.prepare("insert into tasks (date, size, status, data) "
                      "values (?, ?, ?, ?);");
        query.addBindValue(QTime::currentTime().toString("hh:mm ") + QDate::currentDate().toString("dd.MM.yyyy"));
        query.addBindValue(rowIndex.value(2).data().toUInt());
        query.addBindValue("в очереди");
        query.addBindValue(data);
        query.exec();
        mutex.unlock();

        updateTable();
    }
}

void MainWindow::on_pushButton_clicked() {
    QModelIndexList rowIndex = ui->tableView->selectionModel()->selectedIndexes();
    if(!rowIndex.isEmpty()) {
        mutex.lock();
        if(rowIndex.value(3).data().toString() == "идет запись" || rowIndex.value(3).data().toString() == "в очереди") {
            for(std::pair<int, QString>& element : data) {
                if(element.first == rowIndex.value(0).data().toInt()) {
                    element.second.clear();
                    break;
                }
            }
        }

        QSqlQuery query(db);
        query.exec("delete from tasks "
                   "where id=" + rowIndex.value(0).data().toString());

        QString filePath = rowIndex.value(0).data().toString() + ".txt";
        std::remove(filePath.toStdString().c_str());
        mutex.unlock();
        updateTable();
    }
}

void MainWindow::updateTable() {
    mutex.lock();
    QSqlQuery query(db);
    query.exec("select id, date, size, status "
               "from tasks;");
    model.setQuery(query);
    mutex.unlock();
}
