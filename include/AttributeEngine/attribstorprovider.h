#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "datachar.h"
class BufferStringSet;
class SeisMSCProvider;
class SeisTrc;

namespace Attrib
{

class DataHolder;

/*!
  \ingroup AttributeEngine
  \brief Attribute storage provider.
*/

mClass(AttributeEngine) StorageProvider : public Provider
{
public:

    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos=BinID(-1,-1),
	    				bool firstcheck=false);
    bool		getPossibleVolume(int outp,CubeSampling&);
    BinID		getStepoutStep() const;
    void		compDistBetwTrcsStats(
	    				TypeSet< LineTrcDistStats >&) const;
    void		updateStorageReqs(bool all=true);
    void		adjust2DLineStoredVolume();
    PosInfo::GeomID	getGeomID() const;

    void		fillDataCubesWithTrc(DataCubes*) const;
    bool		needStoredInput() const	{ return true; }
    virtual void	getCompNames(BufferStringSet&) const;

protected:

    			StorageProvider(Desc&);
    			~StorageProvider();

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDescAndGetCompNms(Desc&,BufferStringSet*);

    bool		checkInpAndParsAtStart();
    bool		allowParallelComputation() const { return false; }

    //From disc
    SeisMSCProvider*	getMSCProvider(bool&) const;
    bool		initMSCProvider();
    bool		setMSCProvSelData();

    //From memory (in Attrib::DataPacks)
    SeisTrc*		getTrcFromPack(const BinID&,int) const;

    void		setReqBufStepout(const BinID&,bool wait=false);
    void		setDesBufStepout(const BinID&,bool wait=false);
    bool        	computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume_.zrg.step; return true; }

    BinDataDesc		getOutputFormat(int output) const;
    
    bool 		checkDesiredTrcRgOK(StepInterval<int>,
	    				    StepInterval<float>);
    bool 		checkDesiredVolumeOK();
    void		checkClassType(const SeisTrc*,BoolTypeSet&) const;
    bool		setTableSelData();
    bool		set2DRangeSelData();

    void		registerNewPosInfo(SeisTrc*,const BinID&,bool,bool&);
    bool                useInterTrcDist() const;

    TypeSet<BinDataDesc> datachar_;
    SeisMSCProvider*	mscprov_;
    BinID		stepoutstep_;
    CubeSampling	storedvolume_;
    bool		isondisc_;
    bool		useintertrcdist_;

    enum Status        { Nada, StorageOpened, Ready } status_;
};

}; // namespace Attrib

#endif

