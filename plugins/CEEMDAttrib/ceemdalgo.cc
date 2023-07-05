/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ceemdalgo.h"

#include "gridder2d.h"
#include "hilberttransform.h"
#include "statrand.h"
#include "statruncalc.h"
#include "survinfo.h"

#include "od_iostream.h"



IMFComponent::IMFComponent( int nrsamples )
    : values_(new float[nrsamples])
    , size_(nrsamples)
{
    OD::sysMemZero( values_, size_*sizeof(float) );
}


IMFComponent::IMFComponent( const IMFComponent& oth )
    : values_(nullptr)
{
    *this = oth;
}


IMFComponent::~IMFComponent()
{
    delete [] values_;
}


IMFComponent& IMFComponent::operator=( const IMFComponent& oth )
{
    if ( &oth == this )
	return *this;

    delete [] values_;
    size_ = oth.size_;
    values_ = new float[size_];
    OD::sysMemCopy( values_, oth.values_, size_*sizeof(float) );
    name_ = oth.name_;
    nrzeros_ = oth.nrzeros_;

    return *this;
}


OrgTraceMinusAverage::OrgTraceMinusAverage( int nrsamples )
    : values_(new float[nrsamples])
    , size_(nrsamples)
{
    OD::sysMemZero( values_, size_*sizeof(float) );
}


OrgTraceMinusAverage::OrgTraceMinusAverage( const OrgTraceMinusAverage& oth )
    : values_(nullptr)
{
    *this = oth;
}


OrgTraceMinusAverage::~OrgTraceMinusAverage()
{
    delete [] values_;
}


OrgTraceMinusAverage& OrgTraceMinusAverage::operator=
					( const OrgTraceMinusAverage& oth )
{
    if ( &oth == this )
	return *this;

    delete [] values_;
    size_ = oth.size_;
    values_ = new float[size_];
    OD::sysMemCopy( values_, oth.values_, size_*sizeof(float) );
    name_ = oth.name_;
    averageinput_ = oth.averageinput_;
    stdev_ = oth.stdev_;

    return *this;
}



DecompInput::DecompInput( const Setup& setup, int nrsamples,
			  Stats::RandomGenerator* gen )
    : values_(new float[nrsamples])
    , size_(nrsamples)
    , setup_(setup)
    , gen_(gen)
{
    OD::sysMemZero( values_, size_*sizeof(float) );
    if ( setup_.method_ != mDecompModeEMD && !gen_ )
	{ pErrMsg( "EEMD and CEEMD methods need a random number generator" ); }
}


DecompInput::DecompInput( const DecompInput& oth )
    : values_(nullptr)
{
    *this = oth;
}


DecompInput::~DecompInput()
{
    delete [] values_;
}


DecompInput& DecompInput::operator=( const DecompInput& oth )
{
    if ( &oth == this )
	return *this;

    delete [] values_;
    size_ = oth.size_;
    values_ = new float[size_];
    OD::sysMemCopy( values_, oth.values_, size_*sizeof(float) );
    setup_ = oth.setup_;
    halflen_ = oth.halflen_;
    gen_ = oth.gen_;

    return *this;
}


bool DecompInput::doDecompMethod( int nrsamples, float refstep,
			Array2D<float>& output, int outputattrib,
			float startfreq, float endfreq, float stepoutfreq,
			int startcomp, int endcomp )
{
    DecompInput imf( setup_, nrsamples, gen_ );
    int nrnoise=0, nrimf=0, nrmax=0, nrmin=0, nrzeros=0;
    float average, stopimf;
    float stdev;
    float epsilon = setup_.noisepercentage_ / mCast(float,100.0);

    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );

    // remove average from input data and store in orgminusaverage
    computeStats( average, stdev );
    OrgTraceMinusAverage orgminusaverage( nrsamples );
    orgminusaverage.averageinput_ = average;
    orgminusaverage.name_ = "Input-average";
    orgminusaverage.stdev_ = stdev;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	imf.values_[idx] = values_[idx] - average;
	orgminusaverage.values_[idx] = imf.values_[idx];
    }

    // Comment out setup_.method_ = 3;. Only used to test the software
    // post-decomposition.Components were created from the test trace
    // using CEEMD with default parameters in June 2014.
