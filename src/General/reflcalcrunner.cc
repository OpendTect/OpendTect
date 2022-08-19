/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reflcalcrunner.h"

#include "ailayer.h"
#include "iopar.h"

const char* ReflCalcRunner::sKeyParallel()	{ return "parallel"; }


ReflCalcRunner::ReflCalcRunner( const char* refl1dfactkeywd )
    : ParallelTask("Reflectivity calculator")
    , reflpar_(*new IOPar())
{
    const Factory<ReflCalc1D>& refl1dfact = ReflCalc1D::factory();
    if ( refl1dfact.isEmpty() )
	return;

    if ( refl1dfact.hasName(refl1dfactkeywd) )
    {
	reflpar_.set( sKey::Type(), refl1dfactkeywd );
	return;
    }

    const BufferStringSet& factnms = refl1dfact.getNames();
    if ( !factnms.isEmpty() )
    {
	const StringView defnm( refl1dfact.getDefaultName() );
	if ( !defnm.isEmpty() )
	    reflpar_.set( sKey::Type(), defnm.str() );
    }
}


ReflCalcRunner::ReflCalcRunner( const IOPar& reflpars )
    : ParallelTask("Raytracing")
    , reflpar_(*new IOPar(reflpars))
{
    msg_ = tr("Running reflectivity calculation");
}


ReflCalcRunner::ReflCalcRunner( const TypeSet<ElasticModel>& aims,
				const IOPar& reflpars )
    : ReflCalcRunner(reflpars)
{
    setModel( aims );
}


ReflCalcRunner::~ReflCalcRunner()
{
    deepErase( reflcalcs_ );
    delete &reflpar_;
}


od_int64 ReflCalcRunner::nrIterations() const
{
    return totalnr_;
}


uiString ReflCalcRunner::uiNrDoneText() const
{
    return tr("Layers done");
}


od_int64 ReflCalcRunner::nrDone() const
{
    od_int64 nrdone = 0;
    for ( const auto* reflcalc : reflcalcs_ )
	nrdone += reflcalc->nrDone();
    return nrdone;
}


void ReflCalcRunner::setAngle( float angle, bool angleisindegrees )
{
    TypeSet<float> angles; angles += angle;
    setAngles( angles, angleisindegrees );
}


void ReflCalcRunner::setAngles( const TypeSet<float>& angles,
				bool angleisindegrees )
{
    reflpar_.set( ReflCalc1D::sKeyAngle(), angles );
    reflpar_.setYN( ReflCalc1D::sKeyAngleInDegrees(), angleisindegrees );
    for ( auto* reflcalc : reflcalcs_ )
	reflcalc->setAngles( angles, angleisindegrees );
}


#define mErrRet(msg) { msg_ = msg; return false; }

bool ReflCalcRunner::setModel( const TypeSet<ElasticModel>& aimodels )
{
    deepErase( reflcalcs_ );

    if ( aimodels.isEmpty() )
	mErrRet( tr("No Elastic models set") );

    if ( ReflCalc1D::factory().getNames().isEmpty() )
	mErrRet( tr("Reflectivity calculation factory is empty") )

    uiString errmsg;
    totalnr_ = 0;
    for ( const auto& aimodel : aimodels )
    {
	ReflCalc1D* reflcalc = ReflCalc1D::createInstance( reflpar_, &aimodel,
							   errmsg );
	if ( !reflcalc )
	{
	    uiString msg = tr( "Wrong input for reflectivity calculation"
				" on model: %1" )
				.arg(aimodels.indexOf(aimodel)+1);
	    msg.append( errmsg, true );
	    mErrRet( msg );
	}

	const int totnr = reflcalc->totalNr();
	if ( totnr > 0 )
	    totalnr_ += totnr;

	reflcalcs_ += reflcalc;
    }

    return true;
}


