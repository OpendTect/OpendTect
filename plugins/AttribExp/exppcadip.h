#ifndef exppcadip_h
#define exppcadip_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: exppcadip.h,v 1.6 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

PCADip [stepout=4,4] [samplegate=[-4,4]] [fraction=25]

PCADip calculates the dip on non-periodic enteties by probing finding
the dip, that has the lowest variance.

Input:
0	Data on which the dip should be calculated.

Outputs:
0	inlinedip
1	crlinedip

@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <arrayndimpl.h>
#include <attribparamimpl.h>

class PCADipAttrib : public AttribCalc
{
public:
    mAttrib3Param( PCADipAttrib
	, "PCADip"
	, BinIDAttribParameter
	    , stepout
	    , BinIDAttribParameter("stepout"
		, AttribParameter::Required
		, BinID(4,4)
		, Interval<int>(0,INT_MAX)
		,Interval<int>(0,INT_MAX)
	    )
	, IntAttribParameter
	    , fraction
	    , IntAttribParameter("fraction"
		, AttribParameter::Required
		, 25
		, Interval<int>(0,INT_MAX)
	    )
	, SampleGateAttribParameter
	    , samplegate
	    , SampleGateAttribParameter("samplegate"
		, AttribParameter::Required
		, Interval<int>(-4,4)
		, Interval<int>(-100,100)
	    )
	, mAttribParamFormHasNoUpdate
    );
	
			PCADipAttrib( Parameters* );
			~PCADipAttrib();

    bool		init();

    int                 nrAttribs() const { return 2; }
    const char*		attribName(int val) const
			{
			    if ( !val ) return "\"In-line dip\"";
			    if ( val == 1 ) return "\"Cross-line dip\"";

			    return 0;
			}

    const BinID*	reqStepout(int inp,int) const
			{ return inp ? 0 : &stepout; }
    const Interval<int>* reqExtraSamples(int inp,int) const
			{ return inp ? 0 : &sg; }

    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }

    Seis::DataType	dataType(int val,const TypeSet<Seis::DataType>&) const
			{ return Seis::Dip; }

    const char* 	definitionStr() const { return desc; }
    void		setCommonInfo( const AttribProcessCommonInfo& ni )
			{ common = &ni; }

protected:
    
    BufferString		desc;
    Interval<int>		sg;
    BinID			stepout;

    float			inldist;
    float			crldist;

    int 			fraction;
    const AttribProcessCommonInfo*	common;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const PCADipAttrib& calculator_ )
				    : calculator ( calculator_ )
				    , trcs( 0 ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new PCADipAttrib::Task::Input(*this); }

	    Array2DImpl<SeisTrc*>*	trcs;
	    int				attribute;
	    const PCADipAttrib&		calculator;
	};

			    Task( const PCADipAttrib& calculator_ )
				: inldips( 0 )
				, crldips( 0 )
				, calculator( calculator_ )
			    { }

			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
	void		    set( float , int , float ,
				 const AttribCalc::Task::Input* ,
                                 const TypeSet<float*>& );

	AttribCalc::Task*    clone()const;

	int		    getFastestSz() const { return 25; }

	int		    nextStep();

	static float	    getEigen(int*,int*,int*,int*,int,int,
				     Array2D<int>&,float&);

	static float	    getMinEigenVector(  const int*, const int*,
					    	const int*, const int*, int,
						int, double&, double&, double&);

	AttribCalc::Task::Input* getInput() const
			    { return new PCADipAttrib::Task::Input( calculator ); }

    protected:
	float*				inldips;
	float*				crldips;

	const PCADipAttrib&		calculator;
    };

    friend class	PCADipAttrib::Task;
    friend class	PCADipAttrib::Task::Input;
};

#endif
