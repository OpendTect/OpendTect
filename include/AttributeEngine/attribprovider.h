#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.6 2005-02-04 09:28:35 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"

class BasicTask;
class CubeSampling;
class SeisRequester;
namespace Threads { class ThreadWorkManager; };

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class Desc;
class ProviderBasicTask;


int getSteeringIndex( const BinID& );


class Provider
{  mRefCountImpl(Provider);
    friend			class ProviderBasicTask;
public:
    static Provider*		create( Desc& );
				/*!< Creates the provider and all off it's
				     inputs and the input's inputs, and so on */
    virtual bool		isOK() const;

    const Desc&			getDesc() const;
    Desc&			getDesc();

    void			enableOutput( int output, bool yn=true );

    void			setBufferStepout( const BinID& );
    const BinID&		getBufferStepout() const;

    void			setDesiredVolume( const CubeSampling& );
    virtual bool		getPossibleVolume(int outp,CubeSampling&) const;

    virtual int			moveToNextTrace();
    BinID			getCurrentPosition() const;
    virtual bool		setCurrentPosition( const BinID& );
    void			addLocalCompZInterval(const Interval<int>&);
    const Interval<int>&	localCompZInterval() const;
    virtual const DataHolder*	getData( const BinID& relpos=BinID(0,0) );
    virtual const DataHolder*	getDataDontCompute( const BinID& relpos ) const;

protected:
				Provider( Desc& );
    virtual bool		init();
    				/*!< Should be run _after_ inputs are set */
    virtual SeisRequester*	getSeisRequester();
    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );


    virtual bool	getInputOutput( int input, TypeSet<int>& ) const;

    virtual bool	getInputData( const BinID& relpos );
    virtual bool	computeData( const DataHolder&,
	    			     const BinID& relpos,
	    			     int t1, int nrsamples ) const
    			{ return false; }

    DataHolder*		getDataHolder( const BinID& relpos );
    void		removeDataHolder( const BinID& relpos );

    void		setInput( int input, Provider* );
    bool		computeDesInputCube( int inp, int out,
					     CubeSampling& ) const;
    void		updateInputReqs(int input=-1);

    virtual const BinID*		desStepout(int input, int output) const;
    virtual const BinID*		reqStepout(int input, int output) const;
    virtual const Interval<float>*	desZMargin(int input, int output) const;
    virtual const Interval<float>*	reqZMargin(int input, int output) const;

    ObjectSet<Provider>			inputs;
    Desc&				desc;

    TypeSet<int>			outputinterest;
    BinID				bufferstepout;
    CubeSampling*			desiredvolume;

    Interval<int>			localcomputezinterval;

    Threads::ThreadWorkManager*		threadmanager;
    ObjectSet<BasicTask>		computetasks;
    DataHolderLineBuffer*		linebuffer;

    BinID				currentbid;
};


}; //namespace


#endif

