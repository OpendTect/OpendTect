#ifndef expmodgraddip_h
#define expmodgraddip_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: expextremedip.h,v 1.4 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

ModGradientDip


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


class ExtremeDipAttrib : public AttribCalc
{
public:
    mAttrib1Param( ExtremeDipAttrib
	, "ExtremeDip"
	, FloatAttribParameter
	    , maxdip
	    , FloatAttribParameter("maxdip"
		, AttribParameter::Default
		, mUndefValue
		, Interval<float>(0,mUndefValue)
	    )
	, mAttribParamFormHasNoUpdate );

			ExtremeDipAttrib( Parameters* );
			~ExtremeDipAttrib();

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
    const Interval<int>		sg;
    const BinID			stepout;
    const float			maxdip;

    float			inldist;
    float			crldist;

    const AttribProcessCommonInfo*	common;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const ExtremeDipAttrib& calculator_ )
				: calculator ( calculator_ )
				, trcs( 0 ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new ExtremeDipAttrib::Task::Input(*this); }

	    Array2DImpl<SeisTrc*>*	trcs;
	    int				attribute;

	    const ExtremeDipAttrib&	calculator;
	};

			    Task( const ExtremeDipAttrib& calculator_ )
				: inldips( 0 )
				, crldips( 0 )
				, calculator( calculator_ ) {}
	
	void		    set( float , int , float ,
				 const AttribCalc::Task::Input* ,
                                 const TypeSet<float*>& );

	AttribCalc::Task*    clone() const;

        float		    getMaxPos( float val1, float val2,
				       float val3 ) const;
	float		    getRealMaxPos(SeisTrc*,float,float,bool);
	float		findDip(TypeSet<float>&,float,float);
	float		findX(TypeSet<float>&);

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		{ return new ExtremeDipAttrib::Task::Input(calculator); }

    protected:
	float*		inldips;
	float*		crldips;

	const ExtremeDipAttrib& calculator;
    };

    friend class	ExtremeDipAttrib::Task;
    friend class	ExtremeDipAttrib::Task::Input;
};

#endif
