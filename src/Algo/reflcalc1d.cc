/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reflcalc1d.h"

#include "ailayer.h"
#include "keystrs.h"
#include "uistrings.h"

#include "hiddenparam.h"

mImplFactory(ReflCalc1D,ReflCalc1D::factory)


float ReflCalc1D::sDefAngle( bool indeg )
{
    return sDefAngle( indeg ? Seis::OffsetType::AngleDegrees
			    : Seis::OffsetType::AngleRadians );
}


StepInterval<float> ReflCalc1D::sDefAngleRange( bool indeg )
{
    return sDefAngleRange( indeg ? Seis::OffsetType::AngleDegrees
				 : Seis::OffsetType::AngleRadians );
}


float ReflCalc1D::sDefAngle( Seis::OffsetType typ )
{
    return typ == Seis::OffsetType::AngleDegrees ? 15.f : 15.f * mDeg2RadF;
}


StepInterval<float> ReflCalc1D::sDefAngleRange( Seis::OffsetType typ )
{
    StepInterval<float> angles( 0.f, 30.f, 5.f );
    if ( typ == Seis::OffsetType::AngleRadians )
	angles.scale( mDeg2RadF );
    return angles;
}


// ReflCalc1D::Setup

class ReflCalc1DHP
{
public:
				ReflCalc1DHP()	{}

    ReflCalc1DHP&		starttime( float t0 )
				{ starttime_ = t0; return *this; }
    ReflCalc1DHP&		startdepth( float z0 )
				{ startdepth_ = z0; return *this; }
    ReflCalc1DHP&		depthtype( ZDomain::DepthType typ )
				{ depthtype_ = typ; return *this; }

    float starttime_ = 0.f;
    float startdepth_ = 0.f;
    ZDomain::DepthType depthtype_ = ZDomain::DepthType::Meter;
};

static HiddenParam<ReflCalc1D::Setup,ReflCalc1DHP*>
						reflcalc1dsuparsmgr_(nullptr);

ReflCalc1D::Setup::Setup()
{
    reflcalc1dsuparsmgr_.setParam( this, new ReflCalc1DHP() );
}


ReflCalc1D::Setup::Setup( const Setup& oth )
{
    reflcalc1dsuparsmgr_.setParam( this, new ReflCalc1DHP() );
    *this = oth;
}


ReflCalc1D::Setup::~Setup()
{
    reflcalc1dsuparsmgr_.removeAndDeleteParam( this );
}


ReflCalc1D::Setup& ReflCalc1D::Setup::operator =( const Setup& oth )
{
    if ( &oth == this )
	return *this;

    *reflcalc1dsuparsmgr_.getParam( this ) =
				*reflcalc1dsuparsmgr_.getParam( &oth );

    return *this;
}


ReflCalc1D::Setup& ReflCalc1D::Setup::starttime( float val )
{
    reflcalc1dsuparsmgr_.getParam( this )->starttime( val );
    return *this;
}


ReflCalc1D::Setup& ReflCalc1D::Setup::startdepth( float val )
{
    reflcalc1dsuparsmgr_.getParam( this )->startdepth( val );
    return *this;
}


ReflCalc1D::Setup& ReflCalc1D::Setup::depthtype( ZDomain::DepthType typ )
{
    reflcalc1dsuparsmgr_.getParam( this )->depthtype( typ );
    return *this;
}


float ReflCalc1D::Setup::getStartTime() const
{
    return reflcalc1dsuparsmgr_.getParam( this )->starttime_;
}


float ReflCalc1D::Setup::getStartDepth() const
{
    return reflcalc1dsuparsmgr_.getParam( this )->startdepth_;
}


ZDomain::DepthType ReflCalc1D::Setup::depthType() const
{
    return reflcalc1dsuparsmgr_.getParam( this )->depthtype_;
}


bool ReflCalc1D::Setup::areDepthsInFeet() const
{
    return depthType() == ZDomain::DepthType::Feet;
}


// ReflCalc1D

ReflCalc1D::ReflCalc1D()
    : model_(*new ElasticModelOv())
{}


