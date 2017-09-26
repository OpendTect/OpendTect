#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : April 2013
-*/

#include "ceemdattribmod.h"
#include "commondefs.h"
#include "gendefs.h"
#include "arrayndimpl.h"
#include "mathfunc.h"
#include "manobjectset.h"
#include "interpol1d.h"

#define mDecompModeEMD		0
#define mDecompModeEEMD		1
#define mDecompModeCEEMD	2

#define mDecompOutputFreq	0
#define mDecompOutputPeakFreq	1
#define mDecompOutputPeakAmp	2
#define mDecompOutputIMF	3

namespace CEEMD
{

mExpClass(CEEMDAttrib) IMFComponent
{
public:
			IMFComponent( int nrsamples )
				: values_( new float[nrsamples] )
				, size_( nrsamples )
			{}

			IMFComponent( const IMFComponent& comp )
				: values_( new float[comp.size_] )
				, size_( comp.size_ )
				, name_( comp.name_ )
				, nrzeros_( comp.nrzeros_ )
			{
			    for ( int idx=0; idx<size_; idx++ )
				values_[idx] = comp.values_[idx];
			}

			~IMFComponent() { delete [] values_; }

    BufferString	name_;
    int			nrzeros_;
    int			size_;
    float*		values_;

};

mExpClass(CEEMDAttrib) MyPointBasedMathFunction : public PointBasedMathFunction
{
public:

		MyPointBasedMathFunction( PointBasedMathFunction::InterpolType
			    t=PointBasedMathFunction::Poly,
		    PointBasedMathFunction::ExtrapolType
			    e = PointBasedMathFunction::ExtraPolGradient )
		    :PointBasedMathFunction( t, e ){};

    void	replace(int idx, float x, float y)
		{
		    x_[idx]=x;
		    y_[idx]=y;
		}
    float	myGetValue(float x) const
		{ return itype_ == Snap ? snapVal(x) : myInterpVal(x); }

    float	myInterpVal( float x ) const
		{
		    const int sz = x_.size();
		    if ( sz < 1 ) return mUdf(float);

		    if ( x < x_[0] || x > x_[sz-1] )
			return myOutsideVal(x);
		    else if ( sz < 2 )
			return y_[0];

		    bool ispresent;
		    const int i0 = baseIdx( x, ispresent );
		    const float v0 = y_[i0];
		    if ( i0 == sz-1 )
			return v0;

		    const float x0 = x_[i0];
		    const int i1 = i0 + 1;
		    const float x1 = x_[i1];
		    const float v1 = y_[i1];
		    const float dx = x1 - x0;
		    if ( dx == 0 ) return v0;

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

		    if ( mIsEqual(xm1,x0,1e-6) || mIsEqual(x0,x1,1e-6)
			    || mIsEqual(x1,x2,1e-6) )
			return mUdf(float);

		    return Interpolate::poly1D( xm1, vm1, x0, v0, x1, v1,
						x2, v2, x );
		}

    float	myOutsideVal( float x ) const
		{
		    if ( extrapol_==None ) return mUdf(float);

		    const int sz = x_.size();

		    if ( extrapol_==EndVal || sz<2 )
		    {
			return x-x_[0] < x_[sz-1]-x ? y_[0] : y_[sz-1];
		    }

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

mExpClass(CEEMDAttrib) OrgTraceMinusAverage
{
public:
		    OrgTraceMinusAverage( int nrsamples )
			    : values_( new float[nrsamples] )
			    , size_( nrsamples )
		    {}
		    ~OrgTraceMinusAverage() { delete [] values_; }

    BufferString    name_;
    float	    averageinput_;
    float	    stdev_;
    int		    size_;
    float*	    values_;

};

mExpClass(CEEMDAttrib) Setup
{
public:
		Setup();

		mDefSetupMemb(int, method); // 0=EMD, 1=EEMD, 2=CEEMD
		// 0=Freq, 1=Peak Freq, 2= Peak Amp, 3=IMF
		mDefSetupMemb(int, attriboutput);
		// Number of realizations for EEMD and CEEMD
		mDefSetupMemb(int, maxnoiseloop);
		// Maximum number of intrinsic Mode Functions
		mDefSetupMemb(int, maxnrimf);
		// Maximum number of sifting iterations
		mDefSetupMemb(int, maxsift);
		// stop sifting if st.dev res.-imf < value
		mDefSetupMemb(float, stopsift);
		// stop decomp. when st.dev imf < value
		mDefSetupMemb(float, stopimf);
		// noise percentage for EEMD and CEEMD
		mDefSetupMemb(float, noisepercentage);
		// boundary extension symmetric or periodic
		mDefSetupMemb(bool, symmetricboundary);
		// use synthetic trace in ceemdtestprogram.h
		mDefSetupMemb(bool, usetestdata);
		// output frequency.
		mDefSetupMemb(bool, outputfreq);
		// step output frequency.
		mDefSetupMemb(bool, stepoutfreq);
		// output IMF component.
		mDefSetupMemb(bool, outputcomp);
};


mExpClass(CEEMDAttrib) DecompInput
{
public:
		    DecompInput( const Setup& setup, int nrsamples )
			    : values_( new float[nrsamples] )
			    , size_( nrsamples )
			    , setup_(setup)
			    , halflen_(30) // Hilbert Halflength
		    {}
		    ~DecompInput() { delete [] values_; }

    bool doDecompMethod( int nrsamples , float refstep,
			   Array2DImpl<float>* output, int outputattrib,
			   int startfreq, int endfreq, int stepoutfreq,
			   int startcomp, int endcomp );

    Setup	setup_;
    int		size_;
    int		halflen_;
    float*	values_;
    static const char*	transMethodNamesStr(int);

protected:

    void computeStats(float&, float&) const;
    void createNoise(float stdev) const;
    void addDecompInputs(const DecompInput* arraytoadd) const;
    void rescaleDecompInput(float scaler) const;
    void subtractDecompInputs( const DecompInput* arraytosubtract ) const;
    void replaceDecompInputs(const DecompInput* replacement) const;
    void retrieveFromComponent(
		    const ManagedObjectSet<IMFComponent>& components,
		    int comp) const;
    void addToComponent(
		    ManagedObjectSet<IMFComponent>& components,
		    int comp, int nrzeros) const;
    void addZeroComponents(
		    ManagedObjectSet<IMFComponent>& components,
		    int comp ) const;
    void findExtrema(
		    int& nrmax, int& nrmin, int& nrzeros ,
		    bool symmetricboundary ,
		    MyPointBasedMathFunction& maxima ,
		    MyPointBasedMathFunction& minima) const;
    void testFunction(
		    int& nrmax, int& nrmin, int& nrzeros ,
		    MyPointBasedMathFunction& maxima ,
		    MyPointBasedMathFunction& minima) const;
    void resetInput( const OrgTraceMinusAverage* orgminusaverage) const;
    bool decompositionLoop(
		    ManagedObjectSet<IMFComponent>& components ,
		    int maxnrimf , float stdevinput) const;
    void stackEemdComponents(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	ManagedObjectSet<IMFComponent>& stackedcomponents) const;
    void stackCeemdComponents(
		    const ManagedObjectSet<ManagedObjectSet<IMFComponent> >&
							currentrealizations,
		    ManagedObjectSet<IMFComponent>& currentstackedcomponents,
		    int nrimf ) const;
    bool dumpComponents(
		    OrgTraceMinusAverage* orgminusaverage,
		    const ManagedObjectSet<ManagedObjectSet<IMFComponent> >&
							realizations) const;
    void readComponents( ManagedObjectSet<ManagedObjectSet<IMFComponent> >&
							realizations ) const;
    bool doHilbert(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	ManagedObjectSet<IMFComponent>& imagcomponents) const;
    bool calcFrequencies(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	const ManagedObjectSet<IMFComponent>& imagcomponents,
	ManagedObjectSet<IMFComponent>& frequencycomponents,
	const float refstep) const;
    bool calcAmplitudes(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	const ManagedObjectSet<IMFComponent>& imagcomponents,
	const ManagedObjectSet<IMFComponent>& frequencycomponents,
	ManagedObjectSet<IMFComponent>& amplitudecomponents) const;
    bool outputAttribute(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, int outputattrib,
	int startfreq, int endfreq, int stepoutfreq,
	int startcomp, int outputcomp, float average) const;
    bool useGridding(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, int startfreq, int endfreq,
	int stepoutfreq) const;
    bool usePolynomial(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, int startfreq, int endfreq,
	int stepoutfreq) const;
    bool sortSpectrum(
	float* unsortedfrequencies, float* unsortedamplitudes,
	MyPointBasedMathFunction& sortedampspectrum, int size ) const;

};

} // namespace CEEMD
