#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : April 2013
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


mClass(CEEMDAttrib) MyPointBasedMathFunction : public PointBasedMathFunction
{
public:

		MyPointBasedMathFunction( PointBasedMathFunction::InterpolType
			    t=PointBasedMathFunction::Poly,
		    PointBasedMathFunction::ExtrapolType
			    e = PointBasedMathFunction::ExtraPolGradient )
		    :PointBasedMathFunction( t, e ){};

    void	replace( int idx, float x, float y )
		{
		    x_[idx]=x;
		    y_[idx]=y;
		}

    float	myGetValue(float x) const
		{ return itype_ == Snap ? snapVal(x) : myInterpVal(x); }

    float	myInterpVal( float x ) const
		{
		    const int sz = x_.size();
		    if ( sz < 1 )
			return mUdf(float);

		    if ( x < x_[0] || x > x_[sz-1] )
			return myOutsideVal(x);
		    else if ( sz < 2 )
			return y_[0];

		    const int i0 = baseIdx( x );
		    const float v0 = y_[i0];
		    if ( i0 == sz-1 )
			return v0;

		    const float x0 = x_[i0];
		    const int i1 = i0 + 1;
		    const float x1 = x_[i1];
		    const float v1 = y_[i1];
		    const float dx = x1 - x0;
		    if ( dx == 0 )
			return v0;

		    const float relx = (x - x0) / dx;
		    if ( mIsUdf(v0) || mIsUdf(v1) )
			return relx < 0.5 ? v0 : v1;

		    // OK - we have 2 nearest points and they:
		    // - are not undef
		    // - don't coincide

		    if ( itype_ == Linear )
			return v1 * relx + v0 * (1-relx);

		    const int im1 = i0 > 0 ? i0 - 1 : i0;
		    const float xm1 = im1 == i0 ? x0 - dx : x_[im1];
		    const float vm1 = mIsUdf(y_[im1]) ? v0 : y_[im1];

		    const int i2 = i1 < sz-1 ? i1 + 1 : i1;
		    const float x2 = i2 == i1 ? x1 + dx : x_[i2];
		    const float v2 = mIsUdf(y_[i2]) ? v1 : y_[i2];

		    if ( mIsEqual(xm1,x0,1e-6) || mIsEqual(x0,x1,1e-6) ||
			 mIsEqual(x1,x2,1e-6) )
			return mUdf(float);

		    return Interpolate::poly1D( xm1, vm1, x0, v0, x1, v1,
						x2, v2, x );
		}

    float	myOutsideVal( float x ) const
		{
		    if ( extrapol_==None )
			return mUdf(float);

		    const int sz = x_.size();

		    if ( extrapol_==EndVal || sz<2 )
			return x-x_[0] < x_[sz-1]-x ? y_[0] : y_[sz-1];

		    if ( x<x_[0] )
		    {
			const float gradient =
			    mIsZero(x_[1]-x_[0], 1e-3 ) ?
			    mUdf(float) : (y_[1]-y_[0])/(x_[1]-x_[0]);
			return y_[0]+(x-x_[0])*gradient;
		    }

		    const float gradient =
			mIsZero(x_[sz-1]-x_[sz-2], 1e-3 ) ?
			mUdf(float) : (y_[sz-1]-y_[sz-2])/(x_[sz-1]-x_[sz-2]);
		    return y_[sz-1] + (x-x_[sz-1])*gradient;
		}
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
				    MyPointBasedMathFunction& maxima,
				    MyPointBasedMathFunction& minima) const;

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
				  const ObjectSet<IMFComponent>& imagcomps,
				  ObjectSet<IMFComponent>& frequencycomponents,
				  const float refstep) const;
    bool		calcAmplitudes(
				  const ObjectSet<ObjectSet<IMFComponent> >&,
				  const ObjectSet<IMFComponent>& imagcomps,
				  ObjectSet<IMFComponent>& ampcomps) const;
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
			    MyPointBasedMathFunction& sortedampspectrum) const;

    Stats::RandomGenerator* gen_;

private:
    //For debugging only:
    void		testFunction(int& nrmax,int& nrmin,int& nrzeros,
				     MyPointBasedMathFunction& maxima,
				     MyPointBasedMathFunction& minima) const;

    bool		dumpComponents(
				    const ObjectSet<ObjectSet<IMFComponent> >&,
				    const OrgTraceMinusAverage*) const;
    void		readComponents(
				    ObjectSet<ObjectSet<IMFComponent> >&) const;

};

