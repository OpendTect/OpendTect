#ifndef similarity_h
#define similarity_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: similarityattrib.h,v 1.1 2005-05-31 12:47:04 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "runstat.h"
#include "valseries.h"
#include "valseriesinterpol.h"
#include "mathfunc.h"

namespace Attrib
{

class ParamSet;

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
    bool		getInputData( const BinID& );
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

