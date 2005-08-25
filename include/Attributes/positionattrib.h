#ifndef positionattrib_h
#define positionattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: positionattrib.h,v 1.2 2005-08-25 14:57:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "position.h"
#include "arrayndimpl.h"

/*!\brief Position Attribute

Position stepout=2,2 gate=[0,0] oper=Min|Max steering=

Calculates 'attribute 0' on every position within the cube defined by 'stepout'
and 'gate'. Then determines at which position the oper is valid.
At this position 'attribute 1' is calculated.


Input:
0       Input attribute
1       Output attribute

Output:
0       Value of output attribute on statistical calculated position

*/

namespace Attrib
{

class Position : public Provider
{
public:
    static void		initClass();
			Position( Desc& );

    static const char*	attribName()	{ return "Position"; }
    static const char*	stepoutStr()	{ return "stepout"; }
    static const char*	gateStr()	{ return "gate"; }
    static const char*  operStr()   { return "oper"; }
    static const char*	steeringStr()	{ return "steering"; }
    static const char*	operTypeStr(int);

protected:
    			~Position();
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool		getInputOutput( int input, TypeSet<int>& res ) const;
    bool		getInputData( const BinID&, int idx );
    bool		computeData( const DataHolder&, const BinID& relpos,
	    			     int t0, int nrsamples ) const;

    const BinID*		reqStepout( int input, int output ) const;
    const Interval<float>*	reqZMargin( int input, int output ) const;

    BinID			stepout;
    Interval<float>		gate;
    int				oper;
    bool			steering;

    TypeSet<BinID>              positions;

    int				inidx_;
    int				outidx_;

    ObjectSet<const DataHolder>		inputdata;
    Array2DImpl<const DataHolder*>*    	outdata;
    const DataHolder*			steerdata;
};

}; // namespace Attrib


#endif