ReflCalc1D::~ReflCalc1D()
{
    delete &model_;
    delete [] reflectivities_;
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par, uiString& errm )
{
    return createInstance( par, nullptr, errm, nullptr );
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par,
					const ElasticModel* model,
					uiString& errm )
{
    return createInstance( par, model, errm, nullptr );
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par, uiString& errm,
					const Setup* rfsu )
{
    return createInstance( par, nullptr, errm, rfsu );
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par,
					const ElasticModel* model,
					uiString& errm, const Setup* rfsu )
{
    BufferString typekey;
    par.get( sKey::Type(), typekey );

    const Factory<ReflCalc1D>& reflfact = factory();
    if ( !reflfact.hasName(typekey) && !reflfact.isEmpty() )
    {
	const StringView defnm = reflfact.getDefaultName();
	typekey.set( defnm.isEmpty() ? reflfact.getNames().first()->buf()
				     : defnm.buf() );
    }

    ReflCalc1D* reflcalc = reflfact.create( typekey );
    if ( !reflcalc )
    {
	errm = tr("Reflectivity calculator not found."
		  " Perhaps all plugins are not loaded");
	return nullptr;
    }

    if ( rfsu )
	reflcalc->setup() = *rfsu;

    if ( !reflcalc->usePar(par) )
    {
	errm = reflcalc->uiMessage();
	delete reflcalc;
	return nullptr;
    }

    if ( model && !reflcalc->setModel(*model) )
    {
	errm = reflcalc->uiMessage();
	delete reflcalc;
	return nullptr;
    }

    return reflcalc;
}


bool ReflCalc1D::usePar( const IOPar& par )
{
    TypeSet<float> angles;
    par.get( sKeyAngle(), angles );
    if ( angles.isEmpty() )
	angles += 0.f;

    bool angleindegrees = false;
    par.getYN( sKeyAngleInDegrees(), angleindegrees );
    setAngles( angles, angleindegrees ? Seis::OffsetType::AngleDegrees
				      : Seis::OffsetType::AngleRadians );
    return setup().usePar( par );
}


void ReflCalc1D::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), factoryKeyword() );
    TypeSet<float> angles;
    const Seis::OffsetType typ = Seis::OffsetType::AngleDegrees;
    getAngles( angles, typ );
    par.set( sKeyAngle(), angles );
    par.setYN( sKeyAngleInDegrees(), typ == Seis::OffsetType::AngleDegrees );
    setup().fillPar( par );
}


bool ReflCalc1D::hasSameParams( const ReflCalc1D& oth ) const
{
    TypeSet<float> othangles;
    oth.getAngles( othangles, Seis::OffsetType::AngleRadians );
    const BufferString othkeyword = oth.factoryKeyword();
    return othkeyword==factoryKeyword() && thetaangles_ == othangles;
}


void ReflCalc1D::setIOParsToSingleAngle( IOPar& par, float thetaangle,
					 bool angleisindegrees )
{
    setIOParsToSingleAngle( par, thetaangle,
		angleisindegrees ? Seis::OffsetType::AngleDegrees
				 : Seis::OffsetType::AngleRadians );
}


void ReflCalc1D::setIOParsToSingleAngle( IOPar& par, float thetaangle,
					 Seis::OffsetType typ )
{
    par.set( sKeyAngle(), thetaangle );
    par.setYN( sKeyAngleInDegrees(), typ == Seis::OffsetType::AngleDegrees );
}


void ReflCalc1D::setAngle( float thetaangle, bool angleisindegrees )
{
    setAngle( thetaangle,
		angleisindegrees ? Seis::OffsetType::AngleDegrees
				 : Seis::OffsetType::AngleRadians );
}


void ReflCalc1D::setAngle( float thetaangle, Seis::OffsetType typ )
{
    TypeSet<float> angles; angles += thetaangle;
    setAngles( angles, typ );
}


void ReflCalc1D::setAngles( const TypeSet<float>& angles, bool angleisindegrees)
{
    setAngles( angles,
		angleisindegrees ? Seis::OffsetType::AngleDegrees
				 : Seis::OffsetType::AngleRadians );
}


void ReflCalc1D::setAngles( const TypeSet<float>& angles, Seis::OffsetType typ )
{
    if ( !Seis::isOffsetAngle(typ) )
	return;

    thetaangles_ = angles;
    if ( typ == Seis::OffsetType::AngleDegrees )
    {
	for ( auto& ang : thetaangles_ )
	    ang *= mDeg2RadF;
    }
}


void ReflCalc1D::getAngles( TypeSet<float>& angles, bool retindegrees ) const
{
    getAngles( angles, retindegrees ? Seis::OffsetType::AngleDegrees
				    : Seis::OffsetType::AngleRadians );
}


void ReflCalc1D::getAngles( TypeSet<float>& angles,
			    Seis::OffsetType rettyp ) const
{
    angles = thetaangles_;
    if ( rettyp == Seis::OffsetType::AngleDegrees )
    {
	for ( auto& ang : angles )
	    ang *= mRad2DegF;
    }
}


bool ReflCalc1D::areDepthsInFeet() const
{
    return setup().areDepthsInFeet();
}


