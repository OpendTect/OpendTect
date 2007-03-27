#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.23 2007-03-27 16:30:40 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "cubesampling.h"
#include "seisreq.h"
#include "datachar.h"

namespace Attrib
{

class DataHolder;

class StorageProvider : public Provider
{
public:
    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos = BinID(-1,-1),
	    				bool fisrcheck = false);
    bool		getPossibleVolume(int outp,CubeSampling&);
    BinID		getStepoutStep() const;
    void		updateStorageReqs(bool all=true);
    void		adjust2DLineStoredVolume();
    void		fillDataCubesWithTrc(DataCubes*) const;
    void                setExactZ( TypeSet<float> exactz ) { exactz_ = exactz; }

protected:
    			StorageProvider(Desc&);
    			~StorageProvider();

    static Provider*	createFunc(Desc&);
    static void		updateDesc(Desc&);

    bool		init();
    bool		allowParallelComputation() const { return false; }

    SeisRequester*	getSeisRequester() const;
    bool		initSeisRequester(int req);
    bool		setSeisRequesterSelection(int req);

    void		setReqBufStepout(const BinID&,bool wait=false);
    void		setDesBufStepout(const BinID&,bool wait=false);
    bool        	computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume.zrg.step; return true; }

    BinDataDesc		getOutputFormat(int output) const;
    
    bool 		checkDataOK( StepInterval<int> trcrg,
	                             StepInterval<float>zrg );
    bool 		checkDataOK();

    TypeSet<BinDataDesc> datachar_;
    SeisReqGroup	rg;
    int			currentreq;

    CubeSampling	storedvolume;
    TypeSet<float>	exactz_;	//only used for outputs which require
    					//data at exact z values not placed 
    					//at sample locations 

    enum Status        { Nada, StorageOpened, Ready } status;
};

}; // namespace Attrib

#endif
