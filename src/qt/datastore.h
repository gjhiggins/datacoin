/****************************************************************************
****************************************************************************/
#ifndef DATASTORE_H
#define DATASTORE_H

#include <QDialog>
#include <QStandardItemModel>

class QFile;
class QLabel;
class QLineEdit;

namespace Ui {
    class Datastore;
}

class ClientModel;
class WalletModel;

class Datastore : public QDialog
{
    Q_OBJECT

  public:
    explicit Datastore(QWidget *parent);
    ~Datastore();
    QString fileName;
    void setModel(WalletModel *model);
    void setClientModel(ClientModel *model);

private:
    Ui::Datastore *ui;
    ClientModel *clientModel;
    WalletModel *model;
    QFile *file;
    void noFileSelected();

  private Q_SLOTS:
    void on_filePushButton_clicked();
    void on_createPushButton_clicked();

};

#endif // DATASTORE_H
