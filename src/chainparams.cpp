// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <pow.h>
#include <prime/prime.h>
#include <validation.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>
#include <memory>


static CBlock CreateGenesisBlock(const char* pszStartTopic, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, CBigNum bnPrimeChainMultiplier, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(999) << std::vector<unsigned char>((const unsigned char*)pszStartTopic, (const unsigned char*)pszStartTopic + strlen(pszStartTopic));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
	genesis.bnPrimeChainMultiplier = bnPrimeChainMultiplier;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, CBigNum bnPrimeChainMultiplier, const CAmount& genesisReward)
{
    const char* pszStartTopic = "https://bitcointalk.org/index.php?topic=325735.0";
    const CScript genesisOutputScript = CScript();
    return CreateGenesisBlock(pszStartTopic, genesisOutputScript, nTime, nNonce, nBits, nVersion, bnPrimeChainMultiplier, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // always enforce P2SH BIP16
        consensus.BIP34Height = 950; // Move to version 2 blocks, do not recognize v1 blocks after this height + hash
        consensus.BIP34Hash = uint256S("22596accbbde801463d46b802343c915010bcadf1c098119a252a0f17664b466");
        consensus.BIP65Height = -1; // Implements OP_CHECKLOCKTIMEVERIFY opcode which locks tx's until future time.
        consensus.BIP66Height = -1; // Enforces DER encoding from 363725 block onwards. https://en.wikipedia.org/wiki/X.690#DER_encoding
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // a weeks
        consensus.nPowTargetSpacing = 60; // one-minute blocks
        consensus.nCoinbaseMaturity = 100;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        // rule changes require 96% agreement, measured across 4 retargeting periods.
        // 95% consensus is required to accept protocol rule changes
        consensus.nRuleChangeActivationThreshold = 1026; // 95% of 1080
        // The 95% consensus must be obtained in 2016 blocks time (window = one retargeting period).
        consensus.nMinerConfirmationWindow = 1080; // nPowTargetTimespan / nPowTargetSpacing


        // the minimum and maximum times that three softforks can activate and how they are to be signaled using version bits.

        // Satoshi's playground
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        // From the documentation : "Block height at which CSV (BIP68, BIP112 and BIP113) becomes active."
        // These 3 BIP's collectively knowh as CheckSequenceVerify = https://en.bitcoin.it/wiki/CheckSequenceVerify
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL; // NOTE: DATACOIN segwit disabled (was SEGWIT = 1479168000; // November 15th, 2016.)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL; // NOTE: DATACOIN segwit disabled (was SEGWIT =1510704000; // November 15th, 2017.)

        // The best chain should have at least this much work.
        // NOTE: DATACOIN changed Minimum work of the active chain.
        // Loading blocks will not start until the headers reach this level.
        consensus.nMinimumChainWork = uint256S("000000000000000000000000000000000000000000000000000031d4a178b250");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("b7183015dc5e4f2e1be353329e7fd9c0eb32efef412e0a0c5c970a9aacde8d8d"); //3128684

        consensus.nTargetInitialLength = 7; // NOTE: PRIMECOIN initial prime chain target
        consensus.nTargetMinLength = 6;     // NOTE: PRIMECOIN minimum prime chain target

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xda;
        pchMessageStart[1] = 0xdc;
        pchMessageStart[2] = 0xdd;
        pchMessageStart[3] = 0xed;
        nDefaultPort = 4777;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1384627170, 49030125, TargetFromInt(6), 2, (uint64_t) 5651310, COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("1d724e874ee9ea571563239bde095911f128db47c7612fb1968c08c9f95cabe8"));
        assert(genesis.hashMerkleRoot == uint256S("fe5d7082c24c53362f6b82211913d536677aaffafde0dcec6ff7b348ff6265f8"));

        // nodes with support for servicebits filtering should be at the top
        // vSeeds.emplace_back("minkiz.co");
        vSeeds.emplace_back("144.76.118.44");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,30);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,90);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128+30);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x02, 0xF9, 0x61, 0x03};
        base58Prefixes[EXT_SECRET_KEY] = {0x02, 0xF9, 0x61, 0x2D};
        base58Prefixes[EXT_COIN_TYPE]  = {0x80, 0x00, 0x00, 0x46}; // BIP44 coin type is '70'

        bech32_hrp = "dc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // Whether to check mempool and block index consistency by default
        fDefaultConsistencyChecks = false;
        // Filter out transactions that don't comply with the Bitcoin Core standard
        fRequireStandard = true;

        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        // Blockchain is assumed valid before each of these.
        /**
         * What makes a good checkpoint block?
         * + Is surrounded by blocks with reasonable timestamps
         *   (no blocks before with a timestamp after, none after with
         *    timestamp before)
         * + Contains no strange transactions
         */
        // NOTE: DATACOIN changed Add checkpoints
        checkpointData = {
            {
                {   72204, uint256S("661b85bab200d0b1f72c6909c5b2602af8227459ae72b7afbff75d16c8e2b703")},
                { 2000000, uint256S("4e49f85b69f68d6f58b2b18473c4bf17d88e6dd7c79d7d416e22522da17bc91a")},
                { 3128684, uint256S("b7183015dc5e4f2e1be353329e7fd9c0eb32efef412e0a0c5c970a9aacde8d8d")},
            }
        };

        // Transaction data from a certain block onwards.
        chainTxData = ChainTxData{
            // Data as of block b7183015dc5e4f2e1be353329e7fd9c0eb32efef412e0a0c5c970a9aacde8d8d (height 3128684).
            1569522270, // * UNIX timestamp of last known number of transactions
            3538876,    // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            2140497552, // * total data size
            0.020,      // * estimated number of transactions per second after that timestamp
            0.083       // * estimated data rate (bytes per sec)
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // always enforce P2SH BIP16
        consensus.BIP34Height = 750;
        consensus.BIP34Hash = uint256S("8af7eb332ff63e1ff919043fbe87c9cfa2a168903e88c1e34850151a3aed2be0");
        consensus.BIP65Height = -1;
        consensus.BIP66Height = -1;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // a weeks
        consensus.nPowTargetSpacing = 60;
        consensus.nCoinbaseMaturity = 10;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        // rule changes require 96% agreement, measured across 4 retargeting periods.
        // 95% consensus is required to accept protocol rule changes
        consensus.nRuleChangeActivationThreshold = 810; // 75% for testchains
        consensus.nMinerConfirmationWindow = 1080; // nPowTargetTimespan / nPowTargetSpacing

        // Deployment of BIP68, BIP112, and BIP113.
        // From the documentation : "Block height at which CSV (BIP68, BIP112 and BIP113) becomes active."
        // These 3 BIP's collectively knowh as CheckSequenceVerify = https://en.bitcoin.it/wiki/CheckSequenceVerify
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL; // NOTE: DATACOIN segwit disabled (was SEGWIT = 1462060800; // May 1st 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT; // NOTE: DATACOIN segwit disabled (was SEGWIT = 1493596800; // May 1st 2017)

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("26ee5563233ed8cbdd8af5f16bc55b73d9d8cc727392d507292ca959fd08c03f"); //442608

	    consensus.nTargetInitialLength = 4; // NOTE: PRIMECOIN initial prime chain target
        consensus.nTargetMinLength = 2;     // NOTE: PRIMECOIN minimum prime chain target

        pchMessageStart[0] = 0xdb;
        pchMessageStart[1] = 0xde;
        pchMessageStart[2] = 0xdb;
        pchMessageStart[3] = 0xd3;

        nDefaultPort = 4776;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1385686192, 46032, TargetFromInt(4), 2, (uint64_t) 211890, COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("26ee5563233ed8cbdd8af5f16bc55b73d9d8cc727392d507292ca959fd08c03f"));
        assert(genesis.hashMerkleRoot == uint256S("fe5d7082c24c53362f6b82211913d536677aaffafde0dcec6ff7b348ff6265f8"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // Temporary kickstarter
        vSeeds.emplace_back("minkiz.co");
        vSeeds.emplace_back("144.76.118.44");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,70);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,132);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128+70);
        base58Prefixes[EXT_PUBLIC_KEY] = {0xAC, 0x7E, 0x99, 0x8A};
        base58Prefixes[EXT_SECRET_KEY] = {0xAC, 0x7E, 0x99, 0xB4};
        base58Prefixes[EXT_COIN_TYPE]  = {0x80, 0x00, 0x00, 0x01}; // BIP44 coin type is '1' (testnet)

        bech32_hrp = "dct";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0,
            0,
            0
        };

    }
};

