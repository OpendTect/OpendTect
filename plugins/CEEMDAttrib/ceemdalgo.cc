/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : April 2013
-*/


#include "ceemdalgo.h"
#include "statruncalc.h"
#include "statrand.h"
#include <math.h>
#include "ceemdtestprogram.h"
#include "hilberttransform.h"
#include "arrayndimpl.h"
#include "undefval.h"
#include "sorting.h"
#include "odmemory.h"
#include "od_iostream.h"
#include "gridder2d.h"
#include "survinfo.h"

using namespace std;

Setup::Setup()
    : usetestdata_(false)
    , method_(2)
    , maxnrimf_(16)
    , maxsift_(10)
    , stopsift_(0.2)
    , stopimf_(0.005)
    , symmetricboundary_(true)
    , noisepercentage_(10.0)
    , maxnoiseloop_(50)
    , outputfreq_(5)
    , stepoutfreq_(5)
    , attriboutput_(4)
     {}

void DecompInput::testFunction(
	    int& nrmax, int& nrmin, int& nrzeros ,
	    MyPointBasedMathFunction& maxima ,
	    MyPointBasedMathFunction& minima ) const
{
    // function dumps the contents of input trace and
    // derived min and max envelops
    od_ostream strm( "testdata.txt" );
    strm << "input" << '\t';
    for ( int idx=0; idx<size_; idx++ )
    {
	float val = values_[idx];
	strm << val << '\t';
    }
    strm << '\n' << "Envelop max:" << '\t';
    for ( int idx=0; idx<size_; idx++ )
    {
	float val = maxima.getValue(mCast(float,idx));
	strm << val << '\t';
   }
    strm << '\n' << "Envelop min:" << '\t';
    for ( int idx=0; idx<size_; idx++ )
    {
	float val = minima.getValue(mCast(float,idx));
	strm << val << '\t';
    }
}

bool DecompInput::dumpComponents(
	OrgTraceMinusAverage* orgminusaverage,
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >&
							realizations ) const
{
    // write output to file
    od_ostream strm( "components.txt" );
 //   strm << "input-average" << '\t';

 //   strm << orgminusaverage->averageinput_ << '\t';
 //
 //   for ( int idx=0; idx<size_; idx++ )
 //   {
	//float val = orgminusaverage->values_[idx];
	//strm << val << '\t';
 //   }

    for (int real=0; real<realizations.size(); real++)
    {

	for ( int comp=0; comp<realizations[real]->size(); comp++)
	{
	    const IMFComponent* currentcomp = (*realizations[real])[comp];
	    //strm << '\n' <<  currentcomp->name_<< '\t';
	    //strm << currentcomp->nrzeros_ << '\t';

	    for ( int idx=0; idx<size_; idx++ )
	    {
		float val = currentcomp->values_[idx];
		strm << val; strm.addTab();
	    }
	    strm.addNewLine();
	}
    }

    return true;
}

void DecompInput::readComponents(
	ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations ) const
{
    int nrsamples = 2001;
    int nrcomp = 11;
    od_istream strm( "TestData_Components_1-11_MS-DOS.txt" );
    ManagedObjectSet<IMFComponent>* components =
	new ManagedObjectSet<IMFComponent>();

    for ( int i=0; i<nrcomp; i++)
    {
	IMFComponent* currentcomp = new IMFComponent( nrsamples );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    float val;
	    strm >> val;
	    currentcomp->values_[idx] = val;
	}
	*components += currentcomp;
    }

    realizations += components;
}

void DecompInput::computeStats(
	float& average, float& stdev ) const
{
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::Average );
    rcsetup.require( Stats::StdDev );
    Stats::RunCalc<float> stats( rcsetup );
    for ( int idx=0; idx<size_ - 1; idx++ )
    {
	if ( mIsUdf(values_[idx]) )
	    continue;
	stats += values_[idx];
    }
    average = mCast(float,stats.average());
    stdev = mCast(float,stats.stdDev());
    return;
}

