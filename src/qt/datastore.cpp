/****************************************************************************
****************************************************************************/
#include <qt/datastore.h>
#include <qt/forms/ui_datastore.h>
#include <qt/guiutil.h>
#include <qt/bitcoingui.h>

#include <base58.h>
#include <detector.h>
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
#include <QTemporaryFile>
#include <QDataStream>
#include <QTextStream>
#include <QList>
#include <QFileDialog>
#include <QDesktopServices>

#include <sstream>
#include <fstream>
#include <iostream>
#include <zlib.h>

/// mime type -> extension mapping
std::map<std::string, std::string> extension_map = {
    {"application/pdf", ".pdf"},
    {"image/svg+xml", ".svg"},
    {"audio/x-aiff", ".aiff"},
    {"audio/x-wav", ".wav"},
    {"application/ogg", ".ogg"},
    {"audio/mpeg", ".mp3"},
    {"video/mpeg", ".mpg"},
    {"video/mp4", ".mp4"},
    {"text/html", ".html"},
    {"application/x-bzip2", ".bzip2"},
    {"application/x-lzh", ".lzh"},
    {"application/x-7z-compressed", ".7z"},
    {"application/xml", ".xml"},
    {"application/x-turtle", ".ttl"},
    {"application/rdf+xml", ".xml"}
};

static QByteArray gUncompress(const QByteArray &data)
/* Solution courtesy of Ralf - https://stackoverflow.com/a/7351507 */
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

std::vector<unsigned char> uncompressdata(std::vector<unsigned char> txdata) {
    std::string strdata(txdata.begin(), txdata.end());
    QByteArray compressed_data = QByteArray::fromStdString(strdata);
    QByteArray uncompressed_data_array = gUncompress(compressed_data);
    std::string uncompressed_datastr = uncompressed_data_array.toStdString();
    std::vector<unsigned char> vector_of_uncompressed_data(uncompressed_datastr.begin(), uncompressed_datastr.end());
    return vector_of_uncompressed_data;
}

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
    std::string ext;
    std::vector<unsigned char> vuncompressed_data;
    bool compressed = false;
    uint256 hash = uint256S(ui->txLineEdit->text().toUtf8().constData());
    CTransactionRef tx;
    uint256 hashBlock = uint256S("0");

    if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
        return;

    boost::filesystem::path dest = boost::filesystem::temp_directory_path() /= boost::filesystem::unique_path();
#ifdef WIN32
    /* Windows doesn't do native() */
    QString w0 = QString::fromStdWString(dest.c_str());
    const std::string deststr = w0.toStdString();
#else
    const std::string deststr = dest.native();
#endif

    const std::vector<unsigned char> txdata = tx->data;
    Detector *d = new Detector();
    std::string mimetype = d->detect(txdata);
    if (fDebug)
        LogPrintf("txid %s, mimetype of stored data: %s\n", ui->txLineEdit->text().toStdString(), mimetype);

    if ((mimetype == "application/gzip") || (mimetype == "application/zip"))
    {
        compressed = true;
        vuncompressed_data = uncompressdata(txdata);
        std::string uncompressed_mimetype = d->detect(vuncompressed_data);
        if (extension_map.find(uncompressed_mimetype) != extension_map.end())
            ext = extension_map[uncompressed_mimetype];
        else
            ext = ".txt";
    }
    else {
        if (extension_map.find(mimetype) != extension_map.end())
            ext = extension_map[mimetype];
        else {
            std::string::size_type n = mimetype.rfind('/');
            ext = "." + mimetype.substr(n + 1, mimetype.size() - n);
        }
    }

    boost::filesystem::path destext = boost::filesystem::path(deststr + ext);
#ifdef WIN32
    QString w1 = QString::fromStdWString(destext.c_str());
    const std::string destextstr = w1.toStdString();
#else
    const std::string destextstr = destext.native();
#endif

    std::fstream tmpfile(destextstr, std::ios::out | std::ios::binary);
    if (compressed)
        tmpfile.write((const char*)&vuncompressed_data[0], vuncompressed_data.size());
    else
        tmpfile.write((const char*)&txdata[0], txdata.size());
    tmpfile.close();

#ifdef WIN32
    QString url = QString("file:///") + QString::fromStdWString(destext.c_str());
#else
    QString url = QString("file:///") + QString::fromStdString(destext.c_str());
#endif
    QDesktopServices::openUrl(QUrl(url));
    sleep(5);
    boost::filesystem::remove(destext);
}

void Datastore::on_viewBytestampButton_clicked()
{
    if (ui->txLineEdit->text().isEmpty())
        return;

    QString websitePath = "https://www2.bytestamp.net/blocks/qtx/en/";
    CTransactionRef tx;
    uint256 hashBlock = uint256S("0");

    std::string hashref = ui->txLineEdit->text().toStdString();
    uint256 hash = uint256S(ui->txLineEdit->text().toUtf8().constData());

    if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
        return;

    // QString websiteLink = ui->txLineEdit->text();
    QString websiteLink = QString::fromStdString(hashref);
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