/**
 * Regression test
 */

class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Height = 0; // always enforce P2SH BIP16 on regtest
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        // NOTE: DATACOIN changed DTC COINBASE_MATURITY = 3200.
        // For spending tests, you have to create a long chain. But the bitcoin test is designed for a height of (100)
        // which is less, so increase BIP66Height to be > 3200
        consensus.BIP66Height = 12510; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // a week
        consensus.nPowTargetSpacing = 60; // one-minute blocks
        consensus.nCoinbaseMaturity = 10;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;

        // rule changes require 96% agreement, measured across 4 retargeting periods.
        // 95% consensus is required to accept protocol rule changes
        consensus.nRuleChangeActivationThreshold = 60; // 75% for testchains
        consensus.nMinerConfirmationWindow = 80; // Faster than normal for regtest (80 instead of 1080)

        // Deployment of BIP68, BIP112, and BIP113.
        // From the documentation : "Block height at which CSV (BIP68, BIP112 and BIP113) becomes active."
        // These 3 BIP's collectively knowh as CheckSequenceVerify = https://en.bitcoin.it/wiki/CheckSequenceVerify
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL; // NOTE: DATACOIN segwit disabled during tests. then you need to turn it back on (was SEGWIT = 0ULL;)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT; // NOTE: DATACOIN segwit disabled (was SEGWIT = 999999999999ULL);

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("3864a16a5e7c9f79f2ab2ebc41e943f342f6737b83649844f6b41334eb7e5ba8");

        consensus.nTargetInitialLength = 1; // NOTE: PRIMECOIN initial prime chain target
        consensus.nTargetMinLength = 1;     // NOTE: PRIMECOIN minimum prime chain target

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1385686192, 46032, TargetFromInt(1), 2, (uint64_t) 211890, COIN); // NOTE: DATACOIN changed, remake as nTargetMinLength=2, nTargetInitialLength=4
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("3864a16a5e7c9f79f2ab2ebc41e943f342f6737b83649844f6b41334eb7e5ba8"));
        assert(genesis.hashMerkleRoot == uint256S("fe5d7082c24c53362f6b82211913d536677aaffafde0dcec6ff7b348ff6265f8"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,70);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,132);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128+70);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x02, 0xF9, 0x61, 0x03};
        base58Prefixes[EXT_SECRET_KEY] = {0x02, 0xF9, 0x61, 0x2D};
        base58Prefixes[EXT_COIN_TYPE]  = {0x80, 0x00, 0x00, 0x01}; // BIP44 coin type is '1' (testnet)

        bech32_hrp = "dcrt";
        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0,
            0,
            0
        };

    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);


}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}