void DecompInput::findExtrema(
	    int& nrmax, int& nrmin, int& nrzeros ,
	    bool symmetricboundary ,
	    MyPointBasedMathFunction& maxima ,
	    MyPointBasedMathFunction& minima ) const
{
    nrmax=0; nrmin=0; nrzeros=0;
    maxima.setEmpty();
    minima.setEmpty();
    minima.add( 0.0, 0.0); // to be replaced by boundary point
    minima.add( 1.0, 0.0); // to be replaced by boundary point
    maxima.add( 0.0, 0.0); // to be replaced by boundary point
    maxima.add( 1.0, 0.0); // to be replaced by boundary point
    for ( int idx=0; idx<size_-3; idx++ )
    {
	float val = values_[idx];
	float val1 = values_[idx+1];
	float val2 = values_[idx+2];
	float val3 = values_[idx+3];
	if (val1>val && val1>val2)
	{
	    maxima.add( float(idx+2) , val1 );
	    nrmax = nrmax+1;
	}
	if ((val1>val) && (val1==val2) && (val2>val3))
	{
	    maxima.add( float(idx+2) , val1 );
	    nrmax = nrmax+1;
	}

	if (val1<val && val1<val2)
	{
	    minima.add( float(idx+2) , val1 );
	    nrmin = nrmin+1;
	}
	if ((val1<val) && (val1==val2) && (val2<val3))
	{
	    minima.add( float(idx+2) , val1 );
	    nrmin = nrmin+1;
	}
	if ( (val<0 && val1>0)	|| (val>0 && val1<0) )
		nrzeros=nrzeros+1;
    }

    // Extend boundaries
    if ((nrmin <= 1) || (nrmax <= 1)) return;
    if ( symmetricboundary == true ) // symmetric extension
    {
	minima.replace( 0, -minima.xVals()[3], minima.yVals()[3] );
	minima.replace( 1, -minima.xVals()[2], minima.yVals()[2] );
	maxima.replace( 0, -maxima.xVals()[3], maxima.yVals()[3] );
	maxima.replace( 1, -maxima.xVals()[2], maxima.yVals()[2] );

	float lastx, lasty;
	lastx = minima.xVals()[nrmin+1];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = minima.yVals()[nrmin+1];
	minima.add( lastx, lasty );
	lastx = minima.xVals()[nrmin];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = minima.yVals()[nrmin];
	minima.add( lastx, lasty );

	lastx = maxima.xVals()[nrmax+1];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = maxima.yVals()[nrmax+1];
	maxima.add( lastx, lasty );
	lastx = maxima.xVals()[nrmax];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = maxima.yVals()[nrmax];
	maxima.add( lastx, lasty );
    }
    else // periodic extension
    {
	minima.replace( 1, -minima.xVals()[3], minima.yVals()[3] );
	minima.replace( 0, -minima.xVals()[2], minima.yVals()[2] );
	maxima.replace( 1, -maxima.xVals()[3], maxima.yVals()[3] );
	maxima.replace( 0, -maxima.xVals()[2], maxima.yVals()[2] );

	float lastx, lasty;
	lastx = maxima.xVals()[nrmax+1];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = maxima.yVals()[nrmax+1];
	maxima.add( lastx, lasty );
	lastx = maxima.xVals()[nrmax];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = maxima.yVals()[nrmax];
	maxima.add( lastx, lasty );

	lastx = minima.xVals()[nrmin+1];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = minima.yVals()[nrmin+1];
	minima.add( lastx, lasty );
	lastx = minima.xVals()[nrmin];
	lastx = 2*float(size_ - 1) - lastx;
	lasty = minima.yVals()[nrmin];
	minima.add( lastx, lasty );
    }
}

bool DecompInput::decompositionLoop(
	ManagedObjectSet<IMFComponent>& components,
	int maxnrimf, float stdevinput ) const
{
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);

    int nrimf=0, nriter=0;
    bool isimf=false;
    bool isresidual=false;
    int nrmax, nrmin, nrzeros;
    float mean=0, average, stdev;
    float dosift, stopimf;
    IMFComponent* residual = new IMFComponent( size_ );

    for ( int idx=0; idx<size_; idx++ )
    {
	float val = values_[idx];
	residual->values_[idx] = val;
    }

    computeStats( average, stdev );
    stopimf = stdev / stdevinput;
    dosift = 1.0;

    while ( nrimf < maxnrimf )
    {
	if (isresidual) // fill up with zeroes
	{
	    addZeroComponents( components, nrimf );
	    break;
	}

	IMFComponent* currentcomp = new IMFComponent( size_ );
	for ( int idx=0; idx<size_; idx++ )
	{
	    currentcomp->values_[idx] = 0;
	}
	currentcomp->name_ = "Component ";
	currentcomp->name_ += nrimf+1;
	nriter=0;
	stopimf = stdev / stdevinput;
	dosift = 1.0;
	isresidual=false;
	isimf=false;

	while (nriter < setup_.maxsift_)
	{
	    if (isimf) break;

	    findExtrema( nrmax, nrmin,
		    nrzeros, setup_.symmetricboundary_ , maxima, minima );
	    computeStats( average, stdev );
	    stopimf = stdev / stdevinput;

	    if (( nrmin + nrmax + nrzeros < 1 )
		|| ( stopimf < setup_.stopimf_ )) // end of decomposition
	    {
		for ( int idx=0; idx<size_; idx++ )
		{
		    currentcomp->values_[idx] = residual->values_[idx];
		}
		currentcomp->nrzeros_ = nrzeros;
		components += currentcomp;
		isimf=true;
		isresidual=true;
		break;
	    }

	    // Check if data is a true IMF
	    else if ((( abs( nrmax+nrmin-nrzeros ) <= 1)
		|| ( dosift < setup_.stopsift_ ))
		|| ( nriter == setup_.maxsift_ -1))
	    {
		for ( int idx=0; idx<size_; idx++ )
		{
		    currentcomp->values_[idx] = values_[idx];
		    residual->values_[idx] -= values_[idx];
		    values_[idx] = residual->values_[idx];
		}
		isimf=true;
		currentcomp->nrzeros_ = nrzeros;
		stopimf = stdev / stdevinput;
		dosift = 1.0;
		components += currentcomp;
	    }

	    else // continue sifting
	    {
		for ( int idx=0; idx<size_; idx++ )
		{
		    float max = maxima.getValue(mCast(float,idx));
		    float min = minima.getValue(mCast(float,idx));
		    mean = ( (max+min)/2 );
		    float val = values_[idx];
		    dosift += ( (mean*mean) / (val*val));
		    values_[idx] = values_[idx] - mean;
		}
	    }
	    nriter++;
	}
    nrimf++;
    }
    delete residual;
    return isresidual;
}

