#ifndef similarityattrib_h
#define similarityattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: similarityattrib.h,v 1.24 2009-05-18 10:33:38 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
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
0	Data
1	Steering

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

mClass Similarity : public Provider
{
public:
    static void			initClass();
				Similarity(Desc&);

    static const char*		attribName()	{ return "Similarity"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		pos0Str()	{ return "pos0"; }
    static const char*		pos1Str()	{ return "pos1"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		steeringStr()	{ return "steering"; }
    static const char*		normalizeStr()	{ return "normalize"; }
    static const char*		extensionStr()	{ return "extension"; }
    static const char*		extensionTypeStr(int);
    void			initSteering();

    void			prepPriorToBoundsCalc();

protected:
    				~Similarity() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;
    bool			getTrcPos();

    BinID			pos0_;
    BinID			pos1_;
    BinID			stepout_;
    Interval<float>		gate_;
    int				extension_;
    TypeSet<BinID>		trcpos_;

    Interval<float>             desgate_;

    bool			dosteer_;
    TypeSet<int>		steerindexes_;
    bool			donormalize_;
    int				dataidx_;
    TypeSet<int>                pos0s_;
    TypeSet<int>                pos1s_;

    ObjectSet<const DataHolder>	inputdata_;
    const DataHolder*		steeringdata_;

    mClass SimiFunc : public FloatMathFunction
    {
    public:
				SimiFunc(const ValueSeries<float>& func, int sz)
					: func_( func )
					, sz_(sz)
					{}
	
	float           	getValue( float x ) const
				{ 
				    ValueSeriesInterpolator<float> interp(sz_);
    //We can afford using extrapolation with polyReg1DWithUdf because even if
    //extrapolation is needed,position will always be close to v0;
    //it is only supposed to be used for extraz compensation
    //in case needinterp==true, DO NOT REMOVE checks on s0&s1 in .cc file!
				    if ( (-1<x && x<0) || (sz_<x && x<sz_+1) )
					interp.extrapol_ = true;
				    return interp.value(func_,x);
				}

    protected:
	const ValueSeries<float>& func_;
	int sz_;
    };
};

} // namespace Attrib


#endif
