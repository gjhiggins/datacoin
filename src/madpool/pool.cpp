/*
 * pool.cpp
 *
 *  Created on: 18.04.2014
 *      Author: mad
 */



#include "net.h"
#include "validation.h"
#include "miner.h"

#include "pool.h"

PrimeWorker::PrimeWorker(CWallet* pwallet, unsigned threadid, unsigned target)
{
    
    mWallet = pwallet;
    mThreadID = threadid;
    
    mServer = 0;
    mSignals = 0;
    
    mCurrHeight = 0;
    mExtraNonce = 0;
    //mBlockTemplate = 0;
    mIndexPrev = 0;
    mWorkerCount = 0;
    mInvCount = 0;
    
    mServerPort = gArgs.GetArg("-serverport", 60000) + 2*mThreadID;
    mSignalPort = mServerPort+1;
    
    mTarget = target;
    mReqDiff = gArgs.GetArg("-reqdiff", 0);
    mHost = gArgs.GetArg("-host", "127.0.0.1");
    mName = gArgs.GetArg("-servername", "DatacoinMineServer");
    
    mServerInfo.set_host(mHost);
    mServerInfo.set_router(mServerPort);
    mServerInfo.set_pub(mSignalPort);
    mServerInfo.set_target(mTarget);
    
    mServerStats.set_name(mName);
    mServerStats.set_thread(mThreadID);
    mServerStats.set_workers(0);
    
    //Each thread has its own key and counter

    // TODO: DATACOIN optimize? Implement address reuse or mining to a single address?
    // A large number of addresses can slow down the wallet.
    pwallet->GetScriptForMining(coinbase_script);
    
    // If the keypool is exhausted, no script is returned at all.  Catch this.
    if (!coinbase_script) {
        LogPrintf("Keypool ran out. Refilling...\n");
        {
            LOCK2(cs_main, pwallet->cs_wallet);

            EnsureWalletIsUnlocked(pwallet);
            // 0 is interpreted by TopUpKeyPool() as the default keypool size given by -keypool
            pwallet->TopUpKeyPool(0);
    
            if (pwallet->GetKeyPoolSize() < 1) {
                LogPrintf("Error refreshing keypool. Non initialised miner thread.\n");
            }
        }       
        pwallet->GetScriptForMining(coinbase_script);       
        if (!coinbase_script) {
            LogPrintf("Can't get coinbase script from keypool. Non initialised miner thread.\n");
        }
    }
    //throw an error if no script was provided
    if (coinbase_script->reserveScript.empty()) {
        LogPrintf("No coinbase script available. Non initialised miner thread.\n");
    }
}


void PrimeWorker::InvokeWork(zsock_t *pipe, void *args){
    
    ((PrimeWorker*)args)->Work(pipe);
    
}

int PrimeWorker::InvokeInput(zloop_t *wloop, zmq_pollitem_t *item, void* arg){
    
    void** arr= (void**)arg;
    return ((PrimeWorker*)arr[0])->HandleInput((zsock_t*) arr[1]);
    
}

/*int PrimeWorker::InvokeBackend(zloop_t *wloop, zmq_pollitem_t *item, void *arg){
    
    return ((PrimeWorker*)arg)->HandleBackend(item);
    
}*/

int PrimeWorker::InvokeRequest(zloop_t *wloop, zmq_pollitem_t *item, void* arg){
    
    void** arr= (void**)arg;
    return ((PrimeWorker*)arr[0])->HandleRequest((zsock_t*) arr[1]);
    
}

int PrimeWorker::InvokeTimerFunc(zloop_t *wloop, int timer_id, void *arg) {
    
    return ((PrimeWorker*)arg)->FlushStats();
    
}