void DecompInput::stackCeemdComponents(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >&
							currentrealizations,
	ManagedObjectSet<IMFComponent>& currentstackedcomponents,
	int nrimf ) const
{
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);

    DecompInput imf(setup_, size_);
    int stackcount = setup_.maxnoiseloop_;
    int nrmax, nrmin, nrzeros;

    currentstackedcomponents += new IMFComponent(size_);
    currentstackedcomponents[0]->name_ = "Component ";
    currentstackedcomponents[0]->name_ += nrimf+1;

    for ( int idx=0; idx<size_; idx++)
    {
	if ( !currentrealizations.size() )
	    continue;
	const float val = (*currentrealizations[0])[0]->values_[idx];
	if ( mIsUdf(val) )
	    continue;
	currentstackedcomponents[0]->values_[idx] = val / stackcount;
    }

    for ( int nrnoise=1; nrnoise < setup_.maxnoiseloop_; nrnoise++)
    {
	for ( int idx=0; idx<size_; idx++)
	{
	    if ( !currentrealizations.size() )
		continue;
	    const float val = (*currentrealizations[nrnoise])[0]->values_[idx];
	    if ( mIsUdf(val) )
		continue;
	    currentstackedcomponents[0]->values_[idx] += val / stackcount;
	}
    }

    for ( int idx=0; idx < size_; idx++ )
	imf.values_[idx] = currentstackedcomponents[0]->values_[idx] ;

    imf.findExtrema( nrmax, nrmin,
	nrzeros, setup_.symmetricboundary_ , maxima, minima );
    currentstackedcomponents[0]->nrzeros_ = nrzeros;

}

void DecompInput::stackEemdComponents(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	ManagedObjectSet<IMFComponent>& stackedcomponents ) const
{
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);
    DecompInput imf(setup_, size_);
    int stackcount = setup_.maxnoiseloop_;
    int maxcomp = 0;
    int nrmax, nrmin, nrzeros;

    for ( int comp=0; comp < setup_.maxnrimf_; comp++)
    {
	maxcomp = comp;
	int checkzeros = 0;
	for ( int nrnoise=0; nrnoise < stackcount; nrnoise++)
	{
	    if ((*realizations[nrnoise])[comp]->nrzeros_ == -1)
		checkzeros = checkzeros + 1;
	}
	if ( checkzeros == setup_.maxnoiseloop_ ) break;
    }

    for ( int comp=0; comp < maxcomp; comp++)
    {
	stackedcomponents += new IMFComponent(size_);
	stackedcomponents[comp]->name_ = "Component ";
	stackedcomponents[comp]->name_ += comp+1;

	for ( int idx=0; idx<size_; idx++)
	    stackedcomponents[comp]->values_[idx] = 0;

	for ( int nrnoise=0; nrnoise < setup_.maxnoiseloop_; nrnoise++ )
	{
	    for ( int idx=0; idx<size_; idx++)
	    {
	    if ((*realizations[nrnoise])[comp]->nrzeros_ == -1) continue;
	    else
		stackedcomponents[comp]->values_[idx] +=
		    (*realizations[nrnoise])[comp]->values_[idx]
		    / stackcount;
	    }
	}
	for ( int idx=0; idx<size_; idx++ )
	    imf.values_[idx] = stackedcomponents[comp]->values_[idx];

	imf.findExtrema( nrmax, nrmin,
	    nrzeros, setup_.symmetricboundary_ , maxima, minima );
	stackedcomponents[comp]->nrzeros_ = nrzeros;
    }
    return;
}