//    setup_.method_ = 3;

    //defined and set but not used: TODO review Paul
    bool mUnusedVar enddecomp = false;
    ManagedObjectSet<ObjectSet<IMFComponent> > realizations;
    if ( setup_.method_ == mDecompModeEMD )
    {
	ObjectSet<IMFComponent>* components =
				 new ManagedObjectSet<IMFComponent>;
	enddecomp = imf.decompositionLoop( *components, setup_.maxnrimf_,
					   orgminusaverage.stdev_ );
	realizations.add( components );
    }
    else if ( setup_.method_ == mDecompModeEEMD )
    {
	ManagedObjectSet<ObjectSet<IMFComponent> > EEMDrealizations;
	for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	{
	    if ( nrnoise>=1 )
		imf.resetInput( orgminusaverage );

	    DecompInput noisearray( setup_, nrsamples, gen_ );
	    noisearray.createNoise( stdev );
	    noisearray.rescaleDecompInput( epsilon );
	    imf.addDecompInputs( noisearray );

	    ObjectSet<IMFComponent>* components =
				     new ManagedObjectSet<IMFComponent>;
	    enddecomp = imf.decompositionLoop( *components, setup_.maxnrimf_,
					       orgminusaverage.stdev_ );
	    EEMDrealizations.add( components );
	}

	ObjectSet<IMFComponent>* stackedcomponents =
				 new ManagedObjectSet<IMFComponent>;
	imf.stackEemdComponents( EEMDrealizations, *stackedcomponents );
	realizations.add( stackedcomponents );
    }
    else if ( setup_.method_ == mDecompModeCEEMD )
    {
	ManagedObjectSet<ObjectSet<IMFComponent> > noisedecompositions;
	ManagedObjectSet<ObjectSet<IMFComponent> > currentrealizations;
	for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	{
	    DecompInput noisearray( setup_, nrsamples, gen_ );
	    noisearray.createNoise( stdev ); // w_i
	    DecompInput copynoisearray( setup_, nrsamples, gen_ );
	    copynoisearray.replaceDecompInputs( noisearray );

	    ObjectSet<IMFComponent>* noisecomponents =
				     new ManagedObjectSet<IMFComponent>;
	    enddecomp = noisearray.decompositionLoop( *noisecomponents,
				   setup_.maxnrimf_, orgminusaverage.stdev_ );
	    noisedecompositions.add( noisecomponents ); // E_1[w_i],..,E_m[w_i]

	    copynoisearray.rescaleDecompInput( epsilon );
	    imf.resetInput( orgminusaverage );
	    imf.addDecompInputs( copynoisearray );	// x+e*w_i
	    ObjectSet<IMFComponent>* components =
				     new ManagedObjectSet<IMFComponent>;
	    imf.decompositionLoop( *components, 1, orgminusaverage.stdev_ );
	    currentrealizations.add( components );	// E_1[x+e*w_i]
	}

	DecompInput residual( setup_, nrsamples, gen_ );
	residual.resetInput( orgminusaverage );	// r_0 = x
	ManagedObjectSet<IMFComponent> currentstackedcomponents;
	PtrMan<ObjectSet<IMFComponent> > stackedcomponents =
				 new ManagedObjectSet<IMFComponent>;
	while ( true )
	{
	    imf.stackCeemdComponents( currentrealizations,
				      currentstackedcomponents, nrimf );
	    DecompInput currentcomp( setup_, nrsamples, gen_ );
	    currentcomp.retrieveFromComponent( currentstackedcomponents, 0 );
	    // IMF_(k+1)

	    nrimf++;					// k = k+1
	    residual.subtractDecompInputs( currentcomp );// r_k = r_(k-1)-IMF_k

	    stackedcomponents->add( currentstackedcomponents.removeAndTake(0) );

	    residual.findExtrema( nrmax, nrmin, nrzeros,
				  setup_.symmetricboundary_, maxima, minima );
	    residual.computeStats( average, stdev );
	    stopimf = stdev / orgminusaverage.stdev_;
	    if ( nrmin + nrmax + nrzeros < 1 || stopimf < setup_.stopimf_ ||
		 nrimf >= setup_.maxnoiseloop_ )
	    {
		// end of decomposition
		stackedcomponents->add( new IMFComponent( nrsamples ) );
		residual.addToComponent( *stackedcomponents, nrimf, nrzeros );
		break;
	    }

	    currentrealizations.setEmpty();
	    currentstackedcomponents.setEmpty();

	    for ( nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	    {
		if ( nrimf > noisedecompositions[nrnoise]->size() )
		    continue;

		imf.retrieveFromComponent(
			*noisedecompositions[nrnoise], nrimf-1 ); // E_k[w_i]
		imf.rescaleDecompInput( epsilon );
		imf.addDecompInputs( residual );  // r_k+e*E_k[w_i]

		ObjectSet<IMFComponent>* components =
					 new ManagedObjectSet<IMFComponent>;
		enddecomp = imf.decompositionLoop( *components, 1,
						   orgminusaverage.stdev_ );
		currentrealizations.add( components );	// E_1[r_k+e*E_k[w_i]]
	    }
	}

	realizations.add( stackedcomponents.release() );
    }
    else // Start from pre-calculated components
	readComponents( realizations );

    if ( outputattrib < 3 ) // these outputs rely on instantaneous frequencies
    {
	ObjectSet<IMFComponent>* imagcomponents =
				 new ManagedObjectSet<IMFComponent>;
	doHilbert( realizations, *imagcomponents );
	realizations.add( imagcomponents );

	ObjectSet<IMFComponent>* frequencycomponents =
				 new ManagedObjectSet<IMFComponent>();
	calcFrequencies( realizations, *imagcomponents, *frequencycomponents,
			 refstep );
	realizations.add( frequencycomponents );

	ObjectSet<IMFComponent>* amplitudecomponents =
				 new ManagedObjectSet<IMFComponent>;
	calcAmplitudes( realizations, *imagcomponents, *amplitudecomponents );
	realizations.add( amplitudecomponents );
    }

/* Signal is decomposed and frequencies are computed where needed
   now create output */

    outputAttribute( realizations, output, outputattrib,
		     startfreq, endfreq, stepoutfreq, startcomp, endcomp,
		     orgminusaverage.averageinput_ );

//    dumpComponents( realizations, nullptr );
//    dumpComponents( realizations, &orgminusaverage );
    return true;
}