int PrimeWorker::InvokeExitCheck(zloop_t *wloop, zmq_pollitem_t *item, void *arg) {
    
    bool terminate=false;

    zmsg_t* msg = zmsg_recv((zsock_t*) arg);
    if (!msg) terminate=true;   //terminated
    else {
        char *command = zmsg_popstr (msg);
        if (streq (command, "$TERM")) terminate=true;
        free (command);
    }
    zmsg_destroy (&msg);
    if (terminate) return -1; else return 0;
}

void PrimeWorker::Work(zsock_t *pipe) {
    
    zsock_t* frontend = zsock_new(ZMQ_DEALER);
    zsock_t* input = zsock_new(ZMQ_SUB);
    mServer = zsock_new(ZMQ_ROUTER);
    mSignals = zsock_new(ZMQ_PUB);
    
    int err = 0;
    err = zsock_bind(mServer, "tcp://*:%d", mServerPort);
    if(!err)
        LogPrintf("zsock_bind(mServer, tcp://*:*) failed.\n");
    
    err = zsock_bind(mSignals, "tcp://*:%d", mSignalPort);
    if(!err)
        LogPrintf("zsock_bind(mSignals, tcp://*:*) failed.\n");
        
    err = zsock_connect(frontend, "inproc://frontend");
    assert(!err);
        
    err = zsock_connect(input, "inproc://bitcoin");
    assert(!err);
    
    const char one[2] = {1, 0};
    zsock_set_subscribe(input, one);
    
    zloop_t* wloop = zloop_new();
    
    zmq_pollitem_t item_input = {zsock_resolve(input), 0, ZMQ_POLLIN, 0};
    void* args_input[2] = {this, input};
    err = zloop_poller(wloop, &item_input, &PrimeWorker::InvokeInput, args_input);
    assert(!err);
    
    zmq_pollitem_t item_server = {zsock_resolve(mServer), 0, ZMQ_POLLIN, 0};
    void* args_server[2] = {this, mServer};
    err = zloop_poller(wloop, &item_server, &PrimeWorker::InvokeRequest, args_server);
    assert(!err);
    
    zmq_pollitem_t item_frontend = {zsock_resolve(frontend), 0, ZMQ_POLLIN, 0};
    void* args_frontend[2] = {this, frontend};
    err = zloop_poller(wloop, &item_frontend, &PrimeWorker::InvokeRequest, args_frontend);
    assert(!err);
    
    err = zloop_timer(wloop, 60000, 0, &PrimeWorker::InvokeTimerFunc, this);
    assert(err >= 0);
    
    zmq_pollitem_t item_terminate = {zsock_resolve(pipe), 0, ZMQ_POLLIN, 0};
    err = zloop_poller(wloop, &item_terminate, &PrimeWorker::InvokeExitCheck, pipe);
    assert(!err);

    //zloop_set_nonstop(wloop, true);
    //zsys_interrupted=0;
    LogPrintf("PrimeWorker started: mServerPort=%d mSignalPort=%d\n", mServerPort, mSignalPort);
    zsock_signal(pipe, 0);
    
    zloop_start(wloop);
        
    zloop_destroy(&wloop);
    
    zsock_destroy(&mServer);
    zsock_destroy(&mSignals);
    zsock_destroy(&frontend);
    zsock_destroy(&input);
    
    LogPrintf("PrimeWorker stopped.\n");
    zsock_signal(pipe, 0);
    
}