void DecompInput::resetInput( const OrgTraceMinusAverage* orgminusaverage) const
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = orgminusaverage->values_[idx];

    return;
}

void DecompInput::addDecompInputs( const DecompInput* arraytoadd ) const
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] += arraytoadd->values_[idx];

     return;
}

void DecompInput::rescaleDecompInput( float scaler) const
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = values_[idx] * scaler;

    return;
}

void DecompInput::subtractDecompInputs(
	    const DecompInput* arraytosubtract ) const
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] -= arraytosubtract->values_[idx];

    return;
}

void DecompInput::replaceDecompInputs(
	    const DecompInput* replacement ) const
{
     for ( int idx=0; idx<size_; idx++ )
    {
	values_[idx] = replacement->values_[idx];
    }
    return;
}

void DecompInput::addZeroComponents(
	    ManagedObjectSet<IMFComponent>& components,
	    int comp ) const
{
    for ( int nrimf=comp; nrimf<setup_.maxnrimf_; nrimf++)
    {
	IMFComponent* zerocomp = new IMFComponent( size_ );
	zerocomp->name_ = "Zeroes";
	zerocomp->nrzeros_ = -1; // can be used to recognize zero component
	for ( int idx=0; idx<size_; idx++ )
	{
	    zerocomp->values_[idx] = 0;
	}
	components += zerocomp;
    }
    return;
}

void DecompInput::retrieveFromComponent(
		    const ManagedObjectSet<IMFComponent>& components,
		    int comp) const
{
     for ( int idx=0; idx<size_; idx++ )
    {
	values_[idx] = components[comp]->values_[idx];
    }
    return;
}

void DecompInput::addToComponent(
		    ManagedObjectSet<IMFComponent>& components,
		    int comp, int nrzeros) const
{
     for ( int idx=0; idx<size_; idx++ )
    {
	components[comp]->values_[idx] = values_[idx];
    }
    components[comp]->name_ = "Component ";
    components[comp]->name_ += comp+1;
    components[comp]->nrzeros_ = nrzeros;
    return;
}

void DecompInput::createNoise( float stdev ) const
{
    double dnoise, sd;
    int seed=0;
    sd = mCast(double, stdev);
    Stats::randGen().init( seed );
    for ( int idx=0; idx<size_; idx++ )
    {
	dnoise = Stats::NormalRandGen().get( 0, sd );
	values_[idx] = mCast(float, dnoise);
    }
    return;
}

bool DecompInput::doHilbert(
    const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	ManagedObjectSet<IMFComponent>& imagcomponents ) const
{
    const int hilbfilterlen = halflen_*2 + 1;
    const bool enoughsamps = size_ >= hilbfilterlen;
    const int arrminnrsamp = hilbfilterlen
				? size_ : hilbfilterlen;
    const int shift = 0;
    int inpstartidx = 0;
    int startidx = enoughsamps ? shift : 0;
    int comp = 0; // first component
    Array1DImpl<float> createarr( arrminnrsamp );
    ValueSeries<float>* padtrace = 0;
    if ( !enoughsamps )
    {
	for ( int idx=0; idx<arrminnrsamp; idx++ )
	{
	    const float val = idx < size_ ?
		(*realcomponents[0])[comp]->values_[idx] : 0;
	    createarr.set( idx, val );
	}

	padtrace = createarr.getStorage();
	if ( !padtrace )
	    return false;

	startidx = shift;
    }

    HilbertTransform ht;
    if ( !ht.init() )
	return false;

    ht.setCalcRange( startidx, inpstartidx );
    Array1DImpl<float> outarr( arrminnrsamp );
    Array1DImpl<float> inparr( arrminnrsamp );

    for ( int compidx=0; compidx<realcomponents[0]->size(); compidx++ )
    {
	IMFComponent* imagcomp = new IMFComponent( size_ );
	for ( int idx=0; idx<size_; idx++ )
	    inparr.set(shift+idx, (*realcomponents[0])[compidx]->values_[idx]);

	const bool transformok = enoughsamps
		    ? ht.transform(inparr, size_, outarr, size_ )
		    : ht.transform(*padtrace,arrminnrsamp,
				    outarr, size_ );
	if ( !transformok )
	    return false;

	for ( int idx=0; idx<size_; idx++ )
	    imagcomp->values_[idx] = outarr.get(shift+idx);

	imagcomponents += imagcomp;
    }
    return true;
}

#define mCheckRetUdf(val1,val2) \
    if ( mIsUdf(val1) || mIsUdf(val2) ) continue;

