#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

/*

NearSubtract usedip=Yes|No relampl=[Yes|No]

Input:
0	Data on wich the attrib should be calculated
1	Inline Dip (if required)
2	Crossline Dip (if required)

Output:
0	Output data

@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <attribparamimpl.h>


mClass(AttribExp) NearSubtractAttrib : public AttribCalc
{
public:
    mAttrib2Param( NearSubtractAttrib
	, "NearSubtract"
	, BoolAttribParameter
	    , usedip
	    , BoolAttribParameter( "usedip"
		, AttribParameter::Default
		, true
	     )
	, BoolAttribParameter
	    , relampl
	    , BoolAttribParameter( "relampl"
		, AttribParameter::Default
		, false
	    )
	, mAttribParamFormHasNoUpdate
    );

				NearSubtractAttrib( Parameters* );
				~NearSubtractAttrib();

    int				nrAttribs() const { return 1; }
    const BinID*		reqStepout(int inp, int ) const
				{
				    if ( !inp ) return &stepout;
				    return 0;
				}

    Seis::DataType		dataType(int,
					 const TypeSet<Seis::DataType>&) const
				{ return Seis::UnknowData; }

    const char*			definitionStr() const { return desc; }

    bool			init();
    void			setCommonInfo(const AttribProcessCommonInfo& ni)
				{ common = &ni; }

protected:
    bool			usedip;
    bool			relampl;

    float			inldist;
    float			crldist;

    BufferString		desc;

    static const BinID		stepout;
    const AttribProcessCommonInfo*	common;

    mClass(AttribExp) Task : public AttribCalc::Task
    {
    public:
	mClass(AttribExp) Input : public AttribCalc::Task::Input
	{
	public:
				Input( const NearSubtractAttrib& calculator_ )
				    : calculator ( calculator_ )
				    , trc0( 0 )
				    , trc1( 0 )
				    , inldiptrc( 0 )
				    , crldiptrc( 0 ) {}

	    bool		set( const BinID&,
				    const ObjectSet<AttribProvider>&,
				    const TypeSet<int>&,
				    const TypeSet<float*>&  );

	    AttribCalc::Task::Input* clone() const
			{ return new NearSubtractAttrib::Task::Input(*this); }

	    const SeisTrc*		trc0;
	    const SeisTrc*		trc1;
	    const SeisTrc*		inldiptrc;
	    const SeisTrc*		crldiptrc;

	    int				trc0attrib;
	    int				trc1attrib;
	    int				inldiptrcattrib;
	    int				crldiptrcattrib;

	    const NearSubtractAttrib&	calculator;
	};

			    Task( const NearSubtractAttrib& calculator_ )
				: outp( 0 )
				, calculator( calculator_ ) {}

			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it

	void		    set( float t1_, int nrtimes_, float step_,
					    const AttribCalc::Task::Input* inp,
                                            const TypeSet<float*>& outp_)
				{ t1 = t1_; nrtimes = nrtimes_;
				  step = step_; input = inp; outp = outp_[0]; }

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return 25; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		    { return new NearSubtractAttrib::Task::Input( calculator );}

    protected:
	float*				outp;
	const NearSubtractAttrib&	calculator;

    };

    friend class	NearSubtractAttrib::Task;
    friend class	NearSubtractAttrib::Task::Input;
};

