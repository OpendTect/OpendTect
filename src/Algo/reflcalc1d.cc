/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reflcalc1d.h"

#include "ailayer.h"
#include "uistrings.h"

mImplFactory(ReflCalc1D,ReflCalc1D::factory)


float ReflCalc1D::sDefAngle( bool indeg )
{
    return indeg ? 15.f : 15.f * mDeg2RadF;
}


StepInterval<float> ReflCalc1D::sDefAngleRange( bool indeg )
{
    StepInterval<float> angles( 0.f, 30.f, 5.f );
    if ( !indeg )
	angles.scale( mDeg2RadF );
    return angles;
}


// ReflCalc1D::Setup

ReflCalc1D::Setup::Setup()
{
}


ReflCalc1D::Setup::~Setup()
{
}


// ReflCalc1D

ReflCalc1D::ReflCalc1D()
    : model_(*new ElasticModel())
{}


ReflCalc1D::~ReflCalc1D()
{
    delete &model_;
    delete [] reflectivities_;
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par, uiString& errm )
{
    return createInstance( par, nullptr, errm );
}


ReflCalc1D* ReflCalc1D::createInstance( const IOPar& par,
					const ElasticModel* model,
					uiString& errm )
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
    setAngles( angles, angleindegrees );
    return setup().usePar( par );
}


void ReflCalc1D::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), factoryKeyword() );
    TypeSet<float> angles;
    getAngles( angles, true );
    par.set( sKeyAngle(), angles );
    par.setYN( sKeyAngleInDegrees(), true );
    setup().fillPar( par );
}


bool ReflCalc1D::hasSameParams( const ReflCalc1D& oth ) const
{
    TypeSet<float> othangles;
    oth.getAngles( othangles, false );
    const BufferString othkeyword = oth.factoryKeyword();
    return othkeyword==factoryKeyword() && thetaangles_ == othangles;
}


void ReflCalc1D::setIOParsToSingleAngle( IOPar& par, float thetaangle,
					 bool angleisindegrees )
{
    par.set( sKeyAngle(), thetaangle );
    par.setYN( sKeyAngleInDegrees(), angleisindegrees );
}


void ReflCalc1D::setAngle( float thetaangle, bool angleisindegrees )
{
    TypeSet<float> angles; angles += thetaangle;
    setAngles( angles, angleisindegrees );
}


void ReflCalc1D::setAngles( const TypeSet<float>& angles, bool angleisindegrees)
{
    thetaangles_ = angles;
    if ( angleisindegrees )
    {
	for ( auto& ang : thetaangles_ )
	    ang *= mDeg2RadF;
    }
}


void ReflCalc1D::getAngles(TypeSet<float>& angles, bool retindegrees ) const
{
    angles = thetaangles_;
    if ( retindegrees )
    {
	for ( auto& ang : angles )
	    ang *= mRad2DegF;
    }
}


bool ReflCalc1D::setModel( const ElasticModel& lys )
{
    msg_.setEmpty();

    model_ = lys;
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

    AngleReflectivityModel::Setup amsu;
    IOPar par;
    setup().fillPar( par );
    amsu.usePar( par );
    refmodel_ = new AngleReflectivityModel( getModel(), thetaangles_, amsu );
    if ( !refmodel_ || !refmodel_->isOK() || !refmodel_->hasReflectivities() )
	return false;

    const int nrmodels = refmodel_->nrRefModels();
    if ( nrmodels != thetaangles_.size() )
	return false;

    depths_ = refmodel_->getDefaultModel().getDepths()+1;
    twt_ = refmodel_->getDefaultModel().getTimes()+1;

    deleteAndZeroArrPtr( reflectivities_ );
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
	const ElasticLayer& elay0 = model_[layer];
	const ElasticLayer& elay1 = model_[layer+1];
	for ( int angidx=0; angidx<nrmodels; angidx++ )
	{
	    const float& thetaangle = thetaangles_[angidx];
	    if ( mIsZero(thetaangle,1e-4f) )
	    {
		AICalc1D::computeAI( elay0, elay1,
				     reflectivities_[angidx][layer] );
	    }
	    else
	    {
		compute( threadidx, elay0, elay1, thetaangle,
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
    setAngle( 0.f, false );
}


void AICalc1D::compute( int /* threadidx */,
			const ElasticLayer& ail0, const ElasticLayer& ail1,
			float /* thetaangle */,
			float_complex& reflectivity )
{
    return computeAI( ail0, ail1, reflectivity );
}


void AICalc1D::computeAI( const ElasticLayer& ail0, const ElasticLayer& ail1,
			  float_complex& reflectivity )
{
    const float ai0 = ail0.vel_ * ail0.den_;
    const float ai1 = ail1.vel_ * ail1.den_;
    const float real =
	   mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF) ? mUdf(float)
							  : (ai1-ai0)/(ai1+ai0);
    reflectivity = float_complex( real, 0.f );
}