zmsg_t* PrimeWorker::ReceiveRequest(proto::Request& req, zsock_t* socket) {
    
    zmsg_t* msg = zmsg_recv(socket);
    zframe_t* frame = zmsg_last(msg);
    zmsg_remove(msg, frame);
    size_t fsize = zframe_size(frame);
    const byte* fbytes = zframe_data(frame);
    
    bool ok = req.ParseFromArray(fbytes, fsize);
    zframe_destroy(&frame);
    
    bool valid = false;
    while(ok){
        
        if(!proto::Request::Type_IsValid(req.type()))
            break;
        
        if(!req.has_reqnonce())
            break;
        
        if(CheckVersion(req.version()) <= 0)
            break;
        
        uint256 reqnonce;
        {
            const std::string& nonce = req.reqnonce();
            if(nonce.length() != sizeof(uint256))
                break;
            memcpy(reqnonce.begin(), nonce.c_str(), sizeof(uint256));
        }
        
        if(!CheckReqNonce(reqnonce) || !mReqNonces.insert(reqnonce).second)
            break;
        
        if(req.has_stats()){
            
            const proto::ClientStats& stats = req.stats();
            if(!isValidUTF8(stats.addr()))
                break;
            if(!isValidUTF8(stats.name()))
                break;
            if(stats.cpd() < 0 || stats.cpd() > 150. || stats.cpd() != stats.cpd())
                break;
            
        }
        
        if(req.has_share()){
            
            const proto::Share& share = req.share();
            if(!isValidUTF8(share.addr()))
                break;
            if(!isValidUTF8(share.name()))
                break;
            if(!isValidUTF8(share.hash()))
                break;
            if(!isValidUTF8(share.merkle()))
                break;
            if(!isValidUTF8(share.multi()))
                break;
            if(share.has_blockhash() && !isValidUTF8(share.blockhash()))
                break;
            
        }
        
        valid = true;
        break;
    }
    
    if(valid)
        return msg;
    else{
        mInvCount++;
        zmsg_destroy(&msg);
        return 0;
    }
    
}


void PrimeWorker::SendReply(const proto::Reply& rep, zmsg_t** msg, zsock_t* socket) {
    
    size_t fsize = rep.ByteSize();
    zframe_t* frame = zframe_new(0, fsize);
    byte* data = zframe_data(frame);
    rep.SerializeToArray(data, fsize);
    
    zmsg_append(*msg, &frame);
    zmsg_send(msg, socket);
    
}


int PrimeWorker::HandleInput(zsock_t* item) {
    
    zmsg_t* msg = zmsg_recv(item);
    zframe_t* frame = zmsg_next(msg);
    size_t fsize = zframe_size(frame);
    const byte* fbytes = zframe_data(frame);
    
    proto::Signal& sig = mSignal;
    sig.ParseFromArray(fbytes+1, fsize-1);
    
    if(sig.type() == proto::Signal::NEWBLOCK){
        
        mCurrBlock = sig.block();
        mCurrHeight = mCurrBlock.height();
        //LogPrintf("HandleInput(): proto::Signal::NEWBLOCK %d\n", mCurrHeight);
        
        zmsg_send(&msg, mSignals);
        
        while(true){
            
            while(g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL)==0)
                MilliSleep(1000);
            
            mIndexPrev = chainActive.Tip();
            
            if(!mIndexPrev)
                MilliSleep(1000);
            else
                break;
            
        }
        
        mWorkerCount = mNonceMap.size();
        
        mNonceMap.clear();
        mReqNonces.clear();
        mShares.clear();
        
        //if(mBlockTemplate)
        //  delete mBlockTemplate;
        
        if (coinbase_script)
            mBlockTemplate=BlockAssembler(Params()).CreateNewBlock(coinbase_script->reserveScript, false);
        else LogPrintf("ERROR: CreateNewBlock(). No coinbase script available. Non initialised miner thread.\n"); 
        
        if(!mBlockTemplate){
            LogPrintf("ERROR: CreateNewBlock() failed.\n");
            return -1;
        }
        
        if (!mBlockTemplate.get()){
            LogPrintf("ERROR2: CreateNewBlock() failed.\n");
            return -1;
        }
        
    }else if(sig.type() == proto::Signal::SHUTDOWN){
        
        LogPrintf("HandleInput(): proto::Signal::SHUTDOWN\n");
        
        zmsg_send(&msg, mSignals);
        
        FlushStats();
        
        return -1;
        
    }
    
    zmsg_destroy(&msg);
    return 0;
    
}


