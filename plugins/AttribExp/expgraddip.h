#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

GradientDip size= 


Outputs:
0	Inl dip
1	crl dip

@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <arrayndimpl.h>
#include <attribparamimpl.h>


mClass(AttribExp) GradientDipAttrib : public AttribCalc
{
public:
    mAttrib1Param( GradientDipAttrib
	, "GradientDip"
	, IntAttribParameter
	    , size
	    , IntAttribParameter( "size"
		, AttribParameter::Required
		, 7
		, Interval<int>(3,100) 
	    )
	, mAttribParamFormHasNoUpdate
    );

			GradientDipAttrib( Parameters* );
			~GradientDipAttrib();

    bool		init();

    int                 nrAttribs() const { return 2; }
    const char*		attribName(int val) const
			{
			    if ( !val ) return "In-line dip";
			    if ( val == 1 ) return "Cross-line dip";

			    return 0;
			}

    const BinID*	reqStepout( int inp, int ) const { return &stepout; }
    const Interval<int>* reqExtraSamples(int, int) const { return &sg;}

    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }


    Seis::DataType	dataType(int,const TypeSet<Seis::DataType>&) const
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

    int 			sz;
    const AttribProcessCommonInfo*	common;

    mClass(AttribExp) Task : public AttribCalc::Task
    {
    public:
	mClass(AttribExp) Input : public AttribCalc::Task::Input
	{
	public:
				Input( const GradientDipAttrib& calculator_ )
				: calculator ( calculator_ )
				, trcs( 0 ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new GradientDipAttrib::Task::Input(*this); }

	    Array2DImpl<SeisTrc*>*	trcs;
	    int				attribute;

	    const GradientDipAttrib&	calculator;
	};

			    Task( const GradientDipAttrib& calculator_ )
				: inldips( 0 )
				, crldips( 0 )
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
			{ return new GradientDipAttrib::Task::Input( calculator ); }

    protected:
	float*		inldips;
	float*		crldips;

	const GradientDipAttrib& calculator;
    };

    friend class	GradientDipAttrib::Task;
    friend class	GradientDipAttrib::Task::Input;
};

