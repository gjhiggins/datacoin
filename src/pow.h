// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <consensus/params.h>
#include <bignum.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

bool TestNet();

/* Determine the next difficulty for the succeeding block */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const Consensus::Params&, const CBigNum& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength, bool fSilent = false);

#endif // BITCOIN_POW_H