bool DecompInput::calcFrequencies(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	const ManagedObjectSet<IMFComponent>& imagcomponents,
	ManagedObjectSet<IMFComponent>& frequencycomponents,
	const float refstep ) const
{
    for ( int comp=0; comp<realcomponents[0]->size(); comp++)
    {
	IMFComponent* freqcomp = new IMFComponent( size_ );
	for ( int idx=1; idx<size_-1; idx++ )
	{
	    const float realval = (*realcomponents[0])[comp]->values_[idx];
	    const float prevreal = (*realcomponents[0])[comp]->values_[idx-1];
	    const float nextreal = (*realcomponents[0])[comp]->values_[idx+1];
	    mCheckRetUdf( prevreal, nextreal );
	    const float dreal_dt = (nextreal - prevreal) / (2*refstep);

	    const float imagval = imagcomponents[comp]->values_[idx];
	    const float previmag = imagcomponents[comp]->values_[idx-1];
	    const float nextimag = imagcomponents[comp]->values_[idx+1];
	    mCheckRetUdf( previmag, nextimag );
	    const float dimag_dt = (nextimag-previmag) / (2*refstep);

	    float denom = (realval*realval + imagval*imagval) * 2
			  * mCast(float,M_PI);
	    if ( mIsZero( denom, 1e-6 ) ) denom = 1e-6;
	    float freqval = ( (realval*dimag_dt-dreal_dt*imagval) / denom);
	    freqcomp->values_[idx] = freqval;
	}
	// copy boundary points to keep number of samples = size_
	freqcomp->values_[0] = freqcomp->values_[1];
	freqcomp->values_[size_-1] = freqcomp->values_[size_-2];
	frequencycomponents += freqcomp;
    }
    return true;
}

bool DecompInput::useGridding(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, float startfreq, float endfreq,
	float stepoutfreq ) const
{
    int fsize = realizations[0]->size();
    double step = mCast(double, stepoutfreq);
    step = max ( step, 1. );
    TriangulatedGridder2D grdr;
    TypeSet<Coord> pts; TypeSet<float> zvals;
    for ( int idt=0; idt<size_; idt++ )
    {
	double maxf = 0;
	double minf = 0;
	for ( int f=0; f<fsize; f++ )
	{
	    for ( int t=-1; t<2; t++ ) // grid over three traces
	    {
		double dff =
		    mCast(double,
		    (*realizations[2])[f]->values_[idt+t]);
		double dtt = mCast(double, idt+t);
		float z = (*realizations[3])[f]->values_[idt+t];
		maxf = max ( maxf, dff );
		minf = min ( minf, dff );
		pts.add(Coord(dff,dtt));
		zvals.add(z);
	    }
	}
	for ( int t=-1; t<2; t++ ) // add zeros at either end
	{
	    double dtt = mCast(double, idt+t);
	    pts.add(Coord( minf-step, dtt));
	    zvals.add(0);
	    pts.add(Coord( maxf+step, dtt));
	    zvals.add(0);
	}
	grdr.setPoints( pts );
	grdr.setValues( zvals );

	for ( float f=startfreq; f<endfreq; f+=stepoutfreq )
	{
	    if ( f >= output->info().getSize(1) ) break;
	    double dff = f;
	    double dtt = idt;
	    float val;
	    if ( f < minf || f > maxf ) val = 0;
	    else
	    {
		val = grdr.getValue( Coord(dff,dtt) );
		if ( mIsUdf(val) ) val = 0;
	    }
	    output->set( idt, mNINT32(f/stepoutfreq), val );
	}
	pts.erase();
	zvals.erase();
    }
    // copy boundary points to keep number of samples = size_

    for ( float f=startfreq; f<endfreq; f+=stepoutfreq )
    {
	int yidx = mNINT32(f/stepoutfreq);
	float valzero = output->get( 1, yidx ) ;
	output->set( 0, yidx, valzero );
	float vallast = output->get( size_-2, yidx ) ;
	output->set( size_-1, yidx, vallast );
    }
    return true;
}

