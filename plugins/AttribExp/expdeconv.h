#ifndef expdeconv_h
#define expdeconv_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

DeConvolve wavelet=

Performs a deconvolution in fourier domain with the specified wavelet.

Input:
0	Real data
1	Imag data

Output:
0	Real data
1	Imag data

@$*/

#include "attribcalc.h"
#include "task.h"
#include "position.h"
#include <limits.h>
#include "seistrc.h"
#include <complex>
#include "arrayndimpl.h"
#include "fft.h"
#include "attribparamimpl.h"

#include <arrayndalgo.h>


mClass(AttribExp) DeConvolveAttrib : public AttribCalc
{
public:

    mAttrib5Param(DeConvolveAttrib,"DeConvolve",
	TimeGateAttribParameter, gate,
	   TimeGateAttribParameter(	"samplegate",
					AttribParameter::Required,
					TimeGate(-64,64),
					TimeGate(-mUndefValue, mUndefValue)),
	BinIDAttribParameter, pos1,
	    BinIDAttribParameter(	"pos1",
					AttribParameter::Required,
					BinID(0,1),
					Interval<int>(-100,100),
					Interval<int>(-100,100)),
	BinIDAttribParameter, neighbourhood,
	    BinIDAttribParameter(	"neighbourhood",
					AttribParameter::Required,
					BinID(1,1),
					Interval<int>(1,10),
					Interval<int>(1,10)),
	EnumAttribParameter, window,
            EnumAttribParameter(	"window",
					ArrayNDWindow::WindowTypeNames,
					AttribParameter::Required,
					0),
	BoolAttribParameter, steering,
	    BoolAttribParameter(	"steering",
					AttribParameter::Default,
					true),);


			DeConvolveAttrib(Parameters*);
			~DeConvolveAttrib();

    bool		init();

    const Interval<float>* reqInterval( int, int ) const {return &gate;}
    const BinID*	reqStepout( int i, int ) const
			{ return i ? 0 : &neighbourhood; }

    int			nrAttribs() const { return 2; }

    const char*		attribName(int val) const
			{
			    switch ( val )
			    {
				case 0:     return "1";
				case 1:     return "2";
			    }
			    return 0;
			}

    DataType		dataType( int, const TypeSet<DataType>& ) const
			{ return Seis::UnknownData; }

    const char*		definitionStr() const { return desc; }
    void		setCommonInfo( const AttribProcessCommonInfo& ni )
			{ common = &ni; }


protected:

    Interval<float>	gate;
    bool		steering;
    BinID		neighbourhood;
    BinID		pos1;

    FFT			fft;
    FFT			ifft;
    int			fftsz;
    ArrayNDWindow::WindowType windowtype;
    ArrayNDWindow*	window;
    float		inpstep;
    float		df;

    BufferString	desc;
    const AttribProcessCommonInfo* common;

    mClass(AttribExp) Task : public AttribCalc::Task
    {
    public:
	mClass(AttribExp) Input : public AttribCalc::Task::Input
	{
	public:
			Input( const DeConvolveAttrib& calculator_ )
			    : calculator (calculator_)
			    , trcs(0)
			    , inldiptrc(0)
			    , crldiptrc(0)	    {}
			~Input();

	    bool	set(const BinID&,
			    const ObjectSet<AttribProvider>&,
			    const TypeSet<int>&,
			    const TypeSet<float*>&);

	    Task::Input* clone() const
			{ return new DeConvolveAttrib::Task::Input(*this); }

	    Array2DImpl<SeisTrc*>* trcs;
	    SeisTrc*	inldiptrc;
	    SeisTrc*	crldiptrc;

	    int		dataattrib;
	    int		inldipattrib;
	    int		crldipattrib;

	    const DeConvolveAttrib& calculator;
	};

			Task(const DeConvolveAttrib&);
			Task(const Task&)	    = delete;
			~Task();

	void		set( float t1_, int nrtimes_, float step_,
				const Input* inp, const TypeSet<float*>& outp_)
			{
			    t1 = t1_;
			    nrtimes = nrtimes_;
			    step = step_;
			    input = inp;
			    out0 = outp_[0];
			    out1 = outp_[1];
			}

	Task*		clone() const;
	int		getFastestSz() const { return 25; }
	int		nextStep();
	Input*		getInput() const
			{ return new Input( calculator ); }

    protected:

	float*		out0;
	float*		out1;

	typedef Array1D<float_complex> CplxArr;

	const DeConvolveAttrib&	calculator;
	Array2D<CplxArr*>* tracesegments;
	CplxArr*	spectrum0;
	CplxArr*	spectrum1;
	CplxArr*	spectrumaverage;
	CplxArr*	spectrumoutput;
	CplxArr*	traceoutput;
    };

    friend class	Task;
    friend class	Task::Input;
};

#endif
