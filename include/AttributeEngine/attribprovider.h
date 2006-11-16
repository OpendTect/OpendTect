#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.53 2006-11-16 15:22:28 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"
#include "linekey.h"

class CubeSampling;
class SeisRequester;
class SeisSelData;
class SeisTrcInfo;
template <class T> class Array2DImpl;
class BinDataDesc;

#define mMAXDIP 300 * 1e-6
#define mMAXDIPSECURE 2000 * 1e-6

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class Desc;
class ProviderTask;


/*!\brief provides the actual output to ... */

class Provider
{				mRefCountImpl(Provider);

    friend class		ProviderTask;

public:

    static Provider*		create(Desc&,BufferString&);
				/*!< Also creates all inputs, the input's
				     inputs, and so on */
    virtual bool		isOK() const;

    const Desc&			getDesc() const;
    Desc&			getDesc();
    const DataHolder*		getData(const BinID& relpos=BinID(0,0), 
	    				int idx=0);
    const DataHolder*		getDataDontCompute(const BinID& relpos) const;

    int				nrOutputs() const
    				{ return outputinterest.size(); }
    void			enableOutput(int output,bool yn=true);
    bool			isOutputEnabled(int output) const;

    virtual void		setBufferStepout(const BinID&);
    const BinID&		getBufferStepout() const;
    void			setDesiredVolume(const CubeSampling&);
    				/*!< The desired volume is the ideal volume
				  required by the user*/
    CubeSampling*		getDesiredVolume() const
				{ return desiredvolume; }
    void			resetDesiredVolume(); 
    void                        setPossibleVolume( const CubeSampling& );
    				/*!< The possible volume is the volume that can
				  really be computed taking care of all margins
				  and stepouts*/
    virtual bool		getPossibleVolume(int outp,CubeSampling&);
    CubeSampling*		getPossibleVolume() const
    				{ return possiblevolume; }
    int				getTotalNrPos(bool);
    void			setCurLineKey( const char* linename ); 
    virtual void		adjust2DLineStoredVolume(bool adjuststep=false);
    
    virtual int			moveToNextTrace(BinID startpos = BinID(-1,-1),
	    					bool firstcheck = false);
    				/*!<\retval -1	something went wrong
				    \retval  0  finished, no more positions
				    \retval  1	arrived at new position
				*/
    void			computeNewStartPos(BinID&);
    int				alignInputs(ObjectSet<Provider>&);
    int 			comparePosAndAlign(Provider*,bool,Provider*, 
	    					   bool,bool);
    void			resetMoved();
    void                        resetZIntervals();

    virtual const SeisTrcInfo*	getCurrentTrcInfo() const {return curtrcinfo_;}
    BinID			getCurrentPosition() const;
    virtual bool		setCurrentPosition(const BinID&);
    void			updateCurrentInfo();

    void			addLocalCompZIntervals(
						const TypeSet<Interval<int> >&);
    const TypeSet< Interval<int> >&	localCompZIntervals() const;

    void               		updateInputReqs(int input=-1);
    virtual void                updateStorageReqs(bool all=false);
    void			setNeedInterpol(bool);
    void			setSelData(const SeisSelData*);
    void                        setExtraZ(const Interval<float>&);
    float                       getRefStep() const; 
    virtual BinID		getStepoutStep() const;
    BufferString         	errMsg() const;

    ObjectSet<Provider>&	getInputs() 		{ return inputs; }

    virtual void		initSteering()			{}
    virtual void		initSteering(const BinID&)	{}
    void			setOutputInterestSize();
    
    BinID			getTrcInfoBid() const	{ return trcinfobid; }

    virtual void		prepareForComputeData();
    				/*!< Everything is known now. */
    static const char*		prepare(Desc&);
    				//!< Must be called before getting
    				//!< inputs/outputs etc. from a Desc

protected:

				Provider(Desc&);
    virtual bool		init();
    				/*!< Should be run _after_ inputs are set */

