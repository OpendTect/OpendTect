#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.10 2011-03-11 13:42:09 cvsbruno Exp $
________________________________________________________________________

-*/

#include "complex"
#include "ailayer.h"
#include "executor.h"
#include "factory.h"
#include "odmemory.h"
#include "reflectivitymodel.h"
#include "samplingdata.h"

class AILayer;
class RayTracer1D;
class Wavelet;
class SeisTrc;

typedef std::complex<float> float_complex;
template <class T> class Array1DImpl;
namespace Fourier { class CC; };

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

mClass SynthGeneratorBase : public Executor
{
public:
    mDefineFactoryInClass( SynthGeneratorBase, factory );

    virtual bool		setModel(const ReflectivityModel&);
    virtual bool		setWavelet(Wavelet*,OD::PtrPolicy);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain */
    virtual bool		setOutSampling(const StepInterval<float>&);

    const SeisTrc&		result() const		{ return *outtrc_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void 			getSampledReflectivities(TypeSet<float>&) const;

protected:
    				SynthGeneratorBase();
    virtual			~SynthGeneratorBase();

    bool 			computeTrace(float* result); 
    bool 			genericConvolve(float* result); 
    bool 			FFTConvolve(float* result); 

    int				nextStep();

    Wavelet*			wavelet_;

    ReflectivityModel		refmodel_;
    BufferString		errmsg_;
    TypeSet<float_complex>	cresamprefl_;

    Fourier::CC*                fft_;
    int				fftsz_;
    Array1DImpl<float_complex>*	freqwavelet_;

    bool			waveletismine_;
    StepInterval<float>		outputsampling_;

    SeisTrc*			outtrc_;
};



mClass ODSynthGenerator : public SynthGeneratorBase
{
public:

     bool        		setPar(const IOPar&) { return true; }
     void        		fillPar(IOPar&) const {}

protected:

    static void         	initClass() 
				{factory().addCreator(create,"Fast Generator");}
    static SynthGeneratorBase* 	create() 	
    				{ return new ODSynthGenerator; }
};

}

#endif