int PrimeWorker::FlushStats() {
    
    unsigned long latency = 0;
    double cpd = 0;
    for(std::map<std::pair<std::string,uint64_t>, proto::Data>::const_iterator iter = mStats.begin();
            iter != mStats.end(); ++iter)
    {
        const proto::ClientStats& stats = iter->second.clientstats();
        if(stats.latency() < 60*1000)
            latency += stats.latency();
        cpd += stats.cpd();
    }
    
    if(mStats.size())
        latency /= mStats.size();
    
    mServerStats.set_workers(mWorkerCount);
    mServerStats.set_latency(latency);
    mServerStats.set_cpd(cpd);
    
    for(std::map<std::pair<int,int>,int>::const_iterator iter = mReqStats.begin(); iter != mReqStats.end(); ++iter){
        
        proto::ReqStats* stats = mServerStats.add_reqstats();
        stats->set_reqtype((proto::Request::Type)iter->first.first);
        stats->set_errtype((proto::Reply::ErrType)iter->first.second);
        stats->set_count(iter->second);
        
    }
    
    //mServerStats.PrintDebugString();
    // TODO: DATACOIN optimize?
    //LogPrintf("[PrimeServer] %d workers, %d ms latency, %.2f chains/day\n", mWorkerCount, (int)latency, (float)cpd);
    
    mServerStats.mutable_reqstats()->Clear();
    mReqStats.clear();
    mStats.clear();
    
    //LogPrintf("PrimeWorker %d: mInvCount = %d/%d\n", mThreadID, (unsigned)(mInvCount >> 32), (unsigned)mInvCount);
    
    return 0;
    
}


int PrimeWorker::CheckVersion(unsigned version) {
    
    /*unsigned client = version >> 4;
    unsigned target = version % 16;
    
    if(target < mTarget)
        return -1;*/
    
    if(version >= 10){
        return 2;
    }else
        return 0;
    
}


int PrimeWorker::CheckReqNonce(const uint256& nonce) {
    
    const uint32_t* limbs = (uint32_t*)nonce.begin();
    
    uint32_t tmp = limbs[0];
    for(int i = 1; i < 7; ++i)
        tmp *= limbs[i];
    tmp += limbs[7];
    
    return !tmp;
    
}


