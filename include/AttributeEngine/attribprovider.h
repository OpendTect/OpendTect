#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.1 2005-01-26 09:15:22 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"

class CubeSampling;

namespace Attrib
{

class Desc;


class Provider
{  mRefCountImpl(Provider);
public:
    static Provider*		create( Desc& );
				/*!< Creates the provider and all off it's
				     inputs and the input's inputs, and so on */

    const Desc&			getDesc() const;
    Desc&			getDesc();

    void			enableOutput( int output, bool yn=true );

    const Interval<int>*	outputInlStepout() const;
    void			setOutputInlStepout( const Interval<int>& );
    const Interval<int>*	outputCrlStepout() const;
    void			setOutputCrlStepout( const Interval<int>& );
    const Interval<float>*	outputZStepout() const;
    void			setOutputZStepout( const Interval<float>& );

    void		setDesiredVolume( const CubeSampling& );
    bool		getPossibleVolume( int output, CubeSampling& ) const;

    bool		moveToNextTrace();
    BinID		getCurrentPosition() const;
    void		addLocalComputeZInterval( const Interval<float>& );
    Interval<float>*	localComputeZInterval() const;
    float*		computeData( const BinID& relpos );

protected:
			Provider( Desc& );
    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );
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

    TypeSet<int>	outputinterest;
    Interval<int>*	outputinlstepout;
    Interval<int>*	outputcrlstepout;
    Interval<float>*	outputzstepout;
    CubeSampling*	desiredvolume;

    Interval<float>	localcomputezinterval;
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

