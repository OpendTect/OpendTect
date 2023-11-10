/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackanglemute.h"

#include "arrayndslice.h"
#include "ioman.h"
#include "muter.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "raytracerrunner.h"
#include "unitofmeasure.h"


namespace PreStack
{

// PreStack::AngleCompParams

AngleCompParams::AngleCompParams()
    : anglerange_(0,30)
{
    smoothingpar_.set( AngleComputer::sKeySmoothType(),
		       AngleComputer::FFTFilter );
    smoothingpar_.set( AngleComputer::sKeyFreqF3(), 10.0f );
    smoothingpar_.set( AngleComputer::sKeyFreqF4(), 15.0f );
    // defaults for other types:
    smoothingpar_.set( AngleComputer::sKeyWinFunc(), HanningWindow::sName() );
    smoothingpar_.set( AngleComputer::sKeyWinLen(),
		       100.0f / SI().zDomain().userFactor() );
    smoothingpar_.set( AngleComputer::sKeyWinParam(), 0.95f );

    const float srd = getConvertedValue( SI().seismicReferenceDatum(),
				UnitOfMeasure::surveyDefSRDStorageUnit(),
				UnitOfMeasure::surveyDefDepthUnit() );
    const Seis::OffsetType offstyp = SI().xyInFeet()
				   ? Seis::OffsetType::OffsetFeet
				   : Seis::OffsetType::OffsetMeter;
    const ZDomain::DepthType depthtype = SI().depthsInFeet()
				       ? ZDomain::DepthType::Feet
				       : ZDomain::DepthType::Meter;
    rtsu_.offsettype( offstyp ).depthtype( depthtype ).startdepth( -srd );

    const StepInterval<float> offsrange =
			      RayTracer1D::sDefOffsetRange( offstyp );
    TypeSet<float> offsets;
    for ( int idx=0; idx<=offsrange.nrSteps(); idx++ )
	offsets += offsrange.atIndex( idx );

    raypar_.set( sKey::Type(), RayTracer1D::factory().getDefaultName() );
    raypar_.set( RayTracer1D::sKeyOffset(), offsets );
    raypar_.setYN( RayTracer1D::sKeyOffsetInFeet(),
		   offstyp == Seis::OffsetType::OffsetFeet );
    raypar_.setYN( RayTracer1D::sKeyReflectivity(), false );

    uiString msg;
    PtrMan<RayTracer1D> rt1d = RayTracer1D::createInstance( raypar_, msg );
    if ( !rt1d )
	return;

    rt1d->fillPar( raypar_ );
}


AngleCompParams::~AngleCompParams()
{
}


// PreStack::AngleMute::AngleMutePars

PreStack::AngleMute::AngleMutePars::AngleMutePars()
{
}


PreStack::AngleMute::AngleMutePars::~AngleMutePars()
{
}


// PreStack::AngleMute

AngleMuteBase::AngleMuteBase()
{
    velsource_ = new Vel::VolumeFunctionSource();
}


AngleMuteBase::~AngleMuteBase()
{
    delete params_;
    deepErase( rtrunners_ );
}


void AngleMuteBase::fillPar( IOPar& par ) const
{
    PtrMan<IOObj> ioobj = IOM().get( params_->velvolmid_ );
    if ( ioobj ) par.set( sKeyVelVolumeID(), params_->velvolmid_ );

    par.merge( params_->raypar_ );
    par.set( sKeyMuteCutoff(), params_->mutecutoff_ );
}


bool AngleMuteBase::usePar( const IOPar& par  )
{
    params_->raypar_.merge( par );
    par.get( sKeyVelVolumeID(), params_->velvolmid_ );
    par.get( sKeyMuteCutoff(), params_->mutecutoff_ );

    return true;
}


bool AngleMuteBase::setVelocityFunction()
{
    const MultiID& mid = params_->velvolmid_;
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return false;

    velsource_->setFrom( mid );
    return true;
}


void AngleMuteBase::block( ElasticModel& emodel ) const
{
    if ( emodel.isEmpty() || !params_ || mIsUdf(params_->maxthickness_) )
	return;

    emodel.setMaxThickness( params_->maxthickness_ );
}


bool AngleMuteBase::getLayers( const TrcKey& tk, ElasticModel& model,
			       uiString& errmsg )
{
    const float startdepth = params_->rtsu_.startdepth_;
    if ( !velsource_ ||
	 !VelocityBasedAngleComputer::getLayers(tk,startdepth,
						*velsource_,model,errmsg) )
	return false;

    block( model );
    return !model.isEmpty();
}


float AngleMuteBase::getOffsetMuteLayer( const ReflectivityModelBase& refmodel,
					 int ioff, bool innermute,
					 bool& nonemuted, bool& allmuted,
					 TypeSet< Interval<float> >& ret ) const
{
    const bool belowcutoff = innermute;
    const float cutoffsin = (float) sin( params_->mutecutoff_ * M_PIf / 180.f );

    const int nrlayers = refmodel.nrLayers();
    BoolTypeSet ismuted( nrlayers, false );
    int nrmuted = 0;
    for ( int ilay=0; ilay<nrlayers; ilay++ )
    {
	const float sini = refmodel.getSinAngle( ioff, ilay );
	if ( mIsUdf(sini) || (mIsZero(sini,1e-8) && ilay<nrlayers/2) )
	    continue; //Ordered down, get rid of edge 0.

	const bool muted = belowcutoff ? sini < cutoffsin && belowcutoff
				       : sini > cutoffsin;
	ismuted[ilay] = muted;
	if ( muted )
	    nrmuted++;
    }

    nonemuted = nrmuted == 0;
    allmuted = nrmuted == nrlayers;
    if ( nonemuted || allmuted )
	return mUdf(float);

    TypeSet<int> mutedlayers;
    for ( int ilay=0; ilay<nrlayers; ilay++ )
    {
	if ( ismuted[ilay] )
	    mutedlayers += ilay;
    }

    TypeSet<int> addback;
    for ( int ilay=0; ilay<nrlayers; ilay++ )
    {
	const bool mutedbefore = ilay == 0 ? false : (bool)ismuted[ilay-1];
	const bool mutedafter = ilay == nrlayers-1 ? false
						   : (bool)ismuted[ilay+1];
	if ( ismuted[ilay] && !mutedbefore && !mutedafter )
	    addback += ilay;
    }

    BoolTypeSet removeidxs( mutedlayers.size(), false );
    for ( int idx=mutedlayers.size()-2; idx>0; idx-- )
    {
	if ( mutedlayers[idx] == mutedlayers[idx+1]-1 &&
	     mutedlayers[idx] == mutedlayers[idx-1]+1 )
	    removeidxs[idx] = true;
    }

    for ( int idx=mutedlayers.size()-2; idx>0; idx-- )
	if ( removeidxs[idx] )
	    mutedlayers.removeSingle( idx );

    if ( !addback.isEmpty() )
    {
	mutedlayers.append( addback );
	sort( mutedlayers );
    }

    ret.setEmpty();
    for ( int idx=0; idx<mutedlayers.size(); idx+=2 )
    {
	Interval<float> mutelayer = Interval<float>::udf();
	int ilay = mutedlayers[idx];
	mutelayer.start = float(ilay);
	const int previdx = ilay-1;
	if ( ilay > 0 && ilay < nrlayers && previdx >=0 )
	{
	    const float sini = refmodel.getSinAngle( ioff, ilay );
	    const float prevsini = refmodel.getSinAngle( ioff, previdx );
	    mutelayer.start -= (sini-cutoffsin) / (sini-prevsini);
	}

	if ( mutedlayers.validIdx(idx+1) )
	{
	    ilay = mutedlayers[idx+1];
	    mutelayer.stop = float(ilay);
	    const int nextidx = ilay+1;
	    if ( ilay >= 0 && ilay < nrlayers && nextidx < nrlayers )
	    {
		const float sini = refmodel.getSinAngle( ioff, ilay );
		const float nextsini = refmodel.getSinAngle( ioff, nextidx );
		mutelayer.stop += (sini-cutoffsin) / (sini-nextsini);
	    }
	}

	ret += mutelayer;
    }

    if ( ret.size() != 1 )
	return mUdf(float);

    const Interval<float>& intv = ret.first();
    if ( !mIsUdf(intv.start) && mIsUdf(intv.stop) )
	return intv.start;

    if ( innermute )
    {
	if ( mIsEqual(intv.stop,(float)(nrlayers-1),1e-3f) &&
	     !mIsUdf(intv.start) )
	    return intv.start;
    }
    else
    {
	if ( mIsEqual(intv.start,0.f,1e-3f) && !mIsUdf(intv.stop) )
	    return intv.stop;
    }

    return mUdf(float);
}


float AngleMuteBase::getfMutePos( const TimeDepthModel& tdmodel, bool intime,
				  float mutelayer, float offset ) const
{
    if ( mIsZero(offset,1e-4f) )
	return intime ? tdmodel.getTime( 0 ) : tdmodel.getDepth( 0 );

    mutelayer += 1.f; //TD model has one more sample
    const int muteintlayer = (int)mutelayer;
    const float prevzpos = intime ? tdmodel.getTime( muteintlayer )
				  : tdmodel.getDepth( muteintlayer );
    if ( mIsEqual(mutelayer,(float)muteintlayer,1e-4f) )
	return prevzpos;

    const float nextzpos = intime ? tdmodel.getTime( muteintlayer+1 )
				  : tdmodel.getDepth( muteintlayer+1 );
    const float relpos = mutelayer - muteintlayer;
    return prevzpos + ( nextzpos-prevzpos ) * relpos;
}


// PreStack::AngleMute

AngleMute::AngleMute( const char* nm )
    : Processor(nm)
{
    params_ = new AngleMutePars();
}


AngleMute::~AngleMute()
{
    deepErase( muters_ );
}


Muter* AngleMute::getMuter( int idx )
{
    return muters_.validIdx( idx ) ? muters_.get(idx) : nullptr;
}


bool AngleMute::doPrepare( int nrthreads )
{
    deepErase( rtrunners_ );
    deepErase( muters_ );

    if ( !setVelocityFunction() )
	return false;

    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	muters_ += new Muter( params().taperlen_, params().tail_ );
	rtrunners_ += new RayTracerRunner( params().raypar_ );
    }

