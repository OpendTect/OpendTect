#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.2 2005-01-28 16:30:41 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"

class BasicTask;
class CubeSampling;
namespace Threads { class ThreadWorkManager; };

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class Desc;
class Parser;
class ProviderBasicTask;


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

    const Interval<int>*	outputInlStepout() const;
    void			setOutputInlStepout( const Interval<int>& );
    const Interval<int>*	outputCrlStepout() const;
    void			setOutputCrlStepout( const Interval<int>& );
    const Interval<float>*	outputZStepout() const;
    void			setOutputZStepout( const Interval<float>& );

    void			setDesiredVolume( const CubeSampling& );
    bool			getPossibleVolume(int outp,CubeSampling&) const;

    bool			moveToNextTrace();
    BinID			getCurrentPosition() const;
    void			addLocalCompZInterval(const Interval<int>&);
    const Interval<int>&	localCompZInterval() const;
    virtual const DataHolder*	getData( const BinID& relpos );
    virtual const DataHolder*	getDataDontCompute( const BinID& relpos ) const;

protected:
			Provider( Desc& );
    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    virtual bool	getInputData() { return true; }
    virtual bool	computeData( const DataHolder&,
	    			     const BinID& relpos,
	    			     int t1, int nrsamples ) const
    			{ return false; }

    DataHolder*		getDataHolder( const BinID& relpos );
    void		removeDataHolder( const BinID& relpos );

    void		setInput( int input, Provider*, int providerout );
    bool		computeDesInputCube( int inp, int out,
					     CubeSampling& ) const;
    void		updateInputReqs(int input=-1);

    virtual Interval<int>*	desInlMargin(int input, int output) const;
    virtual Interval<int>*	desCrlMargin(int input, int output) const;
    virtual Interval<int>*	reqInlMargin(int input, int output) const;
    virtual Interval<int>*	reqCrlMargin(int input, int output) const;
    virtual Interval<float>*	desZMargin(int input, int output) const;
    virtual Interval<float>*	reqZMargin(int input, int output) const;

    ObjectSet<Provider>	inputs;
    TypeSet<int>	inputprovideroutput;
    Desc&		desc;
    Parser&		parser;

    TypeSet<int>	outputinterest;
    Interval<int>*	outputinlstepout;
    Interval<int>*	outputcrlstepout;
    Interval<float>*	outputzstepout;
    CubeSampling*	desiredvolume;

    Interval<int>	localcomputezinterval;

    Threads::ThreadWorkManager*		threadmanager;
    ObjectSet<BasicTask>		computetasks;
    DataHolderLineBuffer*		linebuffer;

    BinID				currentbid;
};

/*
class StorageProvider : public Provider
{
};


class CalculatingProvider : public Provider
{
};
*/


}; //namespace


#endif

