// Copyright (c) 2020 The Datacoin Core developers
// Copyright (c) 2016-2018 Duality Blockchain Solutions Developers
// Copyright (c) 2014-2018 The Dash Core Developers
// Copyright (c) 2009-2018 The Bitcoin Developers
// Copyright (c) 2009-2018 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MININGPAGE_H
#define MININGPAGE_H

#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <pow.h>

#include <QPushButton>
#include <QWidget>

#include <memory>

#if __STDC_VERSION__ < 201112L
#include <boost/move/unique_ptr.hpp>
#endif


namespace Ui {
class MiningPage;
}

class ClientModel;
class WalletModel;

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MiningPage();

    void setModel(WalletModel *model);
    void setClientModel(ClientModel *model);

private:
    Ui::MiningPage *ui;
    ClientModel *clientModel;
    WalletModel *model;
    int maxGenProc;
    int nThreads;
    int nUseThreads;
#if __STDC_VERSION__ < 201112L
    boost::movelib::unique_ptr<WalletModel::UnlockContext> unlockContext;
#else
    std::unique_ptr<WalletModel::UnlockContext> unlockContext;
#endif
    bool hasMiningprivkey;

    void restartMining(bool fGenerate, int nThreads);
    void timerEvent(QTimerEvent *event);
    void updateUI(bool fGenerate);
    int sieveextensionsValue;
    long long int sievesizeValue;
    int sieveprimesValue;
    int l1cacheValue;
    void StartMiner();
    void StopMiner();
    void showHashMeterControls(bool show);
    void SetMiningParams();
    bool isMinerOn();
    bool isMining;


private Q_SLOTS:

    void changeNumberOfCores(int i);
    void switchMining();
    void updateSievePrimes(int i);
    void showHashRate(int i);
    void changeSampleTime(int i);
    void clearHashRateData();
};

#endif // MININGPAGE_H
