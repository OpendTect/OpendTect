#ifndef expwavelet1d_h
#define expwavelet1d_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: expwavelet1d.h,v 1.5 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

Wavelet1D minwaveletlen= maxwaveletlen= wavelet=

Wavelet1D calculates attributes from the scale-spectrum created by the 1D
wavelet transform of the trace.

minwaveletlen and maxwaveletlen can be:
2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 or 4196

wavelet can be:
"Haar", "Daubechies 4", "Daubechies 6", "Daubechies 8", "Daubechies 10",
"Daubechies 12", "Daubechies 14", "Daubechies 16", "Daubechies 18",
"Daubechies 20", "Beylkin", "Coiflet 1", "Coiflet 2", "Coiflet 3",
"Coiflet 4", "Coiflet 5", "Symmlet 4", "Symmlet 5", "Symmlet 6",
"Symmlet 7", "Symmlet 8", "Symmlet 9", "Symmlet 10" or "Vaidyanathan"

Output

0 - Average scale
1 - Average scale squared
2 - Max scale
3 - Average scale amplitude
4 - Max scale amplitude
5 - Average scale amplitude squared
6 - Spectral Area Below Max Scale
7 - Spectral Area Above Max Scale
8 - Scale Std Dev


@$*/

#include "attribcalc.h"
#include "enums.h"
#include "attribparamimpl.h"
#include "wavelettrans.h"

class SeisTrc;
    
class Wavelet1DAttrib : public AttribCalc
{
public:
    mAttrib3Param( Wavelet1DAttrib
	, "Wavelet1D"
	, EnumAttribParameter
	    , minwaveletlen
	    , EnumAttribParameter("minwaveletlen"
		, Wavelet1DAttrib::WaveletLenNames
		, AttribParameter::Default
		, 0
	    )
	, EnumAttribParameter
	    , maxwaveletlen
	    , EnumAttribParameter("maxwaveletlen"
		, Wavelet1DAttrib::WaveletLenNames
		, AttribParameter::Default
		, 4
	    )
	, EnumAttribParameter
	    , wavelet
	    , EnumAttribParameter("wavelet"
		, WaveletTransform::WaveletTypeNames
		, AttribParameter::Default
		, 0
	    )
	, mAttribParamFormHasNoUpdate
    );

    enum WaveletLen	{ s2, s4, s8, s16, s32, s64, s128,
			  s256, s512, s1024, s2048, s4196 };
    			DeclareEnumUtils(WaveletLen);

			Wavelet1DAttrib( Parameters* );

			~Wavelet1DAttrib();

    const Interval<int>* desExtraSamples( int, int ) const { return &dsg; }
    const Interval<float>* inlDipMargin(int,int) const { return 0; }
    const Interval<float>* crlDipMargin(int,int) const { return 0; }

    int                 nrAttribs() const { return 9; }


    Seis::DataType	dataType(int,const TypeSet<Seis::DataType>&) const
			{ return Seis::UnknowData; }

    const char* 	definitionStr() const { return desc; }

protected:

    WaveletLen		minwaveletlen;
    WaveletLen		maxwaveletlen;    
    int			scalelen;

    WaveletTransform::WaveletType	wavelet;
    
    Interval<int>	dsg;
    BufferString	desc;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const Wavelet1DAttrib& calculator_ )
				: calculator ( calculator_ ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>& );

	    AttribCalc::Task::Input* clone() const
			    { return new Wavelet1DAttrib::Task::Input(*this); }

	    const SeisTrc*	trc;
	    int			attrib;

	    const Wavelet1DAttrib& calculator;
	};

			    Task( const Wavelet1DAttrib& calculator_ )
				: outp( 9, 0 )
				, calculator( calculator_ ) {}

			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
	void		    set( float t1_, int nrtimes_, float step_, 
					    const AttribCalc::Task::Input* inp,
                                            const TypeSet<float*>& outp_)
				{ t1 = t1_; nrtimes = nrtimes_; 
				  step = step_; input = inp; outp = outp_; }

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return 25; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		    { return new Wavelet1DAttrib::Task::Input( calculator ); }

    protected:

	TypeSet<float*>		    	outp;
	const Wavelet1DAttrib&		calculator;

    };

    friend class	Wavelet1DAttrib::Task;
    friend class	Wavelet1DAttrib::Task::Input;
};

#endif
