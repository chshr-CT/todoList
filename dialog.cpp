#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(const QSqlDatabase& dataBase, std::mutex& mutex, QWidget* parent) : QDialog(parent),
                                                                                   ui(new Ui::Dialog),
                                                                                   db(dataBase),
                                                                                   mutex(mutex)
{
    ui->setupUi(this);
}

Dialog::~Dialog() {
    delete ui;
}

void Dialog::on_pushButton_2_clicked() {

    QString data = ui->textEdit->toPlainText();

    mutex.lock();
    QSqlQuery query(db);
    query.prepare("insert into tasks (date, size, status, data) "
                  "values (?, ?, ?, ?);");
    query.addBindValue(QTime::currentTime().toString("hh:mm ") + QDate::currentDate().toString("dd.MM.yyyy"));
    query.addBindValue(data.size());
    query.addBindValue("в очереди");
    query.addBindValue(data);
    query.exec();
    mutex.unlock();

    close();
}


void Dialog::on_pushButton_clicked() {
    close();
}