bool DecompInput::decompositionLoop( ObjectSet<IMFComponent>& components,
				     int maxnrimf, float stdevinput ) const
{
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );

    int nrimf=0, nriter=0;
    bool isresidual = false;
    int nrmax, nrmin, nrzeros;
    float mean=0, average, stdev;
    float dosift, stopimf;

    IMFComponent residual( size_ );
    OD::sysMemCopy( residual.values_, values_, size_*sizeof(float) );
    computeStats( average, stdev );
    stopimf = stdev / stdevinput;
    dosift = 1.0;

    while ( nrimf < maxnrimf )
    {
	if ( isresidual ) // fill up with zeroes
	{
	    addZeroComponents( components, nrimf );
	    break;
	}

	PtrMan<IMFComponent> currentcomp = new IMFComponent( size_ );
	currentcomp->name_.set( "Component " ).add( nrimf+1 );
	nriter=0;
	stopimf = stdev / stdevinput;
	dosift = 1.0;
	isresidual = false;

	while ( nriter < setup_.maxsift_ )
	{
	    findExtrema( nrmax, nrmin, nrzeros, setup_.symmetricboundary_,
			 maxima, minima );
	    computeStats( average, stdev );
	    stopimf = stdev / stdevinput;
	    if ( nrmin + nrmax + nrzeros < 1 || stopimf < setup_.stopimf_ )
	    {
		// end of decomposition
		OD::sysMemCopy( currentcomp->values_, residual.values_,
				size_*sizeof(float) );
		currentcomp->nrzeros_ = nrzeros;
		components.add( currentcomp.release() );
		isresidual = true;
		break;
	    }
	    else if ( (abs(nrmax+nrmin-nrzeros) <= 1 ||
		       dosift < setup_.stopsift_ ) ||
		       nriter == setup_.maxsift_ -1 )
	    {
		// Check if data is a true IMF
		for ( int idx=0; idx<size_; idx++ )
		{
		    currentcomp->values_[idx] = values_[idx];
		    residual.values_[idx] -= values_[idx];
		    values_[idx] = residual.values_[idx];
		}

		currentcomp->nrzeros_ = nrzeros;
		stopimf = stdev / stdevinput;
		dosift = 1.0;
		components.add( currentcomp.release() );
		break;
	    }
	    else
	    {
		// continue sifting
		for ( int idx=0; idx<size_; idx++ )
		{
		    const float min = minima.getValue(mCast(float,idx));
		    const float max = maxima.getValue(mCast(float,idx));
		    mean = ( (max+min)/2 );
		    const float val = values_[idx];
		    dosift = (mean*mean) / (val*val);
		    values_[idx] = values_[idx] - mean;
		}
	    }
	    nriter++;
	}

	nrimf++;
    }

    return isresidual;
}


