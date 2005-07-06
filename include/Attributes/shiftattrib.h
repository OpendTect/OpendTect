#ifndef hash_h
#define hash_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: shiftattrib.h,v 1.1 2005-07-06 15:02:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "position.h"

#include <limits.h>

/*!\brief Hash Attribute

  Hash pos= steering=Yes/No

  Hash takes the input at a specified position and outputs it at other
  relative positions.

Input:
0 - attrib to be hashed
1 - steering (optional)

Output
0 - hashed attrib

*/

namespace Attrib
{

class Hash : public Provider
{
public:
    static void		initClass();
			Hash( Desc& );

    static const char*	attribName()	{ return "Hash"; }
    static const char*	posStr()	{ return "pos"; }
    static const char*	timeStr()	{ return "time"; }
    static const char*	steeringStr()	{ return "steering"; }

protected:
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool		getInputOutput( int input, TypeSet<int>& res ) const;
    bool		getInputData( const BinID&, int idx );
    bool		computeData( const DataHolder&, const BinID& relpos,
	    			     int t0, int nrsamples ) const;

    const BinID*		reqStepout( int input, int output ) const;
    const Interval<float>*      desZMargin(int input, int output) const;

    BinID			pos;
    float 			time;
    bool			steering;
    
    BinID			stepout;
    Interval<float>		interval;

    const DataHolder*		inputdata;
    const DataHolder*		steeringdata;
};

}; // namespace Attrib


#endif