bool DecompInput::usePolynomial(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, float startfreq, float endfreq,
	float stepoutfreq ) const
{
    int fsize = realizations[0]->size();
    float* unsortedamplitudes = new float[fsize+2];
    float* unsortedfrequencies = new float[fsize+2];
    MyPointBasedMathFunction sortedampspectrum(
	    PointBasedMathFunction::Poly,
	    PointBasedMathFunction::ExtraPolGradient);
    const int convfac = SI().zIsTime() ? 1 : 1000;

    const float stepout = stepoutfreq*convfac;


    for ( int idt=0; idt<size_; idt++ )
    {
	float maxf = 0;
	float minf = 0;
	for ( int f=0; f<fsize; f++ )
	{
	    unsortedfrequencies[f] = (*realizations[2])[f]->values_[idt];
	    unsortedamplitudes[f] = (*realizations[3])[f]->values_[idt];
	    maxf = max ( maxf, unsortedfrequencies[f] );
	    minf = min ( minf, unsortedfrequencies[f] );
	}
	// add zeros either end
	unsortedfrequencies[fsize] = minf-mCast(float, stepoutfreq);
	unsortedamplitudes[fsize] = 0;
	unsortedfrequencies[fsize+1] = maxf+mCast(float, stepoutfreq);
	unsortedamplitudes[fsize+1] = 0;

	sortSpectrum( unsortedfrequencies, unsortedamplitudes,
	    sortedampspectrum, fsize+2 );



	for ( float f=startfreq*convfac; f<=endfreq*convfac; f+=stepout )
	{
	    int yidx = mNINT32(f/stepout);
	    if ( yidx == 0 )
		continue;

	    if ( yidx > output->info().getSize(1) ) break;
	    float val;
	    if ( f < (minf*convfac) || f > (maxf*convfac) ) val = 0;
	    else
	    {
		val = sortedampspectrum.getValue(f/convfac) ;
		if ( mIsUdf(val) || ( val<0 ) )
		    val = 0;
	    }

	    output->set( idt, yidx-1, val );
	}
    }
    // copy boundary points to keep number of samples = size_
    for ( float f=startfreq*convfac; f<endfreq*convfac; f+=stepoutfreq*convfac )
    {
	int yidx = mNINT32(f/(stepoutfreq*convfac));
	if ( yidx == 0 )
	    continue;

	float valzero = output->get( 1, yidx-1 ) ;
	output->set( 0, yidx-1, valzero );
	float vallast = output->get( size_-2, yidx-1 ) ;
	output->set( size_-1, yidx-1, vallast );
    }

    delete[] unsortedfrequencies;
    delete[] unsortedamplitudes;
    return true;
}

bool DecompInput::sortSpectrum(
	    float* unsortedfrequencies,  float* unsortedamplitudes,
	    MyPointBasedMathFunction& sortedampspectrum, int size ) const
{
    int* indexsortedamplitudes = new int[size];
    sortedampspectrum.setEmpty();

    for ( int i=0; i<size; i++ )
	indexsortedamplitudes[i] = i;

    sort_coupled( unsortedfrequencies, indexsortedamplitudes, size );

    for (int i=0; i<size; i++ )
    {
	int index = indexsortedamplitudes[i];
	float freq = unsortedfrequencies[i];
	float amp = unsortedamplitudes[index];
	sortedampspectrum.add( freq, amp );
    }

    delete[] indexsortedamplitudes;
    return true;
}


bool DecompInput::outputAttribute(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realizations,
	Array2DImpl<float>* output, int outputattrib,
	float startfreq, float endfreq, float stepoutfreq,
	int startcomp, int endcomp, float average ) const
{

    if ( outputattrib == 0 )  // output instantaneous frequency
    {
	bool gridding = false; // 3-row gridding or 1-row polynomial
	if (gridding)
	    useGridding( realizations, output,
		startfreq, endfreq, stepoutfreq );
	else
	    usePolynomial( realizations, output,
		startfreq, endfreq, stepoutfreq );
    }
    else if ( outputattrib == 1) // output peak frequency
    {
	int size = realizations[2]->size();
	for ( int idx=0; idx<size_; idx++ )
	{
	    float peakfreq = 0;
	    for ( int comp=0; comp<size; comp++ )
	    {
		peakfreq = max(
		    peakfreq, (*realizations[2])[comp]->values_[idx] );
	    }
	    output->set( idx, 0, peakfreq );
	}
    }
    else if ( outputattrib == 2) // output peak amplitude
    {
	int size = realizations[2]->size();
	for ( int idx=0; idx<size_; idx++ )
	{
	    float peakfreq = 0;
	    int index = 0;
	    for ( int comp=0; comp<size; comp++ )
	    {
		if ( (*realizations[2])[comp]->values_[idx] > peakfreq )
		{
		    peakfreq = (*realizations[2])[comp]->values_[idx];
		    index = comp;
		}
	    }
	    float val = (*realizations[3])[index]->values_[idx];
	    output->set( idx, 0, val );
	}
    }
    else  // output IMF Component (= real part)
    {
	int size = realizations[0]->size();
	for ( int comp=startcomp; comp<=endcomp && comp<size; comp++ )
	{
	    if ( comp>= output->info().getSize(1) ) break;
	    for ( int idx=0; idx<size_; idx++ )
	    {
		float val =  (*realizations[0])[comp]->values_[idx];
		output->set( idx, comp, val );
	    }
	}
	// add zeroes
	for ( int comp=size; comp<setup_.maxnrimf_; comp++ )
	{
	    if ( comp>= output->info().getSize(1) ) break;
	    for ( int idx=0; idx<size_; idx++ )
		output->set( idx, comp, 0 );
	}
	// add average of input trace (DC) in last component
	for ( int idx=0; idx<size_; idx++ )
	    output->set( idx, setup_.maxnrimf_, average );
    }
    return true;
}

