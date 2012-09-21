#ifndef scalingattrib_h
#define scalingattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          December 2004
 RCS:           $Id$
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"

/*!\brief Scaling Attribute

  Scaling gate=

  Calculates the squared sum of the gate's samples divided by the number of
  samples in the gate.

Input:
0               Data

Outputs:
0               The scaled trace
*/
    

namespace Attrib
{

mClass(Attributes) Scaling: public Provider
{
public:
    static void		initClass();
			Scaling(Desc&);
    static const char*	attribName()		{ return "Scaling"; }
    static const char*	scalingTypeStr()	{ return "scalingtype"; }
    static const char*	powervalStr()		{ return "powerval"; }
    static const char*	gateStr()		{ return "timegate"; }
    static const char*	factorStr()		{ return "scalefactor"; }
    static const char*	widthStr()		{ return "width"; }
    static const char*	mutefractionStr()	{ return "mutefraction"; }
    static const char*	statsTypeStr()		{ return "statstype"; }
    static const char*	sqrangeStr()		{ return "sqrange"; }
    static const char*	squntouchedStr()	{ return "squntouched"; }
    static const char*	statsTypeNamesStr(int type);
    static const char*	scalingTypeNamesStr(int type);

protected:
			~Scaling() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const;

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples,int threadid) const;

    void		getSampleGates(const TypeSet<Interval<float> >& oldtgs,
				       TypeSet< Interval<int> >& newsampgates,
				       int z0,int nrsamples) const;
    void		scaleSqueeze(const DataHolder&, int, int) const;
    void		scaleZN(const DataHolder&, int, int) const;
    void		scaleAGC(const DataHolder&,int z0,int nrsamples) const;
    void		scaleGain(const DataHolder&,int z0,int nrsamples) const;
    void		getScaleFactorsFromStats(
	    			const TypeSet<Interval<int> >& gates,
				TypeSet<float>& factors,int) const;
    void		getTrendsFromStats(
	    			const TypeSet<Interval<int> >& gates,int);
    const Interval<int>* desZSampMargin( int inp, int ) const;

    int			scalingtype_;
    int			statstype_;
    float		powerval_;
    TypeSet< Interval<float> >	gates_;
    TypeSet<float>	factors_;
    const DataHolder*	inputdata_;
    int			dataidx_;
    Interval<int>	desgate_;

    // for Squeeze
    Interval<float>	sqrg_;
    Interval<float>	squrg_;

    // for AGC
    float		width_;
    Interval<float>     window_;
    float          	mutefraction_;

    //for trend removal ( trend of aX+b type )
    struct Trend
    {
			Trend( float a, float b )
			    : a_( a )
			    , b_( b )			{};

	float		valueAtX( float x ) const	{ return a_ * x + b_; }
	bool		operator==( Trend t ) const
							{ return t.a_==a_
							      && t.b_==b_; }

	float		a_;
	float		b_;
    };

    TypeSet<Trend>	trends_;
};

}; // namespace Attrib

#endif

