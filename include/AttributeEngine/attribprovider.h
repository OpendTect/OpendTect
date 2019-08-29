#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "posinfo2dsurv.h"
#include "fullsubsel.h"
#include "uistring.h"

class BinDataDesc;
class RegularSeisDataPack;
class SeisTrcInfo;
namespace Seis { class MSCProvider; class SelData; }
template <class T> class Array2DImpl;

#define mMAXDIP 300 * 1e-6
#define mMAXDIPSECURE 2000 * 1e-6
#define mMAXDIPSECUREDEPTH 6

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class ProviderTask;

/*!\brief Provides the actual output to clients */

mExpClass(AttributeEngine) Provider : public RefCount::Referenced
{   mODTextTranslationClass(Attrib::Provider)
public:

    mUseType( Pos,		GeomID );
    mUseType( Seis,		MSCProvider );
    mUseType( Seis,		SelData );
    mUseType( Survey,		FullSubSel );
    mUseType( Survey,		GeomSubSel );

    static Provider*		create(Desc&,uiRetVal&);
    uiRetVal			status() const	{ return uirv_; }
    inline bool			isOK() const	{ return uirv_.isOK(); }
    virtual uiRetVal		isActive() const { return uiRetVal::OK(); }
    bool			is2D() const;

    const Desc&			getDesc() const		{ return desc_; }
    Desc&			getDesc()		{ return desc_; }
    const DataHolder*		getData(const BinID& relpos=BinID(0,0),
					int idx=0);
    const DataHolder*		getDataDontCompute(const BinID& relpos) const;

    int				nrOutputs() const
				{ return outputinterest_.size(); }
    void			enableOutput(int output,bool yn=true);
    bool			isOutputEnabled(int output) const;
    void			setOutputInterestSize(bool preserve=false);
    void			enableAllOutputs(bool yn=true);
    virtual void		getCompNames(BufferStringSet&) const;

    virtual bool		isSingleTrace() const;
    virtual bool		usesTracePosition() const;
    virtual void		setReqBufStepout(const BinID&,bool wait=false);
    virtual void		setDesBufStepout(const BinID&,bool wait=false);
    const BinID&		getReqBufStepout() const
				{ return reqbufferstepout_; }
    const BinID&		getDesBufStepout() const
				{ return desbufferstepout_; }
    void			setDesiredSubSel(const FullSubSel&);
    void			setDesiredSubSel(const GeomSubSel&);
    const FullSubSel&		desiredSubSel() const
				{ return desiredsubsel_; }
    void			resetDesiredSubSel();
    virtual bool		calcPossibleSubSel(int outp,
					const FullSubSel& desiredsubsel);
    void			setPossibleSubSel(const FullSubSel&);
    const FullSubSel&		possibleSubSel() const
				{ return possiblesubsel_; }
    int				totalNrPos();
    virtual void		adjust2DLineStoredSubSel();
    virtual GeomID		geomID() const;
    virtual void		setGeomID(GeomID);

    virtual int			moveToNextTrace(BinID startpos = BinID(-1,-1),
						bool firstcheck = false);
				/*!<\retval -1	something went wrong
				    \retval  0  finished, no more positions
				    \retval  1	arrived at new position
				*/
    void			computeNewStartPos(BinID&);
    int				alignInputs(ObjectSet<Provider>&);
    int				comparePosAndAlign(Provider*,bool,Provider*,
						   bool,bool);
    void			resetMoved();
    void                        resetZIntervals();

    virtual const SeisTrcInfo*	getCurrentTrcInfo() const {return curtrcinfo_;}
    BinID			getCurrentPosition() const;
    virtual bool		setCurrentPosition(const BinID&);
    void			updateCurrentInfo();
    virtual bool		needStoredInput() const;

    void			addLocalCompZIntervals(
						const TypeSet<Interval<int> >&);
    const TypeSet< Interval<int> >& localCompZIntervals() const;

    void			updateInputReqs(int input=-1);
    virtual void                updateStorageReqs(bool all=false);
    void			setUseSC()		{ useshortcuts_ = true;}
    void			setSelData(const SelData*);
    void                        setExtraZ(const Interval<float>&);
    void			setNeedInterpol(bool);
    void			setExactZ(const TypeSet<float>&);

    void			computeRefZStep();
				/*!<If an attribute uses as inputs stored cubes
				with a different z step the smallest one will
				be taken as reference step*/
    void			setRefZStep(float step);
    float                       refZStep() const;

    void			computeRefZ0();
				/*!<If an attribute uses as inputs stored cubes
				with a different z0 the smallest one will
				be taken as reference z0*/
    void			setRefZ0(float z0);
    float                       refZ0() const	{ return refz0_; }

    virtual BinID		getStepoutStep() const;
    ObjectSet<Provider>&	getInputs()		{ return inputs_; }
    BinID			getTrcInfoBid() const	{ return trcinfobid_; }
    uiRetVal			errMsg() const;

    virtual void		initSteering()				{}
    virtual void		prepSteeringForStepout(const BinID&)	{}

    virtual bool		prepPriorToOutputSetup();
				/*!< returns whether the outputs plan acquired
				  from the parameter file has to be overruled */
    virtual void		prepPriorToBoundsCalc();
				/*!< Z refzstep is known now,
				  this is meant to be used before possible-
				  and desired- volumes are computed*/
    virtual void		prepareForComputeData();
				/*!< Everything is known now. */
    static uiRetVal		prepare(Desc&);
				//!< Must be called before getting
				//!< inputs/outputs etc. from a Desc
    virtual void		fillDataPackWithTrc(RegularSeisDataPack*) const
				{}

    virtual void		setRdmPaths(const TypeSet<BinID>& truepath,
					    const TypeSet<BinID>& snappedpath);
				//!<Optional, needed by directional attributes

				//!<Special case for attributes (like PreStack)
				//!<which inputs are not treated as normal
				//!<input cubes and thus not delivering
				//!<adequate cs automaticly
    virtual void		updateSSIfNeeded(FullSubSel&) const {}
    virtual bool		compDistBetwTrcsStats(bool force=false);
    float			getApplicableCrlDist(bool) const;
    virtual float		getDistBetwTrcs(bool ismax) const;

    void			setDataUnavailableFlag(bool yn) const;
    bool			getDataUnavailableFlag() const;

protected:

				Provider(Desc&);
                                ~Provider();

    virtual bool		checkInpAndParsAtStart();
				/*!< Should be used for check _after_ inputs
				  are set, for extra checks at other time
				  use isOK()*/

    virtual MSCProvider*	getMSCProvider(bool&) const;
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>&,
					       bool& issame,uiRetVal&);
				/*!< Creates the provider needed and all its
				  input providers*/

    virtual bool		getInputOutput(int input,TypeSet<int>&) const;
				/*!<Specifies the outputs needed for calculation
				among all those provided by the input data;
				very usefull when steering used as input data*/
    virtual bool		getInputData(const BinID& relpos,int idx);
				/*!<Gets all imput data,
				including data for which a stepout is required*/
    virtual bool		preProcCommonToAllThreads(const DataHolder& out,
							  const BinID& relpos)
				{ return true; }
				/*!<Computes things that are common to all
				  threads; typically initialization of class
				  variables which rely on input data
				  (and thus cannot be inited before in other
				  prep* functions)*/
    virtual bool		computeData(const DataHolder& output,
					    const BinID& relpos,
					    int t0,int nrsamples,
					    int threadidx) const	= 0;
				/*!<The system will use the algorithm specified
				in this function to compute the attribute's
				outputs. The results will be stored as
				different series in the DataHolder output.
				 \param threadid thread identifier that may
				  be handy when using multiple threads. */
    int				getDataIndex(int input) const;
				/*!<Gets the index of the serie needed in the
				input DataHolder*/
    void			fillInputRangesArray(
					    Array2DImpl< BasicInterval<int> >&,
					    int,const BasicInterval<int>&);

				// MultiThreading stuff
    virtual bool		allowParallelComputation() const
				{ return false; }
    virtual bool		setNrThreads( int idx )	{ return true; }
    virtual int			minTaskSize() const		{ return 25; }
    virtual bool		finalizeCalculation(bool scs)	{ return scs; }
				/*!<Called one all computeData have returned.
				    \param scs is true if all computeData
					   were successful. */

				// DataHolder stuff
    DataHolder*			getDataHolder(const BinID& relpos);
    void			removeDataHolder(const BinID& relpos);
    void			setInput(int input,Provider*);
    void			addParent( Provider* prov ) { parents_+=prov; }
    virtual BinDataDesc		getOutputFormat(int output) const;
    virtual bool		doNotReUseDH() const		{ return false;}

    void			computeDesInputSubSel(int inp,int out,
						    FullSubSel&,
						    bool usestepout=true) const;
				/*!<The system uses the margin and stepout
				requirements to compute the ideal desired
				volume for each input*/

    void			setUsedMultTimes();
				/*!<The same provider can be used multiple times
				which allows the attribute to be computed
				only once*/
    bool			isUsedMultTimes()  { return isusedmulttimes_; }
    bool			isNew2DLine() const
				{ return prevtrcnr_ > currentbid_.crl(); }

    virtual const BinID*	desStepout(int input,int output) const;
				/*!<The system will
				    try to supply the attribute with the
				    desired stepout, but will not take the
				    desired stepout into account when computing
				    the volume that can be computed.
				    \returns the desired stepout on \a input if
				    \a output is going to be computed, or NULL
				    if no stepout is desired. */
    virtual const BinID*	reqStepout(int input,int output) const;
				/*!<The system
				    will supply the attribute with the required
				    stepout, and use it to compute the volume
				    that can be computed.
				    \returns the required stepout on \a input if
				    \a output is going to be computed, or NULL
				    if no stepout is required.  */
    virtual const Interval<float>* desZMargin(int input,int output) const;
				/*!<The gate is specified in
				    the depth-unit of the survey (e.g. seconds,
				    feet or meters), and is in addition to
				    the gate specified in desZSampMargin. The
				    system will try to supply the attribute with
				    the desired gate, but will not take the
				    desired gate into account when computing
				    the volume that can be computed.
				    \returns the extra gate desired on
				    \a input if \a output is going to be
				    computed, or NULL if no extra gate
				    is desired. */
    virtual const Interval<float>* reqZMargin(int input,int output) const;
				 /*<The gate is specified in
				    the depth-unit of the survey (e.g. seconds,
				    feet or meters), and is in addition to
				    the gate specified in reqZSampMargin. The
				    system will supply the attribute with the
				    required gate, and use it to compute the
				    volume that can be computed.
				    \returns the extra gate required on
				    \a input if \a output is going to be
				    computed, or NULL if no extra gate
				    is required. */
    virtual const Interval<int>* desZSampMargin(int input,int output) const;
				/*!<The gate is specified in
				    the trace samples, and is in addition to
				    the gate specified in desZMargin. The system
				    will try to supply the attribute with the
				    desired gate, but will not take the
				    desired gate into account when computing
				    the volume that can be computed.
				    \returns the extra gate desired on
				    \a input if \a output is going to be
				    computed, or NULL if no extra gate
				    is desired. */
    virtual const Interval<int>* reqZSampMargin(int input,int output) const;
				/*!<The gate is specified in
				    the trace samples, and is in addition to
				    the gate specified in reqZMargin. The system
				    will supply the attribute with the required
				    gate, and use it to compute the volume
				    that can be computed.
				    \returns the extra gate required on
				    \a input if \a output is going to be
				    computed, or NULL if no extra gate
				    is required. */
    virtual float		zStepStoredData() const
				{ return mUdf(float); }
    virtual float		z0StoredData() const
				{ return mUdf(float); }

    float			getInterpolInputValue(const DataHolder&,
						      int inputidx,
						      float zval) const;
    float			getInterpolInputValue(const DataHolder&,
						      int inputidx,float sample,
						      int z0) const;
    float			getInputValue(const DataHolder&,int inputidx,
					      int sampleidx,int z0) const;
    void			setOutputValue(const DataHolder& output,
					       int outputidx,int sampleidx,
					       int z0,float val) const;
    float			getExtraZFromSampPos(float) const;
    float			getExtraZFromSampInterval(int,int) const;
    virtual bool		useInterTrcDist() const;
    float			zAtSample( int idx ) const
				{ return refz0_ + idx*refzstep_; }

    bool                        zIsTime() const;
    float			zFactor() const;
    float			dipFactor() const;
    float			inlDist() const;
    float			crlDist() const;
    float			lineDist() const;
    float			trcDist() const;
    float			maxSecureDip() const
				{ return (float) (zIsTime() ? mMAXDIPSECURE
						   : mMAXDIPSECUREDEPTH); }
    void			stdPrepSteering(const BinID&);
    void			applyMargins(const Interval<float>*,
				     const Interval<int>*,FullSubSel&) const;

    ObjectSet<Provider>		inputs_;
    ObjectSet<Provider>		parents_;
    Desc&			desc_;
    TypeSet<int>		outputinterest_;
    BinID			desbufferstepout_;
    BinID			reqbufferstepout_;
    FullSubSel			desiredsubsel_;
    FullSubSel			possiblesubsel_;
    TypeSet< Interval<int> >	localcomputezintervals_;
    ObjectSet<Provider>		allexistingprov_;
    TypeSet<float>		exactz_;//only used for outputs which require
                                        //data at exact z values not placed
                                        //at sample locations

    ProviderTask*		providertask_;
    DataHolderLineBuffer*	linebuffer_;
    BinID			currentbid_;
    int				prevtrcnr_;
    const SelData*		seldata_;
    Interval<float>		extraz_;
    const SeisTrcInfo*		curtrcinfo_;
    BinID                       trcinfobid_;
    bool			useshortcuts_;

    float			refz0_;
    float                       refzstep_;
    bool			alreadymoved_;

    bool			isusedmulttimes_;
    bool			needinterp_;
    mutable uiRetVal		uirv_;
    bool			dataunavailableflag_;

    friend class		ProviderTask;

};


