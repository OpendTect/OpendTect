#ifndef similarity_h
#define similarity_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: similarityattrib.h,v 1.4 2005-07-06 15:02:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "runstat.h"
#include "valseries.h"
#include "valseriesinterpol.h"
#include "mathfunc.h"

/*!\brief Similarity Attribute

Similarity gate= pos0= pos1= stepout=1,1 extension=[0|90|180|Cube]
	 [steering=Yes|No]

Calculates the gates' distance between each other in hyperspace normalized
to the gates' lengths.

If steering is enabled, it is up to the user to make sure that the steering
goes to the same position as pos0 and pos1 respectively.

Input:
0       Data
1-      Steerings

Extension:      0       90/180          Cube
1               pos0    pos0
2               pos1    pos1
3                       pos0rot
4                       pos1rot

Output:
0       Avg
1       Med
2       Var
3       Min
4       Max

*/

namespace Attrib
{

class Similarity : public Provider
{
public:
    static void		initClass();
			Similarity( Desc& );

    static const char*	attribName()	{ return "Similarity"; }
    static const char*	gateStr()	{ return "gate"; }
    static const char*	pos0Str()	{ return "pos0"; }
    static const char*	pos1Str()	{ return "pos1"; }
    static const char*	stepoutStr()	{ return "stepout"; }
    static const char*	steeringStr()	{ return "steering"; }
    static const char*	normalizeStr()	{ return "normalize"; }
    static const char*	extensionStr()	{ return "extension"; }
    static const char*	extensionTypeStr(int);

protected:
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool		getInputOutput( int input, TypeSet<int>& res ) const;
    bool		getInputData( const BinID&, int idx );
    bool		computeData( const DataHolder&, const BinID& relpos,
	    			     int t0, int nrsamples ) const;

    const BinID*		reqStepout( int input, int output ) const;
    const Interval<float>*	reqZMargin(int input, int output) const;

    BinID			pos0;
    BinID			pos1;
    BinID			stepout;
    Interval<float>		gate;
    int				extension;

    bool			dosteer;
    TypeSet<int>		steeridx;
    bool			donormalize;

    ObjectSet<const DataHolder>	inputdata;
    const DataHolder*		steeringdata;

    class SimiFunc : public FloatMathFunction
    {
    public:
				SimiFunc(const ValueSeries<float>& func)
					:func_( func ){}
	
	float           	getValue( float x ) const
				{ 
				    ValueSeriesInterpolator<float> interp;
				    return interp.value(func_,x);
				}

    protected:
	const ValueSeries<float>& func_;
    };
};

}; // namespace Attrib


#endif

