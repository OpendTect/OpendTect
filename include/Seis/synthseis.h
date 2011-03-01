#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.8 2011-03-01 08:35:36 cvsbruno Exp $
________________________________________________________________________

-*/

#include "complex"
#include "executor.h"
#include "factory.h"
#include "odmemory.h"
#include "reflectivitymodel.h"
#include "samplingdata.h"

class AILayer;
class AIModel;
class RayTracer1D;
class Wavelet;
class SeisTrc;

typedef std::complex<float> float_complex;

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
    virtual bool		setOutSampling(const StepInterval<float>&);
    void			setOffsets(const TypeSet<float>& offsets);

    const SeisTrc&		result() const		{ return *outtrc_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void 			getReflectivities(ReflectivityModel& m) const
     				{ m.copy( refmodel_ ); }

protected:
    				SynthGeneratorBase();
    virtual			~SynthGeneratorBase();

    bool 			computeTrace(float* result); 

    int				nextStep();

    Wavelet*			wavelet_;
    ReflectivityModel		refmodel_;
    BufferString		errmsg_;

    int				fftsz_;
    float_complex*		freqwavelet_;

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