void DecompInput::stackCeemdComponents(
	    const ObjectSet<ObjectSet<IMFComponent> >& currentrealizations,
	    ObjectSet<IMFComponent>& currentstackedcomponents, int nrimf ) const
{
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );

    DecompInput imf( setup_, size_, gen_ );
    int stackcount = setup_.maxnoiseloop_;
    int nrmax, nrmin, nrzeros;

    currentstackedcomponents.add( new IMFComponent(size_) );
    IMFComponent& currentstackedcomponent = *currentstackedcomponents.first();
    //Really? not the newly added=last() ???
    currentstackedcomponent.name_.set( "Component " ).add( nrimf+1 );

    for ( int idx=0; idx<size_; idx++)
    {
	if ( currentrealizations.isEmpty() ||
	     currentrealizations.first()->isEmpty() )
	    continue;

	const float val = currentrealizations.first()->first()->values_[idx];
	if ( mIsUdf(val) )
	    continue;

	currentstackedcomponent.values_[idx] = val / stackcount;
    }

    for ( int nrnoise=1; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
    {
	for ( int idx=0; idx<size_; idx++)
	{
	    if ( !currentrealizations.validIdx(nrnoise) ||
		 currentrealizations.get(nrnoise)->isEmpty() )
		continue;

	    const float val =
			currentrealizations.get(nrnoise)->first()->values_[idx];
	    if ( mIsUdf(val) )
		continue;

	    currentstackedcomponent.values_[idx] += val / stackcount;
	}
    }

    for ( int idx=0; idx<size_; idx++ )
	imf.values_[idx] = currentstackedcomponent.values_[idx] ;

    imf.findExtrema( nrmax, nrmin, nrzeros, setup_.symmetricboundary_,
		     maxima, minima );

    currentstackedcomponent.nrzeros_ = nrzeros;
}


void DecompInput::stackEemdComponents(
		    const ObjectSet<ObjectSet<IMFComponent> >& realizations,
		    ObjectSet<IMFComponent>& stackedcomponents ) const
{
    MyPointBasedMathFunction minima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );
    MyPointBasedMathFunction maxima( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );

    DecompInput imf( setup_, size_, gen_ );
    int stackcount = setup_.maxnoiseloop_;
    int maxcomp = 0;
    int nrmax, nrmin, nrzeros;

    for ( int comp=0; comp<setup_.maxnrimf_; comp++ )
    {
	maxcomp = comp;
	int checkzeros = 0;
	for ( int nrnoise=0; nrnoise<stackcount; nrnoise++)
	{
	    if ( realizations.get(nrnoise)->get(comp)->nrzeros_ == -1 )
		checkzeros++;
	}

	if ( checkzeros >= setup_.maxnoiseloop_ )
	    break;
    }

    for ( int comp=0; comp<maxcomp; comp++)
    {
	stackedcomponents.add( new IMFComponent( size_ ) );
	IMFComponent& stackedcomponent = *stackedcomponents.last();
	stackedcomponent.name_.set( "Component " ).add( comp+1 );
	for ( int nrnoise=0; nrnoise<setup_.maxnoiseloop_; nrnoise++ )
	{
	    const ObjectSet<IMFComponent>& nrealizations =
						*realizations.get( nrnoise );
	    for ( int idx=0; idx<size_; idx++ )
	    {
		const IMFComponent& realization = *nrealizations.get(comp);
		if ( realization.nrzeros_ == -1 )
		    continue;

		stackedcomponent.values_[idx] +=
		     realization.values_[idx] / stackcount;
	    }
	}

	for ( int idx=0; idx<size_; idx++ )
	    imf.values_[idx] = stackedcomponent.values_[idx];

	imf.findExtrema( nrmax, nrmin, nrzeros, setup_.symmetricboundary_ ,
			 maxima, minima );

	stackedcomponent.nrzeros_ = nrzeros;
    }
}


void DecompInput::resetInput( const OrgTraceMinusAverage& orgminusaverage )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = orgminusaverage.values_[idx];
}


void DecompInput::addDecompInputs( const DecompInput& arraytoadd )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] += arraytoadd.values_[idx];
}


void DecompInput::rescaleDecompInput( float scaler )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = values_[idx] * scaler;
}


void DecompInput::subtractDecompInputs( const DecompInput& arraytosubtract )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] -= arraytosubtract.values_[idx];
}


void DecompInput::replaceDecompInputs( const DecompInput& replacement )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = replacement.values_[idx];
}


void DecompInput::createNoise( float stdev )
{
    for ( int idx=0; idx<size_; idx++ )
	values_[idx] = stdev * gen_->get();
}


void DecompInput::computeStats( float& average, float& stdev ) const
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
}


