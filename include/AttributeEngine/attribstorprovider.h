#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "attribprovider.h"
#include "bindatadesc.h"
#include "datapack.h"

class BufferStringSet;
class RegularSeisDataPack;
class SeisMSCProvider;
class SeisTrc;

namespace PosInfo{ class LineSet2DData; }

namespace Attrib
{

class DataHolder;

/*!
\brief Attribute storage provider.
*/

mExpClass(AttributeEngine) StorageProvider : public Provider
{ mODTextTranslationClass(StorageProvider)
public:

    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos=BinID(-1,-1),
	    				bool firstcheck=false);
    bool		getPossibleVolume(int outp,TrcKeyZSampling&);
    BinID		getStepoutStep() const;
    void		updateStorageReqs(bool all=true);
    void		adjust2DLineStoredVolume();
    Pos::GeomID		getGeomID() const;

    void		fillDataPackWithTrc(RegularSeisDataPack*) const;
    bool		needStoredInput() const	{ return true; }
    virtual void	getCompNames(BufferStringSet&) const;
    virtual float	getDistBetwTrcs(bool,const char* linenm =0) const;
    virtual bool	compDistBetwTrcsStats(bool force=false);
    BinID		getElementStepoutStoredSpecial() const;

protected:

    			StorageProvider(Desc&);
    			~StorageProvider();

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDescAndGetCompNms(Desc&,BufferStringSet*);

    bool		getLine2DStoredVolume();
    bool		checkInpAndParsAtStart();
    bool		allowParallelComputation() const { return false; }

    //From disc
    SeisMSCProvider*	getMSCProvider(bool&) const;
    bool		initMSCProvider();
    bool		setMSCProvSelData();

    //From memory (in Attrib::DataPacks)
    SeisTrc*		getTrcFromPack(const BinID&,int) const;
    DataPack::FullID	getDPID() const;

    void		setReqBufStepout(const BinID&,bool wait=false);
    void		setDesBufStepout(const BinID&,bool wait=false);
    bool        	computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume_.zsamp_.step; return true; }
    bool		getZ0StoredData(float& z0) const
			{ z0 = storedvolume_.zsamp_.start; return true; }

    BinDataDesc		getOutputFormat(int output) const;
    
    bool 		checkDesiredTrcRgOK(StepInterval<int>,
	    				    StepInterval<float>);
    bool 		checkDesiredVolumeOK();
    void		checkClassType(const SeisTrc*,BoolTypeSet&) const;
    bool		setTableSelData();
    bool		set2DRangeSelData();

    void		registerNewPosInfo(SeisTrc*,const BinID&,bool,bool&);
    bool                useInterTrcDist() const;
			//to counter impossibility to create a virtual function

    TypeSet<BinDataDesc> datachar_;
    SeisMSCProvider*	mscprov_;
    BinID		stepoutstep_;
    TrcKeyZSampling	storedvolume_;
    bool		isondisc_;
    bool		useintertrcdist_;
    PosInfo::LineSet2DData*  ls2ddata_;

    enum Status        { Nada, StorageOpened, Ready } status_;
};

}; // namespace Attrib

