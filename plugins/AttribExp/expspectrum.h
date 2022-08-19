#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

/*

TraceSpectrum window=[Box]|Hamming|Hanning|Barlett|Blackman|CosTaper5
			complex=[Yes]|No

Calculates the frequenc spectrum of a trace
Input:
0	Real data
1	Imag data

Output:
0	Spectrum
1	Real part
2	Imag part

@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <complex>
#include <arrayndimpl.h>
#include <fft.h>
#include <attribparamimpl.h>

#include <arrayndalgo.h>

    
mClass(AttribExp) TraceSpectrumAttrib : public AttribCalc
{
public:
    mAttrib2Param(TraceSpectrumAttrib
	,"TraceSpectrum"
	, EnumAttribParameter
	    , window
	    , EnumAttribParameter("window"
		, ArrayNDWindow::WindowTypeNames
		, AttribParameter::Default
		, 0
	    )
	, BoolAttribParameter
	    , complex
	    , BoolAttribParameter("complex"
		,AttribParameter::Default
		, true
	    )
	, mAttribParamFormHasNoUpdate
    );

			TraceSpectrumAttrib( Parameters* );
			~TraceSpectrumAttrib();

    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }

    bool		init();

    int                 nrAttribs() const { return 3; }


    Seis::DataType	dataType(int,const TypeSet<Seis::DataType>&) const
			{ return Seis::UnknowData; }
    const char* 	definitionStr() const { return desc; }

protected:
    ArrayNDWindow::WindowType	windowtype;

    BufferString	desc;
    float		inpstep;
    float		df;

    mClass(AttribExp) Task : public AttribCalc::Task
    {
    public:
	mClass(AttribExp) Input : public AttribCalc::Task::Input
	{
	public:
				Input( const TraceSpectrumAttrib& calculator_ )
				: calculator ( calculator_ ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			{ return new TraceSpectrumAttrib::Task::Input(*this); }

	    const SeisTrc*		realtrc;
	    const SeisTrc*		imagtrc;

	    int				reattrib;
	    int				imattrib;

	    const TraceSpectrumAttrib& 	calculator;
	};

			    Task( const TraceSpectrumAttrib& calculator_ )
				: power( 0 )
				, realout( 0 )
				, imagout( 0 )
				, timedomain( 0 )
				, freqdomain( 0 )
				, window( 0 )
				, calculator( calculator_ )
			    { }

			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
			    ~Task();
	
	void		    set( float t1_, int nrtimes_, float step_, 
					    const AttribCalc::Task::Input* inp,
                                            const TypeSet<float*>& outp_)
				{ t1 = t1_; nrtimes = nrtimes_; 
				  step = step_; input = inp; power = outp_[0];
				  realout = outp_[1]; imagout = outp_[2]; }

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return INT_MAX; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		    { return new TraceSpectrumAttrib::Task::Input( calculator ); }

    protected:
	float*				power;
	float*				realout;
	float*				imagout;

	const TraceSpectrumAttrib&	calculator;
	Array1DImpl<float_complex>*	timedomain;
	Array1DImpl<float_complex>*	freqdomain;

	ArrayNDWindow*			window;
	FFT				fft;

    };

    friend class	TraceSpectrumAttrib::Task;
    friend class	TraceSpectrumAttrib::Task::Input;
};

