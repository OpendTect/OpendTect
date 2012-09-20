#ifndef expdiscfilter_h
#define expdiscfilter_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

DiscFilter radius= planefilter=true [constantvel=true] [velocity=] [fast=]

Discfilter places a disc with the "radius" along the dip specified on the
input. The filter extracts samples along the disc and derives statistical
properties from it. If planefilter is enabled, the filter will go the all
traces within the radius. If planefilter is turned off, it will cut out a
disc through the data regardless of which trace it might be. At high
dips, it might equally well take many samples from the same trace.
This option makes it neccecary to convert the time-domain to distance, and
this is what the velocity is for. Velocity can either be stated on commandline
or be given for each sample. The 'fast' option is use when planefilter is
turned off and specifies wether the filter should snap each sample to the
closest trace, or do an interpolation between the traces.  

Input:
0	Data
1	Inline dip
2	Crossline dip
3	Velocity (only if not stated on command-line)

Outputs:
0	Mean
1	Median
2	Stddev
3	Variance
4       Min
5       Max
6       Most Frequent


@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <arrayndimpl.h>
#include <runstat.h>
#include <attribparamimpl.h>

#define mDiscFilterAvg		0
#define mDiscFilterMed		1
#define mDiscFilterStdDev	2
#define mDiscFilterVar		3
#define mDiscFilterMin		4
#define mDiscFilterMax		5
#define mDiscFilterMostFreq	6

#define mDiscFilterNrVals	7


class DiscFilterAttrib : public AttribCalc
{
public:
    mAttrib5Param( DiscFilterAttrib
	, "DiscFilter"
	, IntAttribParameter
	    , radius
	    , IntAttribParameter( "radius"
		, AttribParameter::Required
		, 3
		, Interval<int>(0,100)
	    )
	, BoolAttribParameter
	    , planefilter
	    , BoolAttribParameter( "planefilter"
		, AttribParameter::Default
		, false
	    )
	, BoolAttribParameter
	    , constantvel
	    , BoolAttribParameter( "constantvel"
		, AttribParameter::Default
		, true
	    )
	, FloatAttribParameter
	    , velocity
	    , FloatAttribParameter( "velocity"
		, AttribParameter::Required
		, 4000
		, Interval<float>(0,mUndefValue)
	    )
	, BoolAttribParameter
	    , fast
	    , BoolAttribParameter( "fast"
		, AttribParameter::Default
		, true
	    )
	, mAttribParamFormHasUpdate);

			DiscFilterAttrib( Parameters* );
			~DiscFilterAttrib();

    bool		init();

    int                 nrAttribs() const { return mDiscFilterNrVals ; }
    const char*		attribName(int val) const
			{
			    switch (val)
                            {
				case mDiscFilterAvg:     return "Average";
				case mDiscFilterMed:     return "Median";
				case mDiscFilterStdDev:  return "StdDev";
				case mDiscFilterVar:     return "Variance";
				case mDiscFilterMin:     return "Min";
				case mDiscFilterMax:     return "Max";
				case mDiscFilterMostFreq:return "MostFreq";
			    }

			    return 0;
			}

    Seis::DataType	dataType(int,const TypeSet<Seis::DataType>&) const;

    const BinID*	reqStepout(int inp, int) const
			{ return inp ? 0 : &stepout; }
    const Interval<int>* reqExtraSamples(int inp, int) const
			{ return inp ? 0 : &sg; }

    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }

    Seis::DataType	dataType(int val, int) const;

    const char* 	definitionStr() const { return desc; }
    void		setCommonInfo( const AttribProcessCommonInfo& ni )
			{ common = &ni; }

protected:
    
    BufferString		desc;
    Interval<int>		sg;
    BinID			stepout;

    float			inldist;
    float			crldist;

    int 			radius;
    float			velocity;
    bool			constantvel;
    bool 			fast;
    const AttribProcessCommonInfo*	common;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const DiscFilterAttrib& calculator_ )
				    : calculator ( calculator_ )
				    , inldiptrc( 0 )
				    , crldiptrc( 0 )
				    , veltrc( 0 )
				    , trcs( 0 ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new DiscFilterAttrib::Task::Input(*this); }

	    Array2DImpl<const SeisTrc*>*	trcs;
	    const SeisTrc*			veltrc;
	    const SeisTrc*			inldiptrc;
	    const SeisTrc*			crldiptrc;

	    int					dataattrib;
	    int					velattrib;
	    int					inldipattrib;
	    int					crldipattrib;

	    const DiscFilterAttrib&		calculator;
	};

			    Task( const DiscFilterAttrib& calculator_ )
				: avg( 0 )
				, med( 0 )
				, stddev( 0 )
				, variance( 0 )
				, min( 0 )
				, max( 0 )
				, mostfreq( 0 )
				, indata( 0,0,0 )
				, calculator( calculator_ ) {}

			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
	void		    set( float , int , float ,
				 const AttribCalc::Task::Input* ,
                                 const TypeSet<float*>& );

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return 25; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		{ return new DiscFilterAttrib::Task::Input( calculator ); }

    protected:
	float*          avg;
	float*          med;
	float*          stddev;
	float*          variance;
	float*          min;
	float*          max;
	float*          mostfreq;

	Array3DImpl<float>		indata;
	RunningStatistics<float>	stat;

	const DiscFilterAttrib&	calculator;
    };

    friend class	DiscFilterAttrib::Task;
    friend class	DiscFilterAttrib::Task::Input;
};

#endif
