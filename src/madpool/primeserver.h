/*
 * primeserver.h
 *
 *  Created on: 18.04.2014
 *      Author: mad
 */

#ifndef PRIMESERVER_H_
#define PRIMESERVER_H_

#include "wallet/wallet.h"
#include "chain.h"

class PrimeServer {
public:
    
    virtual ~PrimeServer() {}
    
    static PrimeServer* CreateServer(CWallet* pwallet);
    
    virtual void NotifyNewBlock(CBlockIndex* pindex) = 0;
    
};

extern PrimeServer* gPrimeServer;

#endif /* PRIMESERVER_H_ */
