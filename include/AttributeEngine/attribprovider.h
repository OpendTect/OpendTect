#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.32 2005-12-20 10:29:55 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"
#include "linekey.h"

class BasicTask;
class CubeSampling;
class SeisRequester;
class SeisSelData;
class SeisTrcInfo;

namespace Threads { class ThreadWorkManager; };

#define mMAXDIP 300 * 1e-6

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class Desc;
class ProviderBasicTask;


/*!\brief provides the actual output to ... */

class Provider
{				mRefCountImpl(Provider);

    friend class		ProviderBasicTask;

public:

    static Provider*		create(Desc&);
				/*!< Also creates all inputs, the input's
				     inputs, and so on */
    virtual bool		isOK() const;

    const Desc&			getDesc() const;
    Desc&			getDesc();
    const DataHolder*		getData(const BinID& relpos=BinID(0,0), 
	    				int idx=0);
    const DataHolder*		getDataDontCompute(const BinID& relpos) const;

    void			enableOutput(int output,bool yn=true);
    bool			isOutputEnabled(int output) const;

    virtual void		setBufferStepout(const BinID&);
    const BinID&		getBufferStepout() const;
    void			setDesiredVolume(const CubeSampling&);
    CubeSampling*		getDesiredVolume() const
				    { return desiredvolume; }
    void			resetDesiredVolume(); 
    void                        setPossibleVolume( const CubeSampling& );
    virtual bool		getPossibleVolume(int outp,CubeSampling&);
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
    int 			comparePosAndAlign(Provider*, bool, Provider*, 
	    					   bool, bool);
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
    void			setSelData(const SeisSelData*);
    void                        setExtraZ(const Interval<float>&);
    float                       getRefStep() const; 
    virtual BinID		getStepoutStep() const;
    BufferString         	errMsg() const;

    ObjectSet<Provider>		getInputs() 		{ return inputs; }

    virtual void		initSteering(){};
    virtual void		initSteering( const BinID& ){};
    void			setOutputInterestSize();
    
    BinID			getTrcInfoBid() const	{ return trcinfobid; }
    
protected:

				Provider(Desc&);
    virtual bool		init();
    				/*!< Should be run _after_ inputs are set */

    virtual SeisRequester*	getSeisRequester() const;
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>&, 
					       bool& issame);

    virtual bool		getInputOutput(int input,TypeSet<int>&) const;
    virtual bool		getInputData(const BinID& relpos,int idx);
    virtual bool		computeData(const DataHolder& output,
					    const BinID& relpos,
					    int t0,int nrsamples) const
				{ return false; }
    int				getDataIndex(int input) const;

    virtual bool		allowParallelComputation() const
    				{ return true; }

    				//DataHolder stuff
    DataHolder*			getDataHolder(const BinID& relpos);
    void			removeDataHolder(const BinID& relpos);
    void			setInput(int input,Provider*);
    void			addParent( Provider* prov ) { parents += prov; }

    bool			computeDesInputCube(int inp,int out,
						    CubeSampling&,
						    bool usestepout=true) const;

    void			setUsedMultTimes();
    bool			isUsedMultTimes()  { return isusedmulttimes; }
    bool			isNew2DLine() const
    				{ return prevtrcnr > currentbid.crl; }

    void			computeRefZStep(const ObjectSet<Provider>&);
    void			propagateZRefStep( const ObjectSet<Provider>& );
    virtual const BinID*	desStepout(int input, int output) const;
    virtual const BinID*	reqStepout(int input, int output) const;
    virtual const Interval<float>* desZMargin(int input, int output) const;
    virtual const Interval<float>* reqZMargin(int input, int output) const;
    virtual bool		getZStepStoredData(float& step) const
				{return false;}

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

    Threads::ThreadWorkManager*	threadmanager;
    ObjectSet<BasicTask>	computetasks;
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
    BufferString 		errmsg;
};


int getSteeringIndex( const BinID& );
//!< For every position there is a single steering index ...?


}; // namespace Attrib

#endif