    virtual SeisRequester*	getSeisRequester() const;
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>&, 
					       bool& issame,BufferString&);
    				/*!< Creates the provider needed and all its
				  input providers*/

    virtual bool		getInputOutput(int input,TypeSet<int>&) const;
    				/*!<Specifies the outputs needed for calculation
				among all those provided by the input data; 
				very usefull when steering used as input data*/
    virtual bool		getInputData(const BinID& relpos,int idx);
    				/*!<Gets all imput data, 
				including data for which a stepout is required*/
    virtual bool		computeData(const DataHolder& output,
					    const BinID& relpos,
					    int t0,int nrsamples) const
				{ return false; }
    				/*!<The system will use the algorithm specified
				in this function to compute the attribute's 
				outputs. The results will be stored as 
				different series in the DataHolder output. */
    int				getDataIndex(int input) const;
    				/*!<Gets the index of the serie needed in the 
				input DataHolder*/
    void			fillInputRangesArray(
					    Array2DImpl< Interval<int> >&,
					    int,const Interval<int>&);

    				// MultiThreading stuff
    virtual bool		allowParallelComputation() const
    				{ return false; }
    virtual int			minTaskSize() const		{ return 25; }

    				// DataHolder stuff
    DataHolder*			getDataHolder(const BinID& relpos);
    void			removeDataHolder(const BinID& relpos);
    void			setInput(int input,Provider*);
    void			addParent( Provider* prov ) { parents += prov; }
    virtual BinDataDesc		getOutputFormat(int output) const;

    bool			computeDesInputCube(int inp,int out,
						    CubeSampling&,
						    bool usestepout=true) const;
    				/*!<The system uses the margin and stepout
				requirements to compute the ideal desired 
				volume for each input*/

    void			setUsedMultTimes();
    				/*!<The same provider can be used multiple times
				which allows the attribute to be computed 
				only once*/
    bool			isUsedMultTimes()  { return isusedmulttimes; }
    bool			isNew2DLine() const
    				{ return prevtrcnr > currentbid.crl; }

    void			computeRefStep(const ObjectSet<Provider>&);
    				/*!<If an attribute uses as inputs stored cubes
				with a different z step the smallest one will 
				be taken as reference step*/
    void			propagateRefStep(const ObjectSet<Provider>&);

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
    virtual bool		getZStepStoredData(float& step) const
				{return false;}

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

    bool                        zIsTime() const;
    float			zFactor() const   {return zIsTime() ? 1000 : 1;}
    float			dipFactor() const {return zIsTime() ? 1e6: 1e3;}
    float			inldist() const; 
    float			crldist() const;

    ObjectSet<Provider>		inputs;
    ObjectSet<Provider>		parents;
    Desc&			desc;
    TypeSet<int>		outputinterest;
    BinID			bufferstepout;
    CubeSampling*		desiredvolume;
    CubeSampling*               possiblevolume;
    TypeSet< Interval<int> >	localcomputezintervals;
    ObjectSet<Provider>		allexistingprov;

    ProviderTask*		providertask_;
    DataHolderLineBuffer*	linebuffer;
    BinID			currentbid;
    int				prevtrcnr;
    LineKey			curlinekey_;
    const SeisSelData*		seldata_;
    Interval<float>     	extraz_;
    const SeisTrcInfo*		curtrcinfo_;
    BinID                       trcinfobid;

    float                       refstep;
    bool 			alreadymoved;

    bool			isusedmulttimes;
    bool			needinterp;
    BufferString 		errmsg;
};


int getSteeringIndex( const BinID& );
//!< For every position there is a single steering index ...?



}; // namespace Attrib

#define mAttrDefCreateInstance(clss) \
Provider* clss::createInstance( Desc& desc ) \
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
    Desc* desc = new Desc( attribName()); \
    desc->ref();

#define mAttrStartInitClassWithUpdate \
    Desc* desc = new Desc( attribName(), updateDesc ); \
    desc->ref();

#define mAttrEndInitClass \
    PF().addDesc( desc, createInstance ); \
    desc->unRef();

#endif
