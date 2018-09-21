#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "seismod.h"
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

namespace SynthSeis
{

class RayModel;

/*!\brief base class for synthetic trace generators. */

mExpClass(Seis) GenBase
{ mODTextTranslationClass(SynthSeis::GenBase);
public:

    virtual void	setWavelet(const Wavelet*);
			/* auto computed + will be overruled if too small */
    virtual bool	setOutSampling(const StepInterval<float>&);
			/* depends on the wavelet size too */
    bool		getOutSamplingFromModels(
					const ObjectSet<ReflectivityModel>&,
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
			GenBase();
    virtual		~GenBase();

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
   The MultiTraceGenerator is a Parallel runner of the Generator.

   If you have AI layers and directly want some synthetics out of them,
   then you should use the RayTraceGenerator.
*/


mExpClass(Seis) Generator : public GenBase
{ mODTextTranslationClass(SynthSeis::Generator);
public:

    mDefineFactoryInClass( Generator, factory );

    static Generator*	create(bool advanced);

			Generator();
			~Generator();

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

    const ReflectivityModel*	reflmodel_;
    int				convolvesize_;
    SeisTrc&			outtrc_;

    ReflectivityModel		sampledreflmodel_;
    TypeSet<float_complex>	freqreflectivities_;
    TypeSet<float_complex>	freqwavelet_;

    od_int64                    progress_;

};


mExpClass(Seis) MultiTraceGenerator : public ParallelTask,
				      public GenBase
{ mODTextTranslationClass(SynthSeis::MultiTraceGenerator);
public:

    typedef RefMan<ReflectivityModelSet>	RflMdlSetRef;
    typedef ConstRefMan<ReflectivityModelSet>	ConstRflMdlSetRef;

			MultiTraceGenerator();
			~MultiTraceGenerator();

    void		setModels(const ReflectivityModelSet&);

    void		getResult(ObjectSet<SeisTrc>&) const;
			//!< once only. traces become yours
    RflMdlSetRef	getSampledRMs() const	{ return sampledreflmodels_; }
			//!< ref counted set of man objs, can be done many times

    uiString		message() const { return tr("Generating synthetics"); }
    od_int64		totalNr() const	{ return totalnr_; }

protected:

    virtual od_int64	nrIterations() const;
    virtual bool	doPrepare(int);
    virtual bool	doWork(od_int64,od_int64,int);

    void		cleanUp();

			// input
    ConstRflMdlSetRef	models_;
			// output
    ObjectSet<SeisTrc>	trcs_;
    RflMdlSetRef	sampledreflmodels_;

    ObjectSet<Generator> generators_;
    od_int64		totalnr_;

};

} // namespace SynthSeis