bool DecompInput::calcAmplitudes(
	const ManagedObjectSet<ManagedObjectSet<IMFComponent> >& realcomponents,
	const ManagedObjectSet<IMFComponent>& imagcomponents,
	const ManagedObjectSet<IMFComponent>& frequencycomponents,
	ManagedObjectSet<IMFComponent>& amplitudecomponents ) const
    {
	int size = realcomponents[0]->size();

	for ( int comp=0; comp<size; comp++)
	{
	    IMFComponent* amplitudecomp = new IMFComponent( size_ );
	    for ( int idx=0; idx<size_; idx++ )
	    {
		float realval = (*realcomponents[0])[comp]->values_[idx];
		float imagval = -imagcomponents[comp]->values_[idx];

		amplitudecomp->values_[idx] =
			sqrt (realval*realval + imagval*imagval);
	    }
	    amplitudecomponents += amplitudecomp;
	}
	return true;
    }


bool DecompInput::doDecompMethod(
	    int nrsamples, float refstep,
	    Array2DImpl<float>* output, int outputattrib,
	    float startfreq, float endfreq, float stepoutfreq,
	    int startcomp, int endcomp )
{
    DecompInput* imf = new DecompInput( setup_, nrsamples );
    DecompInput* currentcomp = new DecompInput( setup_, nrsamples );
    DecompInput* residual = new DecompInput( setup_, nrsamples );
    DecompInput* noisearray = new DecompInput( setup_, nrsamples );
    DecompInput* copynoisearray = new DecompInput( setup_, nrsamples );
    int nrnoise=0, nrimf=0, nrmax=0, nrmin=0, nrzeros=0;
    float average, stopimf;
    float stdev;
    float epsilon = setup_.noisepercentage_ / mCast(float,100.0);

    //defined and set but not used: TODO review Paul
    bool mUnusedVar enddecomp = false;
    ManagedObjectSet<ManagedObjectSet<IMFComponent> > realizations;
    OrgTraceMinusAverage* orgminusaverage =
	new OrgTraceMinusAverage( nrsamples );
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
	   PointBasedMathFunction::ExtraPolGradient);

    // remove average from input data and store in currentcomp
    computeStats( average, stdev );
    orgminusaverage->averageinput_ = average;
    orgminusaverage->name_ = "Input-average";
    orgminusaverage->stdev_ = stdev;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	imf->values_[idx] = values_[idx] - average;
	orgminusaverage->values_[idx] = imf->values_[idx];
	residual->values_[idx] = imf->values_[idx];
    }

    // Comment out setup_.method_ = 3;. Only used to test the software
    // post-decomposition.Components were created from the test trace
    // using CEEMD with default parameters in June 2014.