void DecompInput::findExtrema( int& nrmax, int& nrmin, int& nrzeros,
		bool symmetricboundary, MyPointBasedMathFunction& maxima,
		MyPointBasedMathFunction& minima ) const
{
    nrmax=0; nrmin=0; nrzeros=0;
    maxima.setEmpty();
    minima.setEmpty();
    minima.add( 0.0, 0.0 ); // to be replaced by boundary point
    minima.add( 1.0, 0.0 ); // to be replaced by boundary point
    maxima.add( 0.0, 0.0 ); // to be replaced by boundary point
    maxima.add( 1.0, 0.0 ); // to be replaced by boundary point
    for ( int idx=0; idx<size_-3; idx++ )
    {
	float val = values_[idx];
	float val1 = values_[idx+1];
	float val2 = values_[idx+2];
	float val3 = values_[idx+3];
	if ( val1>val && val1>val2 )
	{
	    maxima.add( float(idx+2) , val1 );
	    nrmax = nrmax+1;
	}

	if ( val1>val && val1==val2 && val2>val3 )
	{
	    maxima.add( float(idx+2) , val1 );
	    nrmax = nrmax+1;
	}

	if ( val1<val && val1<val2 )
	{
	    minima.add( float(idx+2) , val1 );
	    nrmin = nrmin+1;
	}

	if ( val1<val && val1==val2 && val2<val3 )
	{
	    minima.add( float(idx+2) , val1 );
	    nrmin = nrmin+1;
	}

	if ( (val<0 && val1>0)	|| (val>0 && val1<0) )
	    nrzeros=nrzeros+1;
    }

    // Extend boundaries
    if ( nrmin <= 1 || nrmax <= 1 )
	return;

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


void DecompInput::addZeroComponents( ObjectSet<IMFComponent>& components,
				     int comp ) const
{
    for ( int nrimf=comp; nrimf<setup_.maxnrimf_; nrimf++ )
    {
	auto* zerocomp = new IMFComponent( size_ );
	zerocomp->name_ = "Zeroes";
	zerocomp->nrzeros_ = -1; // can be used to recognize zero component
	components.add( zerocomp );
    }
}


void DecompInput::addToComponent( ObjectSet<IMFComponent>& components,
				  int comp, int nrzeros ) const
{
    for ( int idx=0; idx<size_; idx++ )
	components.get(comp)->values_[idx] = values_[idx];

    components[comp]->name_.set( "Component " ).add( comp+1 );
    components[comp]->nrzeros_ = nrzeros;
}


void DecompInput::retrieveFromComponent(
			const ObjectSet<IMFComponent>& components, int comp )
{
     for ( int idx=0; idx<size_; idx++ )
	 values_[idx] = components.get(comp)->values_[idx];
}


bool DecompInput::doHilbert(
		const ObjectSet<ObjectSet<IMFComponent> >& realcomponents,
		ObjectSet<IMFComponent>& imagcomponents ) const
{
    if ( realcomponents.isEmpty() )
	return false;

    const ObjectSet<IMFComponent>& firstrealcomponents= *realcomponents.first();
    const int hilbfilterlen = halflen_*2 + 1;
    const bool enoughsamps = size_ >= hilbfilterlen;
    const int arrminnrsamp = hilbfilterlen ? size_ : hilbfilterlen;
    const int shift = 0;
    int inpstartidx = 0;
    int startidx = enoughsamps ? shift : 0;
    int comp = 0; // first component
    Array1DImpl<float> createarr( arrminnrsamp );
    ValueSeries<float>* padtrace = nullptr;
    if ( !enoughsamps )
    {
	for ( int idx=0; idx<arrminnrsamp; idx++ )
	{
	    const float val = idx < size_ ?
			      firstrealcomponents.get(comp)->values_[idx] : 0.f;
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

    for ( const auto* realcomponent : firstrealcomponents )
    {
	for ( int idx=0; idx<size_; idx++ )
	    inparr.set( shift+idx, realcomponent->values_[idx] );

	const bool transformok = enoughsamps
		    ? ht.transform( inparr, size_, outarr, size_ )
		    : ht.transform( *padtrace, arrminnrsamp, outarr, size_ );
	if ( !transformok )
	    return false;

	auto* imagcomp = new IMFComponent( size_ );
	for ( int idx=0; idx<size_; idx++ )
	    imagcomp->values_[idx] = outarr.get( shift+idx );

	imagcomponents.add( imagcomp );
    }

    return true;
}

#define mCheckRetUdf(val1,val2) \
    if ( mIsUdf(val1) || mIsUdf(val2) ) continue;

bool DecompInput::calcFrequencies(
		    const ObjectSet<ObjectSet<IMFComponent> >& realcomponents,
		    const ObjectSet<IMFComponent>& imagcomponents,
		    ObjectSet<IMFComponent>& frequencycomponents,
		    const float refstep ) const
{
    if ( realcomponents.isEmpty() )
	return false;

    const ObjectSet<IMFComponent>& firstrealcomponents= *realcomponents.first();
    for ( int comp=0; comp<firstrealcomponents.size(); comp++ )
    {
	auto* freqcomp = new IMFComponent( size_ );

	const float* realvals = firstrealcomponents.get( comp )->values_;
	const float* imagvals = imagcomponents.get( comp )->values_;
	float* freqvals = freqcomp->values_;
	for ( int idx=1; idx<size_-1; idx++ )
	{
	    const float realval = realvals[idx];
	    const float prevreal = realvals[idx-1];
	    const float nextreal = realvals[idx+1];
	    mCheckRetUdf( prevreal, nextreal );
	    const float dreal_dt = (nextreal - prevreal) / (2*refstep);

	    const float imagval = imagvals[idx];
	    const float previmag = imagvals[idx-1];
	    const float nextimag = imagvals[idx+1];
	    mCheckRetUdf( previmag, nextimag );
	    const float dimag_dt = (nextimag-previmag) / (2*refstep);

	    float denom = (realval*realval + imagval*imagval) * M_2PIf;
	    if ( mIsZero(denom,1e-6f) )
		denom = 1e-6f;

	    freqvals[idx] = ( realval*dimag_dt-dreal_dt*imagval ) / denom;
	}

	if ( size_ > 2 )
	{
	    // copy boundary points to keep number of samples = size_
	    freqvals[0] = freqvals[1];
	    freqvals[size_-1] = freqvals[size_-2];
	}

	frequencycomponents.add( freqcomp );
    }

    return true;
}


bool DecompInput::calcAmplitudes(
		    const ObjectSet<ObjectSet<IMFComponent> >& realcomponents,
		    const ObjectSet<IMFComponent>& imagcomponents,
		    ObjectSet<IMFComponent>& amplitudecomponents ) const
{
    if ( realcomponents.isEmpty() )
	return false;

    const ObjectSet<IMFComponent>& firstrealcomponents= *realcomponents.first();
    const int size = firstrealcomponents.size();
    for ( int comp=0; comp<size; comp++)
    {
	auto* amplitudecomp = new IMFComponent( size_ );

	const float* realvals = firstrealcomponents.get( comp )->values_;
	const float* imagvals = imagcomponents.get( comp )->values_;
	float* ampvals = amplitudecomp->values_;
	for ( int idx=0; idx<size_; idx++ )
	{
	    const float realval = realvals[idx];
	    const float imagval = imagvals[idx];
	    ampvals[idx] = Math::Sqrt( realval*realval + imagval*imagval );
	}

	amplitudecomponents.add( amplitudecomp );
    }

    return true;
}


bool DecompInput::outputAttribute(
			const ObjectSet<ObjectSet<IMFComponent> >& realizations,
			Array2D<float>& output, int outputattrib,
			float startfreq, float endfreq, float stepoutfreq,
			int startcomp, int endcomp, float average ) const
{
    if ( outputattrib == mDecompOutputFreq )
    {
	const bool gridding = false; // 3-row gridding or 1-row polynomial
	if ( gridding )
	    useGridding( realizations, output, startfreq, endfreq, stepoutfreq);
	else
	    usePolynomial( realizations, output, startfreq,endfreq,stepoutfreq);
    }
    else if ( outputattrib == mDecompOutputPeakFreq )
    {
	if ( !realizations.validIdx(2) ) // What is 2 ?
	    return false;

	const ObjectSet<IMFComponent>& realization = *realizations.get( 2 );
	const int size = realization.size();
	for ( int idx=0; idx<size_; idx++ )
	{
	    float peakfreq = 0.f;
	    for ( int comp=0; comp<size; comp++ )
	    {
		const float val = realization.get(comp)->values_[idx];
		if ( val > peakfreq )
		    peakfreq = val;
	    }

	    output.set( idx, 0, peakfreq );
	}
    }
    else if ( outputattrib == mDecompOutputPeakAmp )
    {
	if ( !realizations.validIdx(2) || !realizations.validIdx(3) )
	    return false;		// What are 2, 3 ?

	const ObjectSet<IMFComponent>& realization = *realizations.get( 2 );
	const ObjectSet<IMFComponent>& realization3 = *realizations.get( 3 );
	const int size = realization.size();
	for ( int idx=0; idx<size_; idx++ )
	{
	    float peakfreq = 0.f;
	    int cindex = 0;
	    for ( int comp=0; comp<size; comp++ )
	    {
		const float val = realization.get(comp)->values_[idx];
		if ( val > peakfreq )
		{
		    peakfreq = val;
		    cindex = comp;
		}
	    }

	    const float val = realization3.get(cindex)->values_[idx];
	    output.set( idx, 0, val );
	}
    }
    else if ( outputattrib == mDecompOutputIMF )
    {
	if ( !realizations.validIdx(0) )
	    return false;

	const ObjectSet<IMFComponent>& realization = *realizations.get( 0 );
	const int size = realization.size();
	for ( int comp=startcomp; comp<=endcomp && comp<size; comp++ )
	{
	    if ( comp>=output.info().getSize(1) )
		break;

	    for ( int idx=0; idx<size_; idx++ )
	    {
		const float val = realization.get(comp)->values_[idx];
		output.set( idx, comp, val );
	    }
	}
	// add zeroes
	for ( int comp=size; comp<setup_.maxnrimf_; comp++ )
	{
	    if ( comp>=output.info().getSize(1) )
		break;

	    for ( int idx=0; idx<size_; idx++ )
		output.set( idx, comp, 0.f );
	}
	// add average of input trace (DC) in last component
	for ( int idx=0; idx<size_; idx++ )
	    output.set( idx, setup_.maxnrimf_, average );
    }
    else
	return false;

    return true;
}


bool DecompInput::useGridding(
			const ObjectSet<ObjectSet<IMFComponent> >& realizations,
			Array2D<float>& output, float startfreq, float endfreq,
			float stepoutfreq ) const
{
    if ( realizations.size() < 4 )
	return false;

    const int fsize = realizations.first()->size();
    double step = mCast(double, stepoutfreq);
    if ( step > 1. )
	step = 1.;

    TriangulatedGridder2D grdr;
    TypeSet<Coord> pts;
    TypeSet<float> zvals;
    const ObjectSet<IMFComponent>& realization2 = *realizations.get(2);
    const ObjectSet<IMFComponent>& realization3 = *realizations.get(3);
    for ( int idt=0; idt<size_; idt++ )
    {
	double maxf = 0;
	double minf = 0;
	for ( int f=0; f<fsize; f++ )
	{
	    const float* f2vals = realization2.get(f)->values_;
	    const float* f3vals = realization3.get(f)->values_;
	    for ( int t=-1; t<2; t++ ) // grid over three traces
	    {
		const double dff = mCast(double,f2vals[idt+t]);
		const double dtt = mCast(double, idt+t);
		const float z = f3vals[idt+t];
		maxf = mMAX( maxf, dff );
		minf = mMIN( minf, dff );
		pts.add(Coord(dff,dtt));
		zvals.add(z);
	    }
	}
	for ( int t=-1; t<2; t++ ) // add zeros at either end
	{
	    const double dtt = mCast(double, idt+t);
	    pts.add(Coord( minf-step, dtt));
	    zvals.add(0);
	    pts.add(Coord( maxf+step, dtt));
	    zvals.add(0);
	}
	grdr.setPoints( pts );
	grdr.setValues( zvals );

	for ( float f=startfreq; f<endfreq; f+=stepoutfreq )
	{
	    if ( f >= output.info().getSize(1) )
		break;

	    double dff = f;
	    double dtt = idt;
	    float val;
	    if ( f < minf || f > maxf )
		val = 0.f;
	    else
	    {
		val = grdr.getValue( Coord(dff,dtt) );
		if ( mIsUdf(val) )
		    val = 0.f;
	    }
	    output.set( idt, mNINT32(f/stepoutfreq), val );
	}

	pts.setEmpty();
	zvals.setEmpty();
    }
    // copy boundary points to keep number of samples = size_

    for ( float f=startfreq; f<endfreq; f+=stepoutfreq )
    {
	const int yidx = mNINT32(f/stepoutfreq);
	const float valzero = output.get( 1, yidx ) ;
	const float vallast = output.get( size_-2, yidx ) ;
	output.set( 0, yidx, valzero );
	output.set( size_-1, yidx, vallast );
    }

    return true;
}


bool DecompInput::usePolynomial(
		    const ObjectSet<ObjectSet<IMFComponent> >& realizations,
		    Array2D<float>& output, float startfreq, float endfreq,
		    float stepoutfreq ) const
{
    if ( realizations.size() < 4 )
	return false;

    const int fsize = realizations.first()->size();
    float* unsortedamplitudes = new float[fsize+2];
    float* unsortedfrequencies = new float[fsize+2];
    MyPointBasedMathFunction sortedampspectrum(
				PointBasedMathFunction::Poly,
				PointBasedMathFunction::ExtraPolGradient );
    const int convfac = SI().zIsTime() ? 1 : 1000;

    const float stepout = stepoutfreq*convfac;
    const ObjectSet<IMFComponent>& realization2 = *realizations.get(2);
    const ObjectSet<IMFComponent>& realization3 = *realizations.get(3);
    for ( int idt=0; idt<size_; idt++ )
    {
	float maxf = 0.f;
	float minf = 0.f;
	for ( int f=0; f<fsize; f++ )
	{
	    unsortedfrequencies[f] = realization2.get(f)->values_[idt];
	    unsortedamplitudes[f] = realization3.get(f)->values_[idt];
	    maxf = mMAX( maxf, unsortedfrequencies[f] );
	    minf = mMIN( minf, unsortedfrequencies[f] );
	}
	// add zeros either end
	unsortedfrequencies[fsize] = minf-mCast(float, stepoutfreq);
	unsortedamplitudes[fsize] = 0.f;
	unsortedfrequencies[fsize+1] = maxf+mCast(float, stepoutfreq);
	unsortedamplitudes[fsize+1] = 0.f;

	sortSpectrum( unsortedamplitudes, fsize+2, unsortedfrequencies,
		      sortedampspectrum );

	for ( float f=startfreq*convfac; f<=endfreq*convfac; f+=stepout )
	{
	    const int yidx = mNINT32(f/stepout);
	    if ( yidx == 0 )
		continue;

	    if ( yidx > output.info().getSize(1) )
		break;

	    float val;
	    if ( f < (minf*convfac) || f > (maxf*convfac) )
		val = 0.f;
	    else
	    {
		val = sortedampspectrum.getValue(f/convfac) ;
		if ( mIsUdf(val) || val<0 )
		    val = 0.f;
	    }

	    output.set( idt, yidx-1, val );
	}
    }

    // copy boundary points to keep number of samples = size_
    for ( float f=startfreq*convfac; f<endfreq*convfac; f+=stepoutfreq*convfac )
    {
	const int yidx = mNINT32(f/(stepoutfreq*convfac));
	if ( yidx == 0 )
	    continue;

	const float valzero = output.get( 1, yidx-1 ) ;
	const float vallast = output.get( size_-2, yidx-1 ) ;
	output.set( 0, yidx-1, valzero );
	output.set( size_-1, yidx-1, vallast );
    }

    delete [] unsortedfrequencies;
    delete [] unsortedamplitudes;
    return true;
}

bool DecompInput::sortSpectrum( const float* unsortedamplitudes, int size,
				float* unsortedfrequencies,
			    MyPointBasedMathFunction& sortedampspectrum ) const
{
    int* indexsortedamplitudes = new int[size];
    sortedampspectrum.setEmpty();

    for ( int i=0; i<size; i++ )
	indexsortedamplitudes[i] = i;

    sort_coupled( unsortedfrequencies, indexsortedamplitudes, size );

    for ( int i=0; i<size; i++ )
    {
	const int index = indexsortedamplitudes[i];
	const float freq = unsortedfrequencies[i];
	const float amp = unsortedamplitudes[index];
	sortedampspectrum.add( freq, amp );
    }

    delete [] indexsortedamplitudes;
    return true;
}



void DecompInput::testFunction( int& nrmax, int& nrmin, int& nrzeros,
				MyPointBasedMathFunction& maxima,
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
			const ObjectSet<ObjectSet<IMFComponent> >& realizations,
			const OrgTraceMinusAverage* orgminusaverage ) const
{
    // write output to file
    od_ostream strm( "components.txt" );
    if ( orgminusaverage )
    {
	strm << "input-average" << "/t";
	strm << orgminusaverage->averageinput_ << "/t";
	for ( int idx=0; idx<size_; idx++, strm.addTab() )
	    strm << orgminusaverage->values_[idx];
    }

    for ( const auto* realization : realizations )
    {
	for ( const auto* currentcomp : *realization )
	{
	    //strm << "\n" <<  currentcomp->name_<< "\t";
	    //strm << currentcomp->nrzeros_ << "\t";
	    for ( int idx=0; idx<size_; idx++, strm.addTab() )
		strm << currentcomp->values_[idx];
	    strm.addNewLine();
	}
    }

    return true;
}


void DecompInput::readComponents(
		    ObjectSet<ObjectSet<IMFComponent> >& realizations ) const
{
    const int nrsamples = 2001;
    const int nrcomp = 11;
    od_istream strm( "TestData_Components_1-11_MS-DOS.txt" );
    auto* components = new ManagedObjectSet<IMFComponent>;

    for ( int i=0; i<nrcomp; i++)
    {
	auto* currentcomp = new IMFComponent( nrsamples );
	strm.getBin( currentcomp->values_, currentcomp->size_*sizeof(float) );
	components->add( currentcomp );
    }

    realizations.add( components );
}