int PrimeWorker::HandleRequest(zsock_t* item) {
    
    proto::Request& req = mRequest;
    zmsg_t* msg = ReceiveRequest(req, item);
    if(!msg)
        return 0;
    
    //req.PrintDebugString();
    
    proto::Request::Type rtype = req.type();
    proto::Reply::ErrType etype = proto::Reply::NONE;
    
    proto::Reply& rep = mReply;
    rep.Clear();
    rep.set_type(rtype);
    rep.set_reqid(req.reqid());
    
    if(!proto::Request::Type_IsValid(rtype)){
        LogPrintf("ERROR: !proto::Request::Type_IsValid.\n");
        rtype = proto::Request::NONE;
        etype = proto::Reply::INVALID;
    }
    
    while(etype == proto::Reply::NONE) {
        
        int vstatus = CheckVersion(req.version());
        if(vstatus <= 0){
            rep.set_errstr("Your miner version is no longer supported. Please upgrade.");
            etype = proto::Reply::VERSION;
            break;
        }
        
        if(rtype == proto::Request::CONNECT){
            
            rep.mutable_sinfo()->CopyFrom(mServerInfo);
                
            if(vstatus == 1){
                etype = proto::Reply::VERSION;
                rep.set_errstr("Your miner version will no longer be supported in the near future. Please upgrade.");
            }
            
        }else if(rtype == proto::Request::GETWORK){
            
            if(!mCurrBlock.has_height()){
                etype = proto::Reply::HEIGHT;
                break;
            }
            
            if(req.height() != mCurrHeight){
                etype = proto::Reply::HEIGHT;
                break;
            }
            
            CBlock *pblock = &mBlockTemplate->block;
            IncrementExtraNonce(pblock, mIndexPrev, mExtraNonce);
            pblock->nTime = std::max(pblock->nTime, (unsigned int)GetAdjustedTime());
            
            mNonceMap[pblock->hashMerkleRoot] = mExtraNonce;
            
            proto::Work* work = rep.mutable_work();
            work->set_height(mCurrHeight);
            work->set_merkle(pblock->hashMerkleRoot.GetHex());
            work->set_time(pblock->nTime);
            work->set_bits(pblock->nBits);
            
        }else if(rtype == proto::Request::SHARE){
            
            if(!mCurrBlock.has_height()){
                etype = proto::Reply::STALE;
                break;
            }
            
            if(!req.has_share()){
                LogPrintf("ERROR: !req.has_share().\n");
                etype = proto::Reply::INVALID;
                break;
            }
            
            const proto::Share& share = req.share();
            
            if(share.height() != mCurrHeight){
                etype = proto::Reply::STALE;
                break;
            }
            
            if(share.length() < mCurrBlock.minshare()){
                LogPrintf("ERROR: share.length too short.\n");
                etype = proto::Reply::INVALID;
                break;
            }
            
            uint256 merkleRoot;
            merkleRoot.SetHex(share.merkle());
            
            unsigned extraNonce = mNonceMap[merkleRoot];
            if(!extraNonce){
                etype = proto::Reply::STALE;
                break;
            }
            
            unsigned nCandidateType = share.chaintype();
            if(nCandidateType > 2){
                LogPrintf("ERROR: share.chaintype invalid.\n");
                etype = proto::Reply::INVALID;
                break;
            }
            
            CBlock *pblock = &mBlockTemplate->block;
            extraNonce--;
            IncrementExtraNonce(pblock, mIndexPrev, extraNonce);
            // NOTE: DATACOIN miner 
            // Unfortunately, the current miner does not transmit the version to the network
            // and considers nVersion == 2. Need to edit the xpmclient client to suit
            pblock->nVersion=2; 
            pblock->nTime = share.time();
            pblock->nBits = share.bits();
            pblock->nNonce = share.nonce();
            
            uint256 headerHash = pblock->GetHeaderHash();
            {
                uint256 headerHashClient;
                headerHashClient.SetHex(share.hash());
                if(headerHashClient != headerHash){
                    LogPrintf("ERROR: headerHashClient != headerHash.\n");
                    etype = proto::Reply::INVALID;
                    break;
                }
            }
            
            pblock->bnPrimeChainMultiplier.SetHex(share.multi());
            uint256 blockhash = pblock->GetHash();
            
            if(!mShares.insert(blockhash).second){
                etype = proto::Reply::DUPLICATE;
                break;
            }
            
            //CBigNum bnChainOrigin = CBigNum(headerHash) * pblock->bnPrimeChainMultiplier;
            //unsigned int nChainLength = 0;
            //bool isblock = ProbablePrimeChainTestForMiner(bnChainOrigin, pblock->nBits, nCandidateType+1, nChainLength);
            //unsigned int nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin;
            // TODO: DATACOIN optimize ? or fermat test = false?
            //bool isblock = ProbablePrimeChainTest(bnChainOrigin, pblock->nBits, true, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);
            
            bool isblock = CheckWork(pblock, *mWallet, coinbase_script, true);
            
            if (isblock) {
                LogPrintf("[PrimeServer] target=%s len=%s type=%d\n", TargetToString(pblock->nBits).c_str(), TargetToString(pblock->nPrimeChainLength).c_str(), (int)pblock->nPrimeChainType);
                LogPrintf("[PrimeServer] !!! --- BLOCK ACCEPTED --- !!!\n");
                rep.set_errstr("!!! --- BLOCK ACCEPTED --- !!!");
            }else{
                //LogPrintf("[PrimeServer] target=%s len=%d type=%d\n", TargetToString(pblock->nBits).c_str(), (int)share.length(), (int)share.chaintype()+1);
                //LogPrintf("[PrimeServer] !!! --- BLOCK REJECTED --- !!!\n");
                //rep.set_errstr("BLOCK ACCEPTED");
                //etype = proto::Reply::INVALID;
            }
            
            //nChainLength = TargetGetLength(nChainLength);
            //nChainLength = pblock->nPrimeChainLength;
            //if(nChainLength >= mCurrBlock.minshare()){
            //  
            //  if(isblock)
            //      LogPrintf("[PrimeServer] !!! --- BLOCK ACCEPTED --- !!!\n");
            //      //isblock = CheckWork(pblock, *mWallet, coinbase_script);
            //  
            //  if(share.length() != nChainLength){
            //      LogPrintf("ERROR: share.length() != nChainLength.\n");
            //      etype = proto::Reply::INVALID;
            //  }
            //  
            //}else{
            //  
            //  LogPrintf("ERROR: share.length too short after test: %d/%d\n", nChainLength, share.length());
            //  etype = proto::Reply::INVALID;
            //  break;
            //  
            //}
            
        }else if(rtype == proto::Request::STATS){
            
            //LogPrintf("[PrimeServer] Recieved STATS\n");
            
            if(!req.has_stats()){
                LogPrintf("ERROR: !req.has_stats().\n");
                etype = proto::Reply::INVALID;
                break;
            }
            
            const proto::ClientStats& stats = req.stats();
            std::pair<std::string,uint64_t> key = std::make_pair(stats.addr(), stats.clientid() * stats.instanceid());
            
            std::map<std::pair<std::string,uint64_t>, proto::Data>::iterator iter = mStats.find(key);
            if(iter != mStats.end()){
                
                proto::ClientStats* s = mStats[key].mutable_clientstats();
                s->set_version(std::min(s->version(), stats.version()));
                s->set_cpd(s->cpd() + stats.cpd());
                s->set_errors(s->errors() + stats.errors());
                s->set_temp(std::max(s->temp(), stats.temp()));
                s->set_latency(std::max(s->latency(), stats.latency()));
                s->set_ngpus(s->ngpus() + stats.ngpus());
                /*if(s->name() != stats.name()){
                    s->mutable_name()->append("+");
                    s->mutable_name()->append(stats.name());
                }*/
                
            }else if(mStats.size() < 100000){
                mStats[key].mutable_clientstats()->CopyFrom(stats);
            }
            
        }
        
        break;
    }
    
    if(req.height() < mCurrHeight){
        rep.mutable_block()->CopyFrom(mCurrBlock);
    }
    
    mReqStats[std::make_pair(rtype,etype)]++;
    
    rep.set_error(etype);
    
    SendReply(rep, &msg, item);
    
    zmsg_destroy(&msg);
    return 0;
    
}