bool ReflCalc1D::setModel( const ElasticModel& lys )
{
    msg_.setEmpty();
    if ( thetaangles_.isEmpty() )
    {
	msg_ = tr("Internal: Angles must be set before the model");
	msg_.append( tr("Cannot calculate reflectivities." ), true );
	return false;
    }

    const bool zeroanglesonly =
	thetaangles_.size()==1 && mIsZero(thetaangles_[0],mDefEps);

    const RefLayer::Type reqtype = zeroanglesonly && !needsSwave()
				 ? RefLayer::Acoustic : RefLayer::Elastic;
    model_.copyFrom( lys, reqtype );

    int firsterror = -1;
    model_.checkAndClean( firsterror, true, needsSwave() );

    if ( model_.isEmpty() )
	msg_ = tr( "Model is empty" );
    else if ( firsterror != -1 )
	msg_ = tr( "Model has invalid values on layer: %1" )
		      .arg( firsterror+1 );

    return !model_.isEmpty();
}


od_int64 ReflCalc1D::nrIterations() const
{ return model_.size(); }


bool ReflCalc1D::doPrepare( int /* nrthreads */ )
{
    if ( !msg_.isEmpty() )
	return false;

    if ( thetaangles_.isEmpty() )
    {
	msg_ = tr("Internal: Angles must be set before the model");
	msg_.append( tr("Cannot do reflectivity calculation." ), true );
	return false;
    }

    if ( !isOK() )
	return false;

    const Setup& su = setup();
    AngleReflectivityModel::Setup amsu;
    amsu.starttime( su.getStartTime() ).startdepth( su.getStartDepth() )
	.depthtype( su.depthType() );
    IOPar par;
    su.fillPar( par );
    amsu.usePar( par );
    refmodel_ = new AngleReflectivityModel( getModel(), thetaangles_, amsu );
    if ( !refmodel_ || !refmodel_->isOK() || !refmodel_->hasReflectivities() )
	return false;

    const int nrmodels = refmodel_->nrRefModels();
    if ( nrmodels != thetaangles_.size() )
	return false;

    depths_ = refmodel_->getDefaultModel().getDepths()+1;
    twt_ = refmodel_->getDefaultModel().getTimes()+1;

    deleteAndNullArrPtr( reflectivities_ );
    mTryAlloc( reflectivities_, float_complex*[nrmodels] );
    if ( !reflectivities_ )
	return false;

    for ( int idx=0; idx<nrmodels; idx++ )
	reflectivities_[idx] = refmodel_->getRefs( idx );

    return true;
}


bool ReflCalc1D::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    if ( stop == nrIterations()-1 )
	stop--;

    const int nrmodels = refmodel_->nrRefModels();
    for ( int layer=mCast(int,start); layer<=stop; layer++, addToNrDone(1) )
    {
	const AILayer& ailay0 = model_.get(layer)->asAcoustic();
	const AILayer& ailay1 = model_.get(layer+1)->asAcoustic();
	const ElasticLayer* elay0 = ailay0.asElastic();
	const ElasticLayer* elay1 = ailay1.asElastic();
	for ( int angidx=0; angidx<nrmodels; angidx++ )
	{
	    const float& thetaangle = thetaangles_[angidx];
	    if ( mIsZero(thetaangle,1e-4f) )
	    {
		AICalc1D::computeAI( ailay0, ailay1,
				     reflectivities_[angidx][layer] );
	    }
	    else
	    {
		if ( !elay0 || !elay1 )
		    return false;

		compute( threadidx, *elay0, *elay1, thetaangle,
			 reflectivities_[angidx][layer] );
	    }
	}
    }

    return true;
}


bool ReflCalc1D::doFinish( bool success )
{
    if ( success )
	msg_.setEmpty();

    return success;
}


ConstRefMan<AngleReflectivityModel> ReflCalc1D::getRefModel() const
{
    return refmodel_;
}


float ReflCalc1D::getDepth( int layer ) const
{
    return depths_[layer];
}


float ReflCalc1D::getTime( int layer ) const
{
    return twt_[layer];
}


// AICalc1D

AICalc1D::AICalc1D()
    : ReflCalc1D()
{
    setAngle( 0.f, Seis::OffsetType::AngleRadians );
}


void AICalc1D::compute( int /* threadidx */,
			const ElasticLayer& ail0, const ElasticLayer& ail1,
			float /* thetaangle */,
			float_complex& reflectivity )
{
    return computeAI( ail0, ail1, reflectivity );
}


void AICalc1D::computeAI( const AILayer& ail0, const AILayer& ail1,
			  float_complex& reflectivity )
{
    const float ai0 = ail0.getAI();
    const float ai1 = ail1.getAI();
    const float real = mIsUdf(ai0) || mIsUdf(ai1) ||
		       (mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF))
		     ? mUdf(float) : (ai1-ai0)/(ai1+ai0);
    reflectivity = float_complex( real, 0.f );
}
