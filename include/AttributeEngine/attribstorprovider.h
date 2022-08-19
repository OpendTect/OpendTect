#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    static const char*	attribName()		{ return "Storage"; }
    static const char*	keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos=BinID(-1,-1),
					bool firstcheck=false) override;
    bool		getPossibleVolume(int outp,TrcKeyZSampling&) override;
    BinID		getStepoutStep() const override;
    void		updateStorageReqs(bool all=true) override;
    void		adjust2DLineStoredVolume() override;
    Pos::GeomID		getGeomID() const override;

    void		fillDataPackWithTrc(
					RegularSeisDataPack*) const override;
    bool		needStoredInput() const override	{ return true; }
    void		getCompNames(BufferStringSet&) const override;
    float		getDistBetwTrcs(bool,
					const char* linenm =0) const override;
    bool		compDistBetwTrcsStats(bool force=false) override;
    BinID		getElementStepoutStoredSpecial() const;

protected:

			StorageProvider(Desc&);
			~StorageProvider();

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDescAndGetCompNms(Desc&,BufferStringSet*);

    bool		getLine2DStoredVolume();
    bool		checkInpAndParsAtStart() override;
    bool		allowParallelComputation() const override
			{ return false; }

    //From disc
    SeisMSCProvider*	getMSCProvider(bool&) const override;
    MultiID		getDBKey(const Desc* =nullptr) const;
    bool		initMSCProvider();
    bool		setMSCProvSelData();

    //From memory (in Attrib::DataPacks)
    SeisTrc*		getTrcFromPack(const BinID&,int) const;
    DataPack::FullID	getDPID(const Desc* =nullptr) const;

    void		setReqBufStepout(const BinID&,bool wait=false) override;
    void		setDesBufStepout(const BinID&,bool wait=false) override;
    bool		computeData(const DataHolder& output,
				    const BinID& relpos,int t0,
				    int nrsamples,int threadid) const override;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const override
			{ step = storedvolume_.zsamp_.step; return true; }
    bool		getZ0StoredData(float& z0) const override
			{ z0 = storedvolume_.zsamp_.start; return true; }

    BinDataDesc		getOutputFormat(int output) const override;

    bool		checkDesiredTrcRgOK(StepInterval<int>,
					    StepInterval<float>);
    bool		checkDesiredVolumeOK();
    void		checkClassType(const SeisTrc*,BoolTypeSet&) const;
    bool		setTableSelData();
    bool		set2DRangeSelData();

    void		registerNewPosInfo(SeisTrc*,const BinID&,bool,bool&);
    bool		useInterTrcDist() const override;
			//to counter impossibility to create a virtual function

    TypeSet<BinDataDesc> datachar_;
    SeisMSCProvider*	mscprov_ = nullptr;
    BinID		stepoutstep_;
    TrcKeyZSampling	storedvolume_;
    bool		isondisc_;
    bool		useintertrcdist_ = false;
    PosInfo::LineSet2DData*  ls2ddata_ = nullptr;

    enum Status        { Nada, StorageOpened, Ready } status_ = Nada;
};

} // namespace Attrib
