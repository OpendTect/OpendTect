#ifndef expvardip_h
#define expvardip_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: expvardip.h,v 1.6 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

MinVarianceDip size= [resolution=32] [velocity=] [fast=]

MinVarianceDip calculates the dip on non-periodic enteties by probing finding
the dip, that has the lowest variance.

Input:
0	Data on which the dip should be calculated.
1	Velocity (only if not stated on command-line)

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

class MinVarianceDipAttrib : public AttribCalc
{
public:
    mAttrib5Param(MinVarianceDipAttrib
	, "MinVarianceDip"
	, IntAttribParameter
	    , size
	    , IntAttribParameter( "size"
		, AttribParameter::Required
		, 7
		, Interval<int>(0,100)
	    )
	, IntAttribParameter
	    , resolution
	    , IntAttribParameter( "resolution"
		, AttribParameter::Required
		, 32
		, Interval<int>(0,INT_MAX)
	    )
	, BoolAttribParameter
	    , constantvel
	    , BoolAttribParameter("constantvel"
		, AttribParameter::Default
		, true
	    )
	, FloatAttribParameter
	    , velocity
	    , FloatAttribParameter("velocity"
		, AttribParameter::Required
		, 4000
		, Interval<float>(0,mUndefValue)
	    )
	, BoolAttribParameter
	    , fast
	    , BoolAttribParameter("fast"
		, AttribParameter::Default
		, true
	    )
	, mAttribParamFormHasUpdate);
	

			MinVarianceDipAttrib( Parameters* );
			~MinVarianceDipAttrib();

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
			{ return val < 2 ? Seis::Dip : Seis::UnknowData; }

    const char* 	definitionStr() const { return desc; }
    void		setCommonInfo( const AttribProcessCommonInfo& ni )
			{ common = &ni; }

protected:
    
    BufferString		desc;
    Interval<int>		sg;
    BinID			stepout;

    float			inldist;
    float			crldist;

    int 			sz;
    int 			resolution;
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
				Input( const MinVarianceDipAttrib& calculator_ )
				    : calculator ( calculator_ )
				    , veltrc( 0 )
				    , trcs( 0 ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new MinVarianceDipAttrib::Task::Input(*this); }

	    Array2DImpl<SeisTrc*>*	trcs;
	    int				dataattrib;
	    SeisTrc*			veltrc;
	    int				velattrib;
	    const MinVarianceDipAttrib&	calculator;
	};

			    Task( const MinVarianceDipAttrib& calculator_ )
				: inldips( 0 )
				, crldips( 0 )
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
		{ return new MinVarianceDipAttrib::Task::Input( calculator ); }

    protected:
	float*				inldips;
	float*				crldips;
	Array3DImpl<float>		indata;

	const MinVarianceDipAttrib&	calculator;
    };

    friend class	MinVarianceDipAttrib::Task;
    friend class	MinVarianceDipAttrib::Task::Input;
};

#endif