//    setup_.method_ = 3;

    if ( setup_.method_ == mDecompModeEMD )
    {
	ManagedObjectSet<IMFComponent>* components =
	    new ManagedObjectSet<IMFComponent>();
	enddecomp = imf->decompositionLoop(
	    *components, setup_.maxnrimf_, orgminusaverage->stdev_);
	realizations += components;
    }

    else if ( setup_.method_ == mDecompModeEEMD )
    {
	ManagedObjectSet<ManagedObjectSet<IMFComponent> > EEMDrealizations;
	for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	{
	    ManagedObjectSet<IMFComponent>* components =
		new ManagedObjectSet<IMFComponent>();
	    if ( nrnoise>=1 ) imf->resetInput( orgminusaverage );
	    noisearray->createNoise( stdev);
	    noisearray->rescaleDecompInput( epsilon );
	    imf->addDecompInputs( noisearray );
	    enddecomp = imf->decompositionLoop(
		*components, setup_.maxnrimf_, orgminusaverage->stdev_);
	    EEMDrealizations += components;
	}
	ManagedObjectSet<IMFComponent>* stackedcomponents =
	    new ManagedObjectSet<IMFComponent>();
	imf->stackEemdComponents( EEMDrealizations, *stackedcomponents );
	realizations += stackedcomponents;
	EEMDrealizations.erase();
    }
    else if ( setup_.method_ == mDecompModeCEEMD )
    {

	ManagedObjectSet<ManagedObjectSet<IMFComponent> > noisedecompositions;
	ManagedObjectSet<ManagedObjectSet<IMFComponent> > currentrealizations;
	ManagedObjectSet<IMFComponent> currentstackedcomponents;

	for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	{
	    ManagedObjectSet<IMFComponent>* noisecomponents =
	    new ManagedObjectSet<IMFComponent>();
	    noisearray->createNoise( stdev ); // w_i
	    copynoisearray->replaceDecompInputs( noisearray );
	    enddecomp = noisearray->decompositionLoop(
		*noisecomponents, setup_.maxnrimf_, orgminusaverage->stdev_ );
	    noisedecompositions += noisecomponents; // E_1[w_i],..,E_m[w_i]

	    ManagedObjectSet<IMFComponent>* components =
		    new ManagedObjectSet<IMFComponent>();
	    imf->resetInput( orgminusaverage );

	    copynoisearray->rescaleDecompInput( epsilon );
	    imf->addDecompInputs( copynoisearray );	// x+e*w_i
	    imf->decompositionLoop( *components, 1,
		orgminusaverage->stdev_);
	    currentrealizations += components;			// E_1[x+e*w_i]
	}

	residual->resetInput( orgminusaverage );	// r_0 = x
	ManagedObjectSet<IMFComponent>* stackedcomponents =
		new ManagedObjectSet<IMFComponent>();

	while ( true )
	{
	    imf->stackCeemdComponents( currentrealizations,
		    currentstackedcomponents, nrimf );
	    currentcomp->retrieveFromComponent(
		    currentstackedcomponents, 0 );	// IMF_(k+1)

	    nrimf++;					// k = k+1
	    residual->subtractDecompInputs( currentcomp );// r_k = r_(k-1)-IMF_k

// Move component IMF_k from temporary set to permanent set
// modify next statement in v5.0 to:
// (*stackedcomponents) += currentstackedcomponents.removeAndTake( 0 );
	    (*stackedcomponents) +=
			new IMFComponent(*currentstackedcomponents[0]);

	    residual->findExtrema( nrmax, nrmin,
		    nrzeros, setup_.symmetricboundary_ , maxima, minima );
	    residual->computeStats( average, stdev );
	    stopimf = stdev / orgminusaverage->stdev_;
	    if (( nrmin + nrmax + nrzeros < 1 )
	    || ( stopimf < setup_.stopimf_ )) // end of decomposition
	    {
		(*stackedcomponents) += new IMFComponent( nrsamples );
		residual->addToComponent(
		    *stackedcomponents, nrimf, nrzeros );
		break;
	    }
	    currentrealizations.erase();
	    currentstackedcomponents.erase();

	    for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	    {
		ManagedObjectSet<IMFComponent>* components =
			new ManagedObjectSet<IMFComponent>();

		if ( nrimf > noisedecompositions[nrnoise]->size()) continue;
		imf->retrieveFromComponent(
			*noisedecompositions[nrnoise], nrimf-1 ); // E_k[w_i]
		imf->rescaleDecompInput( epsilon );
		imf->addDecompInputs( residual );  // r_k+e*E_k[w_i]
		enddecomp = imf->decompositionLoop( *components, 1,
			orgminusaverage->stdev_ );
		currentrealizations += components;	// E_1[r_k+e*E_k[w_i]]
	    }
	}
	realizations += stackedcomponents;
	noisedecompositions.erase();
    }

    else // Start from pre-calculated components
    {
	readComponents( realizations );
    }
    if ( outputattrib < 3 ) // these outputs rely on instantaneous frequencies
    {
	ManagedObjectSet<IMFComponent>* imagcomponents =
				new ManagedObjectSet<IMFComponent>();
	doHilbert( realizations, *imagcomponents );
	realizations += imagcomponents;

	ManagedObjectSet<IMFComponent>* frequencycomponents =
		    new ManagedObjectSet<IMFComponent>();
	calcFrequencies( realizations, *imagcomponents,
		    *frequencycomponents, refstep );
	realizations += frequencycomponents;

	ManagedObjectSet<IMFComponent>* amplitudecomponents =
		    new ManagedObjectSet<IMFComponent>();
	calcAmplitudes( realizations, *imagcomponents,
		    *frequencycomponents, *amplitudecomponents );
	realizations += amplitudecomponents;
    }

/* Signal is decomposed and frequencies are computed where needed
   now create output */

    outputAttribute( realizations, output, outputattrib,
	startfreq, endfreq, stepoutfreq, startcomp, endcomp,
	orgminusaverage->averageinput_ );

// delete stuff
    delete imf;
    delete currentcomp;
    delete residual;
    delete noisearray;
    delete copynoisearray;
    delete orgminusaverage;
    realizations.erase();

//    dumpComponents( orgminusaverage, realizations );
    return true;
}
