// Copyright (c) 2011-2017 The Bitcoin Core developers
// Copyright (c) 2020 The Datacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsentry.h>
#include <qt/forms/ui_sendcoinsentry.h>

#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <crypto/sha256.h> // hashing
#include <util.h>
#include <utilstrencodings.h>
#include <iostream>        // std::cout
#include <fstream>         // std::filebuf, std::ifstream
#include <regex>

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

SendCoinsEntry::SendCoinsEntry(const PlatformStyle *_platformStyle, QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::SendCoinsEntry),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    ui->addressBookButton->setIcon(platformStyle->SingleColorIcon(":/icons/address-book"));
    ui->pasteButton->setIcon(platformStyle->SingleColorIcon(":/icons/editpaste"));
    ui->deleteButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_is->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_s->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));

    setCurrentWidget(ui->SendCoins);

    if (platformStyle->getUseExtraSpacing())
        ui->payToLayout->setSpacing(4);
#if QT_VERSION >= 0x040700
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
#endif

    // normal bitcoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying bitcoin address(es)
    ui->payTo_is->setFont(GUIUtil::fixedPitchFont());

    // Connect signals
    connect(ui->payAmount, SIGNAL(valueChanged()), this, SIGNAL(payAmountChanged()));
    connect(ui->inscriptionText, SIGNAL(textEdited(QString)), this, SLOT(inscriptionChanged()));
    connect(ui->checkboxSubtractFeeFromAmount, SIGNAL(toggled(bool)), this, SIGNAL(subtractFeeFromAmountChanged()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->useAvailableBalanceButton, SIGNAL(clicked()), this, SLOT(useAvailableBalanceClicked()));
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_selectFileButton_clicked()
{
    QString fileName;
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);

    if (dlg.exec())
    {
        fileName = dlg.selectedFiles()[0];
        unsigned char shahash[CSHA256::OUTPUT_SIZE];
        std::ifstream infile (fileName.toStdString(), std::ios::binary);

        // get size of file
        infile.seekg (0,infile.end);
        long bufsize = infile.tellg();
        infile.seekg (0);

        // allocate memory for file content
        char* buffer = new char[bufsize];

        // read content of infile
        infile.read (buffer, bufsize);
        infile.close();

        CSHA256().Write((const unsigned char*)buffer, bufsize).Finalize(shahash);
        std::string notaryID = HashToString(shahash);

        // release dynamically-allocated memory
        delete[] buffer;

        if (!(IsHex(notaryID) && notaryID.length() == 64)) {
            ui->inscriptionText->setValid(false);
            return;
        }
        // Make sure wallet is unlocked
        WalletModel::UnlockContext ctx(model->requestUnlock());
        if (!ctx.isValid()) {
            return;
        }

        // Warn if file is NULL
        if (notaryID == "") {
            QMessageBox::warning(this, tr("Notarize File"),
                tr("Unable to open file for hashing."),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
        ui->inscriptionText->setText(QString::fromStdString(notaryID));
    }
}

/*
void SendCoinsEntry::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableWidget->indexAt(point);
    if (index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void SendCoinsEntry::onCopyTxID()
{
    QString txID = ui->tableWidget->selectedItems().at(0)->text();
    if (txID.length() > 0) {
        QApplication::clipboard()->setText(txID);
    }
}
*/

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
    updateLabel(address);
}

bool SendCoinsEntry::inscriptionChanged()
{
    if(!model)
        return false;

    if (!this->validateInscription())
        return false;

    return true;
}

void SendCoinsEntry::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model && _model->getOptionsModel())
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    clear();
}

void SendCoinsEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->checkboxSubtractFeeFromAmount->setCheckState(Qt::Unchecked);
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    ui->inscriptionText->clear();
    // clear UI elements for unauthenticated payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->inscriptionText_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for authenticated payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->inscriptionText_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void SendCoinsEntry::checkSubtractFeeFromAmount()
{
    ui->checkboxSubtractFeeFromAmount->setChecked(true);
}

void SendCoinsEntry::deleteClicked()
{
    Q_EMIT removeEntry(this);
}

void SendCoinsEntry::useAvailableBalanceClicked()
{
    Q_EMIT useAvailableBalance(this);
}

