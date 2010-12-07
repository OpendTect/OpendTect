#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.7 2010-12-07 16:15:43 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "samplingdata.h"
class AIModel;
class Wavelet;
class SeisTrc;


namespace Seis
{

/* Generates synthetic traces.
 
   Note that the Wavelet and the AIModel will copied, but ... they will need to
   stay alive during each of the actions.

   The different constructors and generate() functions will optimize for
   different situations. For example, if your AIModel is fixed and you need
   to generate for multiple wavelets, then you benefit from only one anti-alias
   being done.

   If you don't call setOutSampling yourself, then getDefOutSampling() will be
   used.
 
 */

mClass SynthGenerator
{
public:

			SynthGenerator();
			SynthGenerator(const AIModel&);
			SynthGenerator(const Wavelet&);
			SynthGenerator(const AIModel&,const Wavelet&);
    virtual		~SynthGenerator();

    static SamplingData<float> getDefOutSampling(const AIModel&,
						 const Wavelet&,int& nrsamples);
    void		setOutSampling(const SamplingData<float>&,int ns);

    void		generate();
    void		generate(const AIModel&);
    void		generate(const Wavelet&);
    void		generate(const AIModel&,const Wavelet&);

    const SeisTrc&	result() const		{ return outtrc_; }
    			//!< will have no positioning at all

protected:

    const AIModel*	inpaimdl_;
    const Wavelet*	inpwvlt_;

    AIModel*		aimdl_;
    Wavelet*		wvlt_;
    SeisTrc&		outtrc_;

    void		init(const AIModel*,const Wavelet*);
    void		prepAIModel();
    void		prepWavelet();

};

}

#endif