/*
mainnet / regtest
=================

EXT_PUBLIC_KEY: {0x02, 0xF9, 0x61, 0x03}
EXT_SECRET_KEY: {0x02, 0xF9, 0x61, 0x2D}

p2pkhpub: {0x02, 0xF9, 0x61, 0x03}
p2pkhprv: {0x02, 0xF9, 0x61, 0x2D}
p2shpub: {0x02, 0xF9, 0x61, 0x03}
p2shprv: {0x02, 0xF9, 0x61, 0x2D}
p2sh_p2wpkhpub: {0x02, 0xF9, 0x62, 0x99}
p2sh_p2wpkhprv: {0x02, 0xF9, 0x62, 0xC2}
p2sh_p2wshpub: {0x02, 0xF9, 0x3B, 0x0A}
p2sh_p2wshprv: {0x02, 0xF9, 0x3B, 0x34}
p2wpkhpub: {0x02, 0xF9, 0x64, 0x2E}
p2wpkhprv: {0x02, 0xF9, 0x64, 0x57}
p2wshpub: {0x02, 0xF9, 0x3C, 0x9F}
p2wshprv: {0x02, 0xF9, 0x3C, 0xC9}

testnet
=======

EXT_PUBLIC_KEY: {0xAC, 0x7E, 0x99, 0x8A}
EXT_SECRET_KEY: {0xAC, 0x7E, 0x99, 0xB4}

p2pkhpub: {0x02, 0xF9, 0x5A, 0xAF}
p2pkhprv: {0x02, 0xF9, 0x5A, 0xD9}
p2shpub: {0x02, 0xF9, 0x5A, 0xAF}
p2shprv: {0x02, 0xF9, 0x5A, 0xD9}
p2sh_p2wpkhpub: {0x02, 0xF9, 0x5C, 0x44}
p2sh_p2wpkhprv: {0x02, 0xF9, 0x5C, 0x6E}
p2sh_p2wshpub: {0x02, 0xF9, 0x34, 0xB6}
p2sh_p2wshprv: {0x02, 0xF9, 0x34, 0xE0}
p2wpkhpub: {0x02, 0xF9, 0x5D, 0xD9}
p2wpkhprv: {0x02, 0xF9, 0x5E, 0x03}
p2wshpub: {0x02, 0xF9, 0x36, 0x4B}
p2wshprv: {0x02, 0xF9, 0x36, 0x75}
*/
