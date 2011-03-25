#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.12 2011-03-25 14:42:04 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "executor.h"
#include "factory.h"
#include "odmemory.h"
#include "reflectivitymodel.h"
#include "samplingdata.h"

#include "complex"

class Wavelet;
class SeisTrc;

typedef std::complex<float> float_complex;
namespace Fourier { class CC; };

namespace Seis
{

/* brief generates synthetic traces.The SynthGenerator performs the basic 
   convolution with a reflectivity series and a wavelet. If you have AI layers 
   and want directly some synthetics out of them, then you should use the 
   RayTraceSynthGenerator.

   The different constructors and generate() functions will optimize for
   different situations. For example, if your Reflectivity/AI Model is fixed 
   and you need to generate for multiple wavelets, then you benefit from only 
   one anti-alias being done.
*/


mClass SynthGenerator
{
public:
    				SynthGenerator();
    				~SynthGenerator();

    bool			setModel(const ReflectivityModel&);
    bool			setWavelet(const Wavelet*,OD::PtrPolicy);
    bool			setOutSampling(const StepInterval<float>&);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain */

    bool                        doPrepare();
    bool			doWork();
    const char*			errMsg() const		{ return errmsg_.buf();}
    const SeisTrc&		result() const		{ return outtrc_; }

    void 			getSampledReflectivities(TypeSet<float>&) const;

protected:

    bool 			computeTrace(float* result); 
    bool 			doTimeConvolve(float* result); 
    bool 			doFFTConvolve(float* result);

    const Wavelet*		wavelet_;
    StepInterval<float>		outputsampling_;
    ReflectivityModel		refmodel_;

    BufferString		errmsg_;

    Fourier::CC*                fft_;
    int				fftsz_;
    float_complex*		freqwavelet_;
    bool			needprepare_;	
    bool			waveletismine_;
    TypeSet<float_complex>	cresamprefl_;
    SeisTrc&			outtrc_;
};



mClass RaySynthGenerator : public ParallelTask
{
public:
    mDefineFactoryInClass( RaySynthGenerator, factory );

    virtual bool		setModel(const AIModel&);
    virtual bool		setOffsets(const TypeSet<float>&);

    virtual bool		setWavelet(const Wavelet*,OD::PtrPolicy);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain*/
    virtual bool		setOutSampling(const StepInterval<float>&);

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    const char*			errMsg() const;

    const SeisTrc*		result(int off) const;

protected:
    				RaySynthGenerator();
    virtual 			~RaySynthGenerator();

    od_int64            	nrIterations() const;
    virtual bool        	doPrepare(int);
    virtual bool        	doWork(od_int64,od_int64,int);

    BufferString		errmsg_;
    SynthGenerator   		synthgenbase_;	
    int				nrdone_;

    RayTracer1D&		raytracer_;
    AIModel			aimodel_;
    TypeSet<float>		offsets_;

    ObjectSet<SeisTrc>		outtrcs_;
    TypeSet<int>		offsetidxs_;
};



mClass ODRaySynthGenerator : public RaySynthGenerator
{
public:

     bool        		setPar(const IOPar&) { return true; }
     void        		fillPar(IOPar&) const {}

protected:

    static void         	initClass() 
				{factory().addCreator(create,"Fast Generator");}
    static RaySynthGenerator* 	create() { return new ODRaySynthGenerator; }
};

} //namespace


#endif

