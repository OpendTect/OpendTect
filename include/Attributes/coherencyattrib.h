#ifndef coherencyattrib_h
#define coherencyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: coherencyattrib.h,v 1.3 2005-08-05 10:51:52 cvshelene Exp $
________________________________________________________________________

Coherency type= gate= [maxdip=250] [ddip=10]

Calculates the coherency.

Input:
0       Data

Output:
0       Coherency
1       Inline dip
2       Crossline dip
-*/

#include "attribprovider.h"
#include "valseries.h"
#include "valseriesinterpol.h"
#include "arrayndimpl.h"

namespace Attrib
{

class DataHolder;

class Coherency : public Provider
{
public:
    static void		initClass();
			Coherency( Desc& );

    static const char*	attribName()	{ return "Coherency"; }
    static const char*	typeStr()	{ return "type"; }
    static const char*	gateStr()	{ return "gate"; }
    static const char*	maxdipStr()	{ return "maxdip"; }
    static const char*	ddipStr()	{ return "ddip"; }
    static const char*	stepoutStr()	{ return "stepout"; }

protected:
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool		getInputOutput( int input, TypeSet<int>& res ) const;
    bool		getInputData( const BinID&, int idx );
    bool		computeData( const DataHolder&, const BinID& relpos,
	    			     int t0, int nrsamples ) const;
    bool		computeData1( const DataHolder&, 
	    			     int t0, int nrsamples ) const;
    bool		computeData2( const DataHolder&, 
	    			     int t0, int nrsamples ) const;

    float 		calc1( float s1, float s2, const Interval<int>& sg,
	                       const DataHolder&, const DataHolder&) const;
    float 		calc2( float t, const Interval<int>& rsg,
	                       float, float, const Array2DImpl<DataHolder*>& re,
			       const Array2DImpl<DataHolder*>& im ) const;
	

    const BinID*		reqStepout( int input, int output ) const;
    const Interval<float>*	reqZMargin(int input, int output) const;

    int			type;
    float		maxdip;
    float		ddip;
    BinID		stepout;
    Interval<float>	gate;

    float 		distinl;
    float		distcrl;

    ObjectSet<const DataHolder>	inputdata;
    Array2DImpl<DataHolder*>*	redh;
    Array2DImpl<DataHolder*>*    imdh;

};

}; // namespace Attrib


#endif

