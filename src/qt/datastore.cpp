/****************************************************************************
****************************************************************************/
#include <qt/datastore.h>
#include <qt/forms/ui_datastore.h>
#include <qt/guiutil.h>
#include <qt/bitcoingui.h>

#include <base58.h>
#include <hash.h>
#include <key.h>
#include <init.h>
#include <util.h>
#include <utilstrencodings.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>
#include <wallet/walletdb.h>

#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QFileDialog>
#include <QDesktopServices>

#include <fstream>

Datastore::Datastore(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Datastore)
{
    ui->setupUi(this);
}

Datastore::~Datastore()
{
    delete ui;
}

void Datastore::setModel(WalletModel *model)
{
    this->model = model;
}

void Datastore::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void Datastore::on_filePushButton_clicked()
{
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "./", tr("All Files (*.*)"));
    ui->labelFile->setText(fileName);
}

void Datastore::on_viewLocalButton_clicked()
{
    std::string hashref = ui->txLineEdit->text().toStdString();
    uint256 hash = uint256S(ui->txLineEdit->text().toUtf8().constData());
    std::string tmpfilename = "/tmp/datacoin." + hashref;
    std::ofstream tmpfile(tmpfilename, std::ios::out | std::ios::binary);

    CTransactionRef tx;
    uint256 hashBlock = uint256S("0");
    if (GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
    {
        const std::vector<unsigned char> txdata = tx->data;
        std::string strdata(txdata.begin(), txdata.end());
        tmpfile << strdata;
    }
    tmpfile.close();
    QString url = QString("file:///") + QString::fromStdString(tmpfilename);
    QDesktopServices::openUrl(QUrl(url));
    // unlink(tmpfilename.c_str());
}

void Datastore::on_viewBytestampButton_clicked()
{
    QString websitePath = "https://www2.bytestamp.net/blocks/qtx/en/";
    QString websiteLink = "eab079e959d3f2371460460bd8abe52ab172bb679061a79e6d49ac20aa22b171";
    QDesktopServices::openUrl(websitePath + websiteLink);
}

void Datastore::on_createPushButton_clicked()
{
    if (!model || vpwallets.empty())
        return;

    CWallet *pwallet = vpwallets[0];
    EnsureWalletIsUnlocked(pwallet);

    if (fileName == "")
    {
        noFileSelected();
        return;
    }

    // read whole file
    std::ifstream dataFile;
    dataFile.open(fileName.toStdString().c_str(), std::ios::binary);
    std::vector<unsigned char> dataContents(std::istreambuf_iterator<char>(dataFile), {});
    std::string encodedData = EncodeBase64(dataContents.data(), dataContents.size());

    CWalletTx wtx;

    std::string strError = "";

    if (pwallet->IsLocked())
    {
        QMessageBox unlockbox;
        unlockbox.setText("Error, Your wallet is locked! Please unlock your wallet!");
        unlockbox.exec();
        ui->txLineEdit->setText("ERROR: Your wallet is locked! Cannot send proof of data. Unlock your wallet!");
    }
    else if (pwallet->GetBalance() < 0.05)
    {
        QMessageBox error2box;
        error2box.setText("Error, You need at least 0.05 DTC to send data");
        error2box.exec();
        ui->txLineEdit->setText("ERROR: You need at least a 0.05 DTC balance to send data.");
    }
    else if (dataContents.size() > MAX_TX_DATA_SIZE)
    {
        QMessageBox error2box;
        error2box.setText("Error, data chunk is too long. split it the payload to several transactions.");
        error2box.exec();
        ui->txLineEdit->setText("ERROR: data chunk is too long. split it the payload to several transactions.");
    }
    else
    {
        strError = pwallet->SendData(wtx, false, encodedData);
    }

    if (strError != "")
    {
        QMessageBox infobox;
        infobox.setText(QString::fromStdString(strError));
        infobox.exec();
    }

    QMessageBox successbox;
    successbox.setText("Data successfully stored.");
    successbox.exec();
    ui->txLineEdit->setText(QString::fromStdString(wtx.GetHash().GetHex()));
}

void Datastore::noFileSelected()
{
  //err message
  QMessageBox errorbox;
  errorbox.setText("No file selected!");
  errorbox.exec();
}
