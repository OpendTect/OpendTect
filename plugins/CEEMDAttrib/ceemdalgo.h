#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ceemdattribmod.h"

#include "mathfunc.h"

template <class T> class Array2D;
namespace Stats { class RandomGenerator; }

#define mDecompModeEMD		0
#define mDecompModeEEMD		1
#define mDecompModeCEEMD	2

#define mDecompOutputFreq	0
#define mDecompOutputPeakFreq	1
#define mDecompOutputPeakAmp	2
#define mDecompOutputIMF	3


mClass(CEEMDAttrib) IMFComponent
{
public:
			IMFComponent(int nrsamples);
			IMFComponent(const IMFComponent&);
			~IMFComponent();

    IMFComponent&	operator= (const IMFComponent&);

    BufferString	name_;
    int			nrzeros_;
    int			size_;
    float*		values_;

};


mClass(CEEMDAttrib) OrgTraceMinusAverage
{
public:
		    OrgTraceMinusAverage(int nrsamples);
		    OrgTraceMinusAverage(const OrgTraceMinusAverage&);
		    ~OrgTraceMinusAverage();

    OrgTraceMinusAverage& operator =( const OrgTraceMinusAverage&);

    BufferString    name_;
    float	    averageinput_;
    float	    stdev_;
    int		    size_;
    float*	    values_;
};


mExpClass(CEEMDAttrib) Setup
{
public:
			Setup()
			  : method_(2)
			  , attriboutput_(4)
			  , maxnoiseloop_(50)
			  , maxnrimf_(16)
			  , maxsift_(10)
			  , stopsift_(0.2f)
			  , stopimf_(0.005f)
			  , noisepercentage_(10.f)
			  , symmetricboundary_(true)
			  , usetestdata_(false)
			  , outputfreq_(5)
			  , stepoutfreq_(5)
//			  , outputcomp_()
			{}

	mDefSetupMemb(int,method); // 0=EMD, 1=EEMD, 2=CEEMD
	// 0=Freq, 1=Peak Freq, 2= Peak Amp, 3=IMF
	mDefSetupMemb(int,attriboutput);
	// Number of realizations for EEMD and CEEMD
	mDefSetupMemb(int,maxnoiseloop);
	// Maximum number of intrinsic Mode Functions
	mDefSetupMemb(int,maxnrimf);
	// Maximum number of sifting iterations
	mDefSetupMemb(int,maxsift);
	// stop sifting if st.dev res.-imf < value
	mDefSetupMemb(float,stopsift);
	// stop decomp. when st.dev imf < value
	mDefSetupMemb(float,stopimf);
	// noise percentage for EEMD and CEEMD
	mDefSetupMemb(float,noisepercentage);
	// boundary extension symmetric or periodic
	mDefSetupMemb(bool,symmetricboundary);
	// use synthetic trace in ceemdtestprogram.h
	mDefSetupMemb(bool,usetestdata);
	// output frequency.
	mDefSetupMemb(bool,outputfreq);
	// step output frequency.
	mDefSetupMemb(bool,stepoutfreq);
	// output IMF component.
	mDefSetupMemb(bool,outputcomp);
};


mClass(CEEMDAttrib) DecompInput
{
public:
			DecompInput(const Setup&,int nrsamples,
				    Stats::RandomGenerator* =nullptr);
			DecompInput(const DecompInput&);
			~DecompInput();

    DecompInput&	operator =(const DecompInput&);

    bool		doDecompMethod(int nrsamples,float refstep,
			       Array2D<float>& output,int outputattrib,
			       float startfreq,float endfreq,float stepoutfreq,
			       int startcomp,int endcomp);

    Setup		setup_;
    int			size_;
    int			halflen_ = 30; // Hilbert Halflength
    float*		values_;
    static const char*	transMethodNamesStr(int);

private:

    bool		decompositionLoop(ObjectSet<IMFComponent>&,
					int maxnrimf,float stdevinput) const;
    void		stackEemdComponents(
				    const ObjectSet<ObjectSet<IMFComponent> >&,
				    ObjectSet<IMFComponent>&) const;
    void		stackCeemdComponents(
				    const ObjectSet<ObjectSet<IMFComponent> >&,
				    ObjectSet<IMFComponent>&,int nrimf) const;

    void		resetInput(const OrgTraceMinusAverage&);
    void		addDecompInputs(const DecompInput&);
    void		rescaleDecompInput(float scaler);
    void		subtractDecompInputs(const DecompInput&);
    void		replaceDecompInputs(const DecompInput&);
    void		createNoise(float stdev);

    void		computeStats(float&,float&) const;
    void		findExtrema(int& nrmax,int& nrmin,int& nrzeros,
				    bool symmetricboundary,
				    PointBasedMathFunction& maxima,
				    PointBasedMathFunction& minima) const;

    void		addZeroComponents(ObjectSet<IMFComponent>&,
					  int comp) const;
    void		addToComponent(ObjectSet<IMFComponent>&,
				       int comp, int nrzeros) const;
    void		retrieveFromComponent(const ObjectSet<IMFComponent>&,
					      int comp);

    bool		doHilbert(const ObjectSet<ObjectSet<IMFComponent> >&,
				  ObjectSet<IMFComponent>& imagcomp) const;
    bool		calcFrequencies(
				  const ObjectSet<ObjectSet<IMFComponent> >&,
				  const ObjectSet<IMFComponent>& phasecomps,
				  ObjectSet<IMFComponent>& frequencycomponents,
				  const float refstep) const;
    bool		calcAmplitudes(
				  const ObjectSet<ObjectSet<IMFComponent> >&,
				  const ObjectSet<IMFComponent>& imagcomps,
				  ObjectSet<IMFComponent>& ampcomps) const;
    bool		calcPhase(const ObjectSet<ObjectSet<IMFComponent> >&,
				  const ObjectSet<IMFComponent>& imagcomps,
				  ObjectSet<IMFComponent>& phasecomponents,
				  const float refstep) const;
    bool		outputAttribute(
				const ObjectSet<ObjectSet<IMFComponent> >&,
				Array2D<float>& output,int outputattrib,
				float startfreq,float endfreq,float stepoutfreq,
				int startcomp,int outputcomp,
				float average) const;

    bool		useGridding(
				const ObjectSet<ObjectSet<IMFComponent> >&,
				Array2D<float>& output,float startfreq,
				float endfreq,float stepoutfreq) const;
    bool		usePolynomial(
				const ObjectSet<ObjectSet<IMFComponent> >&,
				Array2D<float>& output,float startfreq,
				float endfreq,float stepoutfreq) const;
    bool		sortSpectrum(const float* unsortedamplitudes,int size,
				     float* unsortedfrequencies,
			    PointBasedMathFunction& sortedampspectrum) const;

    Stats::RandomGenerator* gen_;

private:
    //For debugging only:
    void		testFunction(int& nrmax,int& nrmin,int& nrzeros,
				     PointBasedMathFunction& maxima,
				     PointBasedMathFunction& minima) const;

    bool		dumpComponents(
				    const ObjectSet<ObjectSet<IMFComponent> >&,
				    const OrgTraceMinusAverage*) const;
    void		readComponents(
				    ObjectSet<ObjectSet<IMFComponent> >&) const;

};