PoolFrontend::PoolFrontend(unsigned port) {
    
    mPort = port;
    mDealer = 0;
    mRouter = 0;
    
    mPipe = zactor_new(&PoolFrontend::InvokeProxy, this);
    
}

PoolFrontend::~PoolFrontend() {
    
    zsock_destroy(&mRouter);
    zsock_destroy(&mDealer);
    
    //zsock_wait(mPipe);
    //zactor_destroy(&mPipe);
    
    LogPrintf("PoolFrontend stopped.\n");
    
}


void PoolFrontend::InvokeProxy(zsock_t *pipe, void *arg) {
    
    ((PoolFrontend*)arg)->ProxyLoop(pipe);
    
}

void PoolFrontend::ProxyLoop(zsock_t *pipe) {

    mDealer = zsock_new(ZMQ_DEALER);
    mRouter = zsock_new(ZMQ_ROUTER);
    
    zsock_bind(mDealer, "inproc://frontend");
    unsigned ret = zsock_bind(mRouter, "tcp://*:%d", mPort);
    if(ret != mPort){
        LogPrintf("Frontend: ERROR: zsock_bind failed.\n");
        exit(-1);
    }
    
    void* zmq_socket_r = zsock_resolve(mRouter);
    void* zmq_socket_d = zsock_resolve(mDealer);
    
    LogPrintf("PoolFrontend started at %s on port %d.\n", gArgs.GetArg("-host", "127.0.0.1").c_str(), mPort);
    zsock_signal(pipe, 0);
    
    zmq_proxy(zmq_socket_r, zmq_socket_d, 0);
    
        
    //zsock_destroy(&mRouter);
    //zsock_destroy(&mDealer);
    
    LogPrintf("PoolFrontend proxy loop stopped.\n");
    //zsock_signal(pipe, 0);
    
}




