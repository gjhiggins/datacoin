// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <bignum.h>
#include <hash.h>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;  // Primecoin: prime chain target, see prime.cpp
    uint32_t nNonce;

    // Primecoin: proof-of-work certificate
    // Multiplier to block hash to derive the probable prime chain (k=0, 1, ...)
    // Cunningham Chain of first kind:  hash * multiplier * 2**k - 1
    // Cunningham Chain of second kind: hash * multiplier * 2**k + 1
    // BiTwin Chain:                    hash * multiplier * 2**k +/- 1
    CBigNum bnPrimeChainMultiplier;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        READWRITE(bnPrimeChainMultiplier);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        bnPrimeChainMultiplier = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    // NOTE: PRIMECOIN header hash does not include prime certificate
    // This hash is used to check POW. As noted above, bnPrimeChainMultiplier hash isn't included
    uint256 GetHeaderHash() const
    {
        // NOTE: DATACOIN changed. Changing hashing
        //return Hash(BEGIN(nVersion), END(nNonce));

        //CDataStream ss(SER_GETHASH, 0);
        //ss << nVersion << hashPrevBlock << hashMerkleRoot << nTime << nBits << nNonce;
        //return Hash(ss.begin(), ss.end());

        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << nVersion << hashPrevBlock << hashMerkleRoot << nTime << nBits << nNonce;
        return ss.GetHash();
    }

	uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;
    mutable unsigned int nPrimeChainType;   // NOTE: PRIMECOIN chain type (memory-only)
    mutable unsigned int nPrimeChainLength; // NOTE: PRIMECOIN chain length (memory-only)

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
        nPrimeChainType = 0;
        nPrimeChainLength = 0;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.bnPrimeChainMultiplier = bnPrimeChainMultiplier;
        // TODO(gjh): DATACOIN oldclient, XPM does not fill this field either. Fix?
        // also CBlockIndex::GetBlockHeader()
        //block.bnPrimeChainMultiplier = bnPrimeChainMultiplier; 
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
