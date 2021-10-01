#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QtSql>

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dialog(const QSqlDatabase& dataBase, std::mutex& mutex, QWidget* parent = nullptr);
    ~Dialog() override;

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();

private:
    Ui::Dialog* ui;
    const QSqlDatabase& db;
    std::mutex& mutex;
};

#endif // DIALOG_H