mGlobal(AttributeEngine) int getSteeringIndex(const BinID&);
//!< For every position there is a single steering index ...?



}; // namespace Attrib

#define mAttrDefCreateInstance(clss) \
Attrib::Provider* clss::createInstance( Attrib::Desc& desc ) \
{ \
    clss* res = new clss( desc ); \
    res->ref(); \
    if ( !res->isOK() ) \
    { \
	res->unRef(); \
	return 0; \
    } \
 \
    res->unRefNoDelete(); \
    return res; \
}

#define mAttrStartInitClass \
    Attrib::Desc* desc = new Attrib::Desc( attribName()); \
    desc->ref();

#define mAttrStartInitClassWithUpdate \
    Attrib::Desc* desc = new Attrib::Desc( attribName(), updateDesc ); \
    desc->ref();

#define mAttrStartInitClassWithDescAndDefaultsUpdate \
    Attrib::Desc* desc = new Attrib::Desc( attribName(), updateDesc,\
					   updateDefaults ); \
    desc->ref();

#define mAttrStartInitClassWithDefaultsUpdate \
    Attrib::Desc* desc = new Attrib::Desc( attribName(), 0,updateDefaults ); \
    desc->ref();

#define mAttrEndInitClass \
    Attrib::PF().addDesc( desc, createInstance ); \
    desc->unRef();
