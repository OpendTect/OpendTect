#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.1 2005-02-04 09:32:09 kristofer Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"

class BinID;

namespace Attrib
{
class Processor;
class SliceSet;

class Output
{ mRefCountImpl(Output);
public:
    				Output() {mRefCountConstructor;}
    virtual bool		getDesiredVolume(CubeSampling&) const	= 0;
    virtual bool		wantsOutput( const BinID& ) const	= 0;
    virtual void		getDesiredOutputs( TypeSet<int>& ) const {}
    virtual Interval<int>	getLocalZRange(const BinID&) const	= 0;
    virtual void		collectData(const BinID&,
	    				    const DataHolder& )		= 0;
};


class SliceSetOutput : public Output
{
public:
    			SliceSetOutput( const CubeSampling& );

    SliceSet*		getSliceSet();

    bool		getDesiredVolume(CubeSampling&) const;
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange(const BinID&) const;
    void		collectData(const BinID&, const DataHolder& );
protected:
    CubeSampling	desiredvolume;
    Interval<int>	sampleinterval;
    SliceSet*		sliceset;
};




}; //Namespace


#endif
