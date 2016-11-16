#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "ailayer.h"
#include "factory.h"
#include "iopar.h"
#include "odcomplex.h"
#include "odmemory.h"
#include "paralleltask.h"
#include "reflectivitymodel.h"
#include "threadlock.h"
#include "wavelet.h"
#include "uistrings.h"

class RayTracer1D;
class SeisTrc;
template <class T> class Array1D;
namespace Fourier { class CC; };

namespace Seis
{

/*!\brief base class for synthetic trace generators. */


mExpClass(Seis) SynthGenBase
{ mODTextTranslationClass(SynthGenBase);
public:

    virtual void	setWavelet(const Wavelet*);
			/* auto computed + will be overruled if too small */
    virtual bool	setOutSampling(const StepInterval<float>&);
			/* depends on the wavelet size too */
    bool		getOutSamplingFromModel(
					const RefMan<ReflectivityModelSet>&,
					StepInterval<float>&,bool usenmo=false);

    void		setMuteLength(float n)	{ mutelength_ = n; }
    float		getMuteLength() const	{ return mutelength_; }

    void		setStretchLimit(float n){ stretchlimit_ = n; }
    float		getStretchLimit() const;
    void		doSampledReflectivity(bool yn)
			{ dosampledreflectivities_ = yn; }

    virtual void	enableFourierDomain(bool fourier)
			{ isfourier_ = fourier; }

    uiString		errMsg() const		{ return errmsg_;}

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static float	cStdMuteLength() { return 0.02f; }
    static float	cStdStretchLimit() { return 0.2f; }

    static const char*	sKeyFourier()	{ return "Convolution Domain"; }
    static const char*	sKeyNMO()	{ return "Use NMO"; }
    static const char*  sKeyInternal()  { return "Internal Multiples"; }
    static const char*  sKeySurfRefl()	{ return "Surface Reflection coef"; }
    static const char*	sKeyMuteLength(){ return "Mute length"; }
    static const char*	sKeyStretchLimit(){ return "Stretch limit"; }

protected:
			SynthGenBase();
    virtual		~SynthGenBase();

    bool		isfourier_;
    bool		applynmo_;
    float		stretchlimit_;
    float		mutelength_;
    ConstRefMan<Wavelet> wavelet_;
    StepInterval<float>	outputsampling_;
    bool		dointernalmultiples_;
    bool		dosampledreflectivities_;
    float		surfreflcoeff_;

    uiString		errmsg_;

    bool		isInputOK();
};


/*!\brief generates synthetic traces. It performs the basic
   convolution with a reflectivity series and a wavelet.
   The MultiTraceSynthGenerator is a Parallel runner of the SynthGenerator.

   If you have AI layers and directly want some synthetics out of them,
   then you should use the RayTraceSynthGenerator.
*/


mExpClass(Seis) SynthGenerator : public SynthGenBase
{ mODTextTranslationClass(SynthGenerator);
public:
    mDefineFactoryInClass( SynthGenerator, factory );

    static SynthGenerator* create(bool advanced);

			SynthGenerator();
			~SynthGenerator();

    virtual void	setWavelet(const Wavelet*);
    virtual bool	setOutSampling(const StepInterval<float>&);
    bool		setModel(const ReflectivityModel&);

    bool		doWork();
    od_int64            currentProgress() const { return progress_; }

    const SeisTrc&	result() const		{ return outtrc_; }
    SeisTrc&		result()		{ return outtrc_; }

			/*<! available after execution */
    const TypeSet<float_complex>& freqReflectivities() const
			{ return freqreflectivities_; }
    void		getSampledRM(ReflectivityModel&) const;


protected:

    int			nextStep();
    int			setConvolveSize();
    int			genFreqWavelet();

    bool		computeTrace(SeisTrc&);
    bool		doNMOStretch(const ValueSeries<float>&, int insz,
				     ValueSeries<float>& out,int outsz) const;
    bool		doFFTConvolve(ValueSeries<float>&,int sz);
    bool		doTimeConvolve(ValueSeries<float>&,int sz);
    void		getWaveletTrace(Array1D<float>&,float z,float scal,
					SamplingData<float>&) const;
    void		sortOutput(float_complex*,ValueSeries<float>&,
				   int sz) const;

    virtual bool	computeReflectivities();

    const ReflectivityModel*	refmodel_;
    int				convolvesize_;
    SeisTrc&			outtrc_;

    ReflectivityModel		sampledrefmodel_;
    TypeSet<float_complex>	freqreflectivities_;
    TypeSet<float_complex>	freqwavelet_;

    od_int64                    progress_;

};


mExpClass(Seis) MultiTraceSynthGenerator : public ParallelTask,
					   public SynthGenBase
{ mODTextTranslationClass(MultiTraceSynthGenerator);
public:
				MultiTraceSynthGenerator();
				~MultiTraceSynthGenerator();

    void			setModels(RefMan<ReflectivityModelSet>&);

    void			getResult(ObjectSet<SeisTrc>&);
    void			getSampledRMs(RefMan<ReflectivityModelSet>&);

    uiString			message() const {
				    return m3Dots(tr("Generating synthetics"));
						  }

    od_int64                    totalNr() const	{ return totalnr_; }

protected:

    od_int64	nrIterations() const;
    bool                        doPrepare(int);
    virtual bool	doWork(od_int64,od_int64,int);

    RefMan<ReflectivityModelSet>	models_;
    RefMan<ReflectivityModelSet>	sampledrefmodels_;
    ObjectSet<SynthGenerator>	synthgens_;
    ObjectSet<SeisTrc>		trcs_;
    TypeSet<int>		trcidxs_;
    od_int64			totalnr_;
    Threads::Lock		lock_;
};

} // namespace Seis