PoolServer::PoolServer(CWallet* pwallet) {
    
    LogPrintf("[PrimeServer] PoolServer starting...\n");
    
    mWallet = pwallet;

    mFrontend = new PoolFrontend(gArgs.GetArg("-frontport", 6666));
    
    mWorkerSignals = zsock_new(ZMQ_PUB);
    zsock_bind(mWorkerSignals, "inproc://bitcoin");
    
    mMinShare = gArgs.GetArg("-minshare", 9); // NOTE: DATACOIN miner optimize? was 8
    mTarget = gArgs.GetArg("-target", 9); // NOTE: DATACOIN miner optimize? was 10
    
    PrimeWorker* worker = new PrimeWorker(mWallet, 0, mTarget);
    zactor_t* pipe = zactor_new(&PrimeWorker::InvokeWork, worker);
    mWorkers.push_back(std::make_pair(worker, pipe));
        
    LogPrintf("[PrimeServer] PoolServer started.\n");
}

PoolServer::~PoolServer(){
    
    LogPrintf("[PrimeServer] PoolServer stopping...\n");
    
    proto::Signal sig;
    sig.set_type(proto::Signal_Type_SHUTDOWN);
    
    SendSignal(sig, mWorkerSignals);

    zsys_interrupted=0;
    for(unsigned i = 0; i < mWorkers.size(); ++i){
        zstr_send(mWorkers[i].second, "$TERM");
        zsock_wait(mWorkers[i].second);
        zactor_destroy(&mWorkers[i].second);
        delete mWorkers[i].first;
    }
    
    zsock_destroy(&mWorkerSignals);
    
    delete mFrontend;
    
    zsys_shutdown();
    
    LogPrintf("[PrimeServer] PoolServer stopped.\n");
    
}


void PoolServer::NotifyNewBlock(CBlockIndex* pindex) {
    
    // TODO: DATACOIN optimize?
    //LogPrintf("[PrimeServer] NotifyNewBlock, height=%d\n", pindex->nHeight);
    
    proto::Signal sig;
    sig.set_type(proto::Signal::NEWBLOCK);
    
    proto::Block* block = sig.mutable_block();
    block->set_height(pindex->nHeight);
    block->set_hash(pindex->phashBlock->GetHex());
    if (pindex->nHeight > 0) {
        block->set_prevhash(pindex->pprev->phashBlock->GetHex());
    } else {
        block->set_prevhash("0");
    }
    block->set_reqdiff(0);
    block->set_minshare(mMinShare);
    
    SendSignal(sig, mWorkerSignals);
    
}


void PoolServer::SendSignal(proto::Signal& sig, zsock_t* socket) {
    
    size_t fsize = sig.ByteSize()+1;
    zframe_t* frame = zframe_new(0, fsize);
    byte* data = zframe_data(frame);
    data[0] = 1;
    sig.SerializeToArray(data+1, fsize-1);
    
    zmsg_t* msg = zmsg_new();
    zmsg_append(msg, &frame);
    zmsg_send(&msg, socket);
    
}


PrimeServer* PrimeServer::CreateServer(CWallet* pwallet) {
    
    return new PoolServer(pwallet);
    
}