bool ReflCalcRunner::doPrepare( int /* nrthreads */ )
{
    msg_ = tr("Running reflectivity calculation");

    return !reflcalcs_.isEmpty();
}


bool ReflCalcRunner::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const bool parallel = start == 0 && threadidx == 0 &&
		(stop == nrIterations()-1);

    bool startlayer = false;
    int startmdlidx = modelIdx( start, startlayer );
    if ( !startlayer ) startmdlidx++;
    const int stopmdlidx = modelIdx( stop, startlayer );
    for ( int idx=startmdlidx; idx<=stopmdlidx; idx++ )
    {
	ParallelTask* reflcalc = reflcalcs_[idx];
	if ( !reflcalc->executeParallel(!parallel) )
	    mErrRet( reflcalc->uiMessage() );
    }

    return true;
}


bool ReflCalcRunner::doFinish( bool success )
{
    if ( !success )
	deepErase( reflcalcs_ );

    return success;
}


int ReflCalcRunner::modelIdx( od_int64 idx, bool& startlayer ) const
{
    od_int64 stopidx = -1;
    startlayer = false;
    int modelidx = 0;
    for ( const auto* reflcalc : reflcalcs_ )
    {
	if ( idx == (stopidx + 1) )
	    startlayer = true;

	const int totnr = reflcalc->totalNr();
	stopidx += totnr > 0 ? totnr : 0;
	if ( stopidx>=idx )
	    return modelidx;

	modelidx++;
    }

    return -1;
}


ConstRefMan<ReflectivityModelSet> ReflCalcRunner::getRefModels() const
{
    RefMan<ReflectivityModelSet> ret = new ReflectivityModelSet( reflpar_ );
    getResults( *ret.ptr() );
    return ConstRefMan<ReflectivityModelSet>( ret.ptr() );
}


bool ReflCalcRunner::getResults( ReflectivityModelSet& ret ) const
{
    const ObjectSet<ReflCalc1D>& reflcalcs = reflcalcs_;
    if ( reflcalcs.isEmpty() )
	return true;

    for ( const auto* reflcalc : reflcalcs )
    {
	ConstRefMan<AngleReflectivityModel> refmodel = reflcalc->getRefModel();
	if ( !refmodel || !refmodel->isOK() )
	    return false;

	ret.add( *refmodel );
    }

    return true;
}


ConstRefMan<ReflectivityModelSet> ReflCalcRunner::getRefModels(
				    const TypeSet<ElasticModel>& emodels,
				    const IOPar& reflpar, uiString& msg,
				    TaskRunner* taskrun,
			    const ObjectSet<const TimeDepthModel>* tdmodels )
{
    IOPar reflpars( reflpar );
    if ( !reflpar.isPresent(sKey::Type()) )
    {
	const BufferStringSet& facnms = ReflCalc1D::factory().getNames();
	if ( !facnms.isEmpty() )
	{
	    const BufferString defnm( ReflCalc1D::factory().getDefaultName() );
	    reflpars.set( sKey::Type(), defnm.isEmpty()
			  ? AICalc1D::sFactoryKeyword() : defnm.str() );
	}
    }

    if ( !reflpars.isPresent(ReflCalc1D::sKeyAngle()) )
	ReflCalc1D::setIOParsToSingleAngle( reflpars );

    ReflCalcRunner reflrunner( emodels, reflpars );
    bool parallel = true;
    if ( reflpars.getYN(sKeyParallel(),parallel) )
	reflrunner.doParallel( parallel );

    if ( !TaskRunner::execute(taskrun,reflrunner) )
    {
	msg = reflrunner.uiMessage();
	return nullptr;
    }

    RefMan<ReflectivityModelSet> refmodels =
				    new ReflectivityModelSet( reflpars );
    if ( !reflrunner.getResults(*refmodels.ptr()) )
	return nullptr;

    if ( tdmodels )
	refmodels->use( *tdmodels );

    return refmodels.ptr();
}