    return true;
}


bool AngleMute::usePar( const IOPar& par )
{
    if ( !AngleMuteBase::usePar(par) )
	return false;

    par.get( Mute::sTaperLength(), params().taperlen_ );
    par.getYN( Mute::sTailMute(), params().tail_ );

    return true;
}


void AngleMute::fillPar( IOPar& par ) const
{
    AngleMuteBase::fillPar( par );
    par.set( Mute::sTaperLength(), params().taperlen_ );
    par.setYN( Mute::sTailMute(), params().tail_ );
}


bool AngleMute::doWork( od_int64 start, od_int64 stop, int thread )
{
    if ( !rtrunners_.validIdx(thread) )
	return false;

    RayTracerRunner* rtrunner = rtrunners_[thread];
    Muter* muter = getMuter( thread );
    if ( !rtrunner || !muter )
	return false;

    const RayTracer1D::Setup& rtsu = params().rtsu_;
    const bool innermute = params().tail_;

    ElasticModelSet emodels;
    auto* layers = new ElasticModel();
    emodels.add( layers );
    bool nonemuted, allmuted;
    TypeSet<float> offsets;
    offsets.setCapacity( 100, true );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const Gather* input = inputs_[idx];
	Gather* output = outputs_[idx];
	if ( !input || !output )
	    continue;

	const TrcKey& tk = input->getTrcKey();
	layers->setEmpty();
	if ( !getLayers(tk,*layers,errmsg_) )
	    continue;

	rtrunner->setModel( emodels, &rtsu );
	const int nroffsets = input->size( Gather::offsetDim()==0 );
	offsets.setEmpty();
	for ( int ioff=0; ioff<nroffsets; ioff++ )
	    offsets += input->getOffset( ioff );

	rtrunner->setOffsets( offsets, input->offsetType() );
	if ( !rtrunner->executeParallel(raytraceparallel_) )
	    { errmsg_ = rtrunner->uiMessage(); continue; }

	ConstRefMan<ReflectivityModelSet> refmodels = rtrunner->getRefModels();
	const ReflectivityModelBase* refmodel = refmodels ? refmodels->get(0)
							  : nullptr;
	if ( !refmodel || !refmodel->hasAngles() )
	    return false;

	const TimeDepthModel& tdmodel = refmodel->getDefaultModel();
	Array1DSlice<float> trace( output->data() );
	trace.setDimMap( 0, Gather::zDim() );

	const ZSampling& zrg = input->zRange();
	const bool zistime = input->zIsTime();
	const int nrsamps = input->size( Gather::zDim() == 0 );
	float zpos;
	for ( int ioff=0; ioff<nroffsets; ioff++ )
	{
	    trace.setPos( Gather::offsetDim(), ioff );
	    if ( !trace.init() )
		continue;

	    const float offset = offsets[ioff];
	    TypeSet< Interval<float> > mutelayeritvs;
	    const float mutelayer = getOffsetMuteLayer( *refmodel, ioff,
						innermute, nonemuted,
						allmuted, mutelayeritvs );
	    if ( nonemuted )
		continue;

	    if ( allmuted )
	    {
		trace.setAll( 0.f );
		continue;
	    }

	    if ( !mIsUdf(mutelayer) )
	    {
		zpos = getfMutePos( tdmodel, zistime, mutelayer, offset );
		const float muteflayer = zrg.getfIndex( zpos );
		muter->mute( trace, nrsamps, muteflayer );
		continue;
	    }

	    for ( auto& itvml : mutelayeritvs )
	    {
		if ( mIsUdf(itvml.start) )
		    continue;

		zpos = getfMutePos( tdmodel, zistime, itvml.start, offset );
		itvml.start = zrg.getfIndex( zpos );
		if ( mIsUdf(itvml.stop) )
		    continue;

		zpos = getfMutePos( tdmodel, zistime, itvml.stop, offset );
		itvml.stop = zrg.getfIndex( zpos );
	    }

	    if ( !mutelayeritvs.isEmpty() )
		muter->muteIntervals( trace, nrsamps, mutelayeritvs );
	}
    }

    return true;
}


AngleMute::AngleMutePars& AngleMute::params()
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }


const AngleMute::AngleMutePars& AngleMute::params() const
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }


void AngleMute::removeClass()
{
    factory().removeCreator( createInstance );
}

} // namespace PreStack