bool SendCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!model->validateAddress(ui->payTo->text()))
    {
        retval = false;
        ui->payTo->setValid(retval);
        return retval;
    }

    if (ui->inscriptionText->text().isEmpty()) {
        if (!ui->payAmount->validate())
        {
            retval = false;
        }

        // Sending a zero amount is invalid
        if (ui->payAmount->value(0) <= 0)
        {
            QMessageBox::warning(this, tr("ZERO AMOUNT"),
                tr("Amount cannot be zero."),
                QMessageBox::Ok, QMessageBox::Ok);
            ui->payAmount->setValid(false);
            retval = false;
        }
        // Reject dust outputs:
        if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
            QMessageBox::warning(this, tr("AMOUNT IS DUST"),
                tr("Amount too small, below dust amount."),
                QMessageBox::Ok, QMessageBox::Ok);
            ui->payAmount->setValid(false);
            retval = false;
        }
    }

    if (!ui->inscriptionText->text().isEmpty())
    {
        retval = false;

        CTxDestination destination = DecodeDestination(ui->payTo->text().toStdString());
        const CKeyID* keyID = boost::get<CKeyID>(&destination);
        CKey key;
        if (!model->getPrivKey(*keyID, key))
        {
            QMessageBox::warning(this, tr("NOT SENDING TO SELF"),
                tr("Inscription field is not blank, you must use an address that you own."),
                QMessageBox::Ok, QMessageBox::Ok);
            ui->payTo->setValid(retval);
            return retval;
        }

        if (ui->payAmount->value(0) > 0)
        {
            QMessageBox::warning(this, tr("AMOUNT NOT ZERO."),
                tr("The amount must be 0 for an inscription transaction."),
                QMessageBox::Ok, QMessageBox::Ok);
            retval = false;
            ui->payAmount->setValid(retval);
            return retval;
        }

        // Check if it is a hex string (produced by clicking "Notarise File")
        if (IsHex(ui->inscriptionText->text().toStdString())) {
            // and is of the correct length
            if (ui->inscriptionText->text().length() == 64)
                retval = true;
            else {
                QMessageBox::warning(this, tr("Invalid Hash"),
                    tr("Incorrect hash digest length."),
                    QMessageBox::Ok, QMessageBox::Ok);
                retval = false;
            }
        }
        // Else check if it's a valid TrustyUri
        else if ((ui->inscriptionText->text().startsWith("ni://") && (ui->inscriptionText->text().length() < 127))) {
            std::string str = ui->inscriptionText->text().toStdString();
            std::regex str_expr ("(^ni.?://)(.*?)([/\\?]{1,})(.*)");
            std::smatch sm;
            std::regex_match(str, sm, str_expr);
            if (sm.size() != 5) {
                QMessageBox::warning(this, tr("Unrecognised TrustyURI"),
                    tr("TrustyURI is not in recognised format of ni://example.org/sha-256;5AbXdpz5DcaYXCh9l3eI9ruBosiL5XDU3rxBbBaUO70"),
                    QMessageBox::Ok, QMessageBox::Ok);
                retval = false;
            } else {
                retval = true;
            }
        }
        ui->inscriptionText->setValid(retval);
    }

    return retval;
}

bool SendCoinsEntry::validateInscription()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    if (!ui->inscriptionText->text().isEmpty())
    {
        // Check if it is a hex string (produced by clicking "Notarise File")
        if (IsHex(ui->inscriptionText->text().toStdString())) {
            // and is of the correct length
            if (ui->inscriptionText->text().length() == 64)
                retval = false;
        }
        // Else check if it's a valid TrustyUri
        else if ((ui->inscriptionText->text().startsWith("ni://") && (ui->inscriptionText->text().length() < 127))) {
            std::string str = ui->inscriptionText->text().toStdString();
            std::regex str_expr ("(^ni.?://)(.*?)([/\\?]{1,})(.*)");
            std::smatch sm;
            std::regex_match(str, sm, str_expr);
            if (sm.size() != 5) {
                QMessageBox::warning(this, tr("Unrecognised TrustyURI"),
                    tr("TrustyURI is not in recognised format of ni://example.org/sha-256;5AbXdpz5DcaYXCh9l3eI9ruBosiL5XDU3rxBbBaUO70"),
                    QMessageBox::Ok, QMessageBox::Ok);
                retval = false;
            } else {
                retval = true;
            }
        }

        ui->inscriptionText->setValid(retval);
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->messageTextLabel->text();
    recipient.inscription = ui->inscriptionText->text();
    recipient.fSubtractFeeFromAmount = (ui->checkboxSubtractFeeFromAmount->checkState() == Qt::Checked);

    return recipient;
}

QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    QWidget *w = ui->payAmount->setupTabChain(ui->addAsLabel);
    QWidget::setTabOrder(w, ui->checkboxSubtractFeeFromAmount);
    QWidget::setTabOrder(ui->checkboxSubtractFeeFromAmount, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    return ui->deleteButton;
}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    recipient = value;

    if (recipient.paymentRequest.IsInitialized()) // payment request
    {
        if (recipient.authenticatedMerchant.isEmpty()) // unauthenticated
        {
            ui->payTo_is->setText(recipient.address);
            ui->memoTextLabel_is->setText(recipient.message);
            ui->inscriptionText_is->setText(recipient.inscription);
            ui->payAmount_is->setValue(recipient.amount);
            ui->payAmount_is->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_UnauthenticatedPaymentRequest);
        }
        else // authenticated
        {
            ui->payTo_s->setText(recipient.authenticatedMerchant);
            ui->memoTextLabel_s->setText(recipient.message);
            ui->inscriptionText_s->setText(recipient.inscription);
            ui->payAmount_s->setValue(recipient.amount);
            ui->payAmount_s->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_AuthenticatedPaymentRequest);
        }
    }
    else // normal payment
    {
        // message
        ui->messageTextLabel->setText(recipient.message);
        ui->messageTextLabel->setVisible(!recipient.message.isEmpty());
        ui->messageLabel->setVisible(!recipient.message.isEmpty());

        // inscriptiom
        ui->inscriptionText->setText(recipient.inscription);
        ui->inscriptionText->setVisible(true);
        ui->inscriptionLabel->setVisible(true);

        ui->addAsLabel->clear();
        ui->payTo->setText(recipient.address); // this may set a label from addressbook
        if (!recipient.label.isEmpty()) // if a label had been set from the addressbook, don't overwrite with an empty label
            ui->addAsLabel->setText(recipient.label);
        ui->payAmount->setValue(recipient.amount);
    }
}

void SendCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

void SendCoinsEntry::setInscription(const QString &inscription)
{
    ui->inscriptionText->setText(inscription);
}

void SendCoinsEntry::setAmount(const CAmount &amount)
{
    ui->payAmount->setValue(amount);
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}
