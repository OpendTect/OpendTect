#ifndef coherencyattrib_h
#define coherencyattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

Coherency type= gate= [maxdip=250] [ddip=10]

Calculates the coherency.

Input:
0       Data

Output:	3D			2D
0       Coherency		Coherency
1       Inline dip		Trace dip
2       Crossline dip
-*/

#include "attribprovider.h"
#include "valseries.h"
#include "valseriesinterpol.h"
#include "arrayndimpl.h"

namespace Attrib
{

class DataHolder;

mClass Coherency : public Provider
{
public:
    static void		initClass();
			Coherency( Desc& );

    static const char*	attribName()	{ return "Coherency"; }
    static const char*	sKeyType()	{ return "type"; }
    static const char*	sKeyGate()	{ return "gate"; }
    static const char*	sKeyMaxDip()	{ return "maxdip"; }
    static const char*	sKeyDDip()	{ return "ddip"; }
    static const char*	sKeyStepout()	{ return "stepout"; }

    virtual void	prepPriorToBoundsCalc();
    virtual void	prepareForComputeData();

protected:
			~Coherency();
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples,int threadid) const;
    bool		computeData1(const DataHolder&, 
	    			     int t0,int nrsamples) const;
    bool		computeData2(const DataHolder&, 
	    			     int t0,int nrsamples) const;

    float 		calc1(float s1,float s2,const Interval<int>& sg,
	                       const DataHolder&,const DataHolder&) const;
    float 		calc2(float t,const Interval<int>& rsg,
	                      float,float,const Array2DImpl<DataHolder*>& re,
			      const Array2DImpl<DataHolder*>& im) const;

    bool		allowParallelComputation() const { return true; }
	

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;

    int			type_;
    float		maxdip_;
    float		ddip_;
    BinID		stepout_;
    Interval<float>	gate_;
    Interval<float>	desgate_;

    float 		distinl_;
    float		distcrl_;

    ObjectSet<const DataHolder>	inputdata_;
    Array2DImpl<DataHolder*>*	realdataholder_;
    Array2DImpl<DataHolder*>*   imagdataholder_;

    int			realidx_;
    int			imagidx_;
};

}; // namespace Attrib


#endif

