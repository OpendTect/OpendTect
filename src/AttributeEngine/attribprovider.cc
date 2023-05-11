/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "attribstorprovider.h"

#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attriblinebuffer.h"
#include "attribparam.h"

#include "binidvalset.h"
#include "convmemvalseries.h"
#include "trckeyzsampling.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seiscubeprov.h"
#include "seisinfo.h"
#include "seisselectionimpl.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "task.h"
#include "uistrings.h"
#include "valseriesinterpol.h"


namespace Attrib
{

class ProviderTask : public ParallelTask
{
public:

ProviderTask( Provider& p )
    : provider_( p )
{}


int minThreadSize() const override { return provider_.minTaskSize(); }


void setVars( const DataHolder* res, const BinID& relpos, int z0,
	      int nrsamples )
{
    res_ = res;
    relpos_ = relpos;
    z0_ = z0;
    nrsamples_ = nrsamples;
}


od_int64 nrIterations() const override { return nrsamples_; }


bool doWork( od_int64 start, od_int64 stop, int threadid ) override
{
    if ( !res_ ) return true;
    return provider_.computeData( *res_, relpos_, mCast(int,start+z0_),
				  mCast(int,stop-start+1), threadid );
}


bool doPrepare( int nrthreads ) override
{ return provider_.setNrThreads( nrthreads ); }


bool doFinish( bool success ) override
{ return provider_.finalizeCalculation( success ); }

protected:

    Provider&			provider_;
    const DataHolder*		res_;
    BinID			relpos_;
    int				z0_;
    int				nrsamples_;
};


Provider* Provider::create( Desc& desc, uiString& errstr )
{
    ObjectSet<Provider> existing;
    bool issame = false;
    Provider* prov = internalCreate( desc, existing, issame, errstr );
    if ( !prov ) return 0;

    prov->allexistingprov_ = existing;
    return prov;
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing,
				    bool& issame, uiString& errstr )
{
    for ( int idx=0; idx<existing.size(); idx++ )
    {
	if ( existing[idx]->getDesc().isIdenticalTo( desc, false ) )
	{
	    const int selout = desc.selectedOutput();
	    if ( existing[idx]->getDesc().selectedOutput() != selout )
		existing[idx]->enableOutput( selout );
	    issame = true;
	    existing[idx]->setUsedMultTimes();
	    return existing[idx];
	}
    }

    if ( desc.nrInputs() && !desc.descSet() )
    {
	errstr = tr("No attribute set specified");
	return 0;
    }

    Provider* newprov = PF().create( desc );
    if ( !newprov )
    {
	StringView errmsg = desc.errMsg();
	if ( errmsg )
	{
	    if ( errmsg==DescSet::storedIDErrStr() && desc.isStored() )
	    {
		errstr = tr( "Impossible to find stored data '%1'\n"
				 "used as input for other attribute(s). \n"
				 "Data might have been deleted or corrupted.\n"
				 "Please check your attribute set \n"
				 "Please select valid stored data.")
				.arg( desc.userRef() );
	    }
	    else
	    {

		BufferString usrref = desc.userRef();
		if ( usrref.startsWith("CentralSteering")
		    || usrref.startsWith("FullSteering") )
		    return 0;

		errstr = tr( "Attribute '%1'\n\n%2")
			 .arg(desc.userRef()).arg(errmsg);
	    }
	}
	else
	{
	    errstr = tr( "Error in definition of %1 attribute." )
		     .arg( desc.attribName() );
	}
	return 0;
    }

    newprov->ref();

    if ( desc.selectedOutput()!=-1 && existing.isEmpty() )
	newprov->enableOutput( desc.selectedOutput(), true );

    existing += newprov;

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	Desc* inputdesc = desc.getInput(idx);
	if ( !inputdesc ) continue;

	Provider* inputprovider =
			internalCreate( *inputdesc, existing, issame, errstr );
	if ( !inputprovider )
	{
	    existing.removeRange( existing.indexOf(newprov),existing.size()-1 );
	    newprov->unRef();
	    return 0;
	}

	if ( newprov == inputprovider )
	{
	    existing.removeRange( existing.indexOf(newprov),existing.size()-1 );
	    newprov->unRef();
	    errstr =
		tr("Input is not correct. One of the inputs depends on itself");
	    return 0;
	}

	newprov->setInput( idx, inputprovider );
	inputprovider->addParent(newprov);
	issame = false;
    }

    if ( !newprov->checkInpAndParsAtStart() )
    {
	existing.removeRange( existing.indexOf(newprov), existing.size()-1 );
	BufferString attribnm = newprov->desc_.attribName();
	if ( attribnm == StorageProvider::attribName() )
	{
	    errstr = tr("Cannot load Stored Cube '%1'.")
		     .arg( newprov->desc_.userRef() );
	}
	else
	{
	    errstr = tr("Attribute \"%1\" of type \"%2\" cannot be initialized")
		     .arg( newprov->desc_.userRef() ).arg( attribnm );
	}
	newprov->unRef();
	return 0;
    }

    newprov->unRefNoDelete();
    return newprov;
}


Provider::Provider( Desc& nd )
    : desc_( nd )
    , outputinterest_( nd.nrOutputs(), 0 )
    , desbufferstepout_( 0, 0 )
    , reqbufferstepout_( 0, 0 )
    , desiredvolume_( 0 )
    , possiblevolume_( 0 )
    , providertask_( 0 )
    , linebuffer_( 0 )
    , currentbid_( BinID::udf() )
    , prevtrcnr_( 0 )
    , geomid_(Survey::GM().cUndefGeomID())
    , seldata_( 0 )
    , extraz_( 0, 0 )
    , curtrcinfo_( 0 )
    , trcinfobid_( BinID::udf() )
    , useshortcuts_( 0 )
    , refz0_( 0 )
    , refstep_( 0 )
    , alreadymoved_( 0 )
    , isusedmulttimes_( 0 )
    , needinterp_( 0 )
    , dataunavailableflag_(false)
{
    desc_.ref();
    inputs_.allowNull( true );
    for ( int idx=0; idx<desc_.nrInputs(); idx++ )
	inputs_ += 0;


    if ( !desc_.descSet() )
	errmsg_ = tr("No attribute set specified");
}


Provider::~Provider()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] ) inputs_[idx]->unRef();

    inputs_.erase();
    allexistingprov_.erase();

    desc_.unRef();

    delete providertask_;

    delete linebuffer_;
    delete possiblevolume_;
    delete desiredvolume_;
}


bool Provider::is2D() const
{ return getDesc().descSet() ? getDesc().descSet()->is2D() : getDesc().is2D(); }


bool Provider::isOK() const
{
    return errmsg_.isEmpty(); /* Huh? &parser && parser.isOK(); */
}


bool Provider::isSingleTrace() const
{
    return desc_.locality() == Desc::SingleTrace;
}


bool Provider::usesTracePosition() const
{
    return desc_.usesTracePosition();
}


Desc& Provider::getDesc()
{
    return desc_;
}


const Desc& Provider::getDesc() const
{
    return const_cast<Provider*>(this)->getDesc();
}


void Provider::enableOutput( int out, bool yn )
{
    if ( out<0 || out >= outputinterest_.size() )
	{ pErrMsg( "Huh?" ); return; }

    if ( yn )
	outputinterest_[out]++;
    else
    {
	if ( !outputinterest_[out] )
	    { pErrMsg( "Huh2?"); return; }
	outputinterest_[out]--;
    }
}


bool Provider::isOutputEnabled( int out ) const
{
    if ( out<0 || out >= outputinterest_.size() )
	return false;
    else
	return outputinterest_[out];
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl() <= prefix##bufferstepout_.inl() \
	    && ns.crl() <= prefix##bufferstepout_.crl() ) \
	return; \
\
    if ( ns.inl() > prefix##bufferstepout_.inl() ) \
	prefix##bufferstepout_.inl() = ns.inl(); \
    if ( ns.crl() > prefix##bufferstepout_.crl() ) \
	prefix##bufferstepout_.crl() = ns.crl();\
}


void Provider::setDesBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(des);

    if ( !wait )
	updateInputReqs(-1);
}


void Provider::setReqBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(req);

    if ( !wait )
	updateInputReqs(-1);
}


void Provider::setDesiredVolume( const TrcKeyZSampling& ndv )
{
    if ( !desiredvolume_ )
	desiredvolume_ = new TrcKeyZSampling(ndv);
    else
    {
	if ( !isUsedMultTimes() )
	    *desiredvolume_ = ndv;
	else
	{
	    desiredvolume_->hsamp_.start_.inl() =
		desiredvolume_->hsamp_.start_.inl() < ndv.hsamp_.start_.inl() ?
		desiredvolume_->hsamp_.start_.inl() : ndv.hsamp_.start_.inl();
	    desiredvolume_->hsamp_.stop_.inl() =
		desiredvolume_->hsamp_.stop_.inl() > ndv.hsamp_.stop_.inl() ?
		desiredvolume_->hsamp_.stop_.inl() : ndv.hsamp_.stop_.inl();
	    desiredvolume_->hsamp_.stop_.crl() =
		desiredvolume_->hsamp_.stop_.crl() > ndv.hsamp_.stop_.crl() ?
		desiredvolume_->hsamp_.stop_.crl() : ndv.hsamp_.stop_.crl();
	    desiredvolume_->hsamp_.start_.crl() =
		desiredvolume_->hsamp_.start_.crl() < ndv.hsamp_.start_.crl() ?
		desiredvolume_->hsamp_.start_.crl() : ndv.hsamp_.start_.crl();
	    desiredvolume_->zsamp_.start =
		desiredvolume_->zsamp_.start < ndv.zsamp_.start?
		desiredvolume_->zsamp_.start : ndv.zsamp_.start;
	    desiredvolume_->zsamp_.stop =
		desiredvolume_->zsamp_.stop >ndv.zsamp_.stop
				    ? desiredvolume_->zsamp_.stop
				    : ndv.zsamp_.stop;
	}
    }

    TrcKeyZSampling inputcs;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	for ( int idy=0; idy<outputinterest_.size(); idy++ )
	{
	    if ( outputinterest_[idy]<1 || !inputs_[idx] ) continue;

	    bool isstored = inputs_[idx]->desc_.isStored();
	    computeDesInputCube( idx, idy, inputcs, !isstored );
	    inputs_[idx]->setDesiredVolume( inputcs );
	}
    }
}


#define mGetMargin( type, var, tmpvar, tmpvarsource ) \
{ \
    type* tmpvar = tmpvarsource; \
    if ( tmpvar ) { var.start += tmpvar->start; var.stop += tmpvar->stop; } \
}

#define mGetOverallMargin( type, var, funcPost ) \
type var(0,0); \
mGetMargin( type, var, des##var, des##funcPost ); \
mGetMargin( type, var, req##var, req##funcPost )

bool Provider::getPossibleVolume( int output, TrcKeyZSampling& res )
{
    if ( !getDesc().descSet() )
	return false;

    TrcKeyZSampling tmpres = res;
    if ( inputs_.size()==0 )
    {
	if ( !is2D() ) res.init(true);
	if ( !possiblevolume_ )
	    possiblevolume_ = new TrcKeyZSampling;

	if ( is2D() ) *possiblevolume_ = res;
	return true;
    }

    if ( !desiredvolume_ ) return false;

    TypeSet<int> outputs;
    if ( output != -1 )
	outputs += output;
    else
    {
	for ( int idx=0; idx<outputinterest_.size(); idx++ )
	{
	    if ( outputinterest_[idx] > 0 )
		outputs += idx;
	}
    }

    bool isset = false;
    for ( int idx=0; idx<outputs.size(); idx++ )
    {
	const int out = outputs[idx];
	for ( int inp=0; inp<inputs_.size(); inp++ )
	{
	    if ( !inputs_[inp] )
		continue;

	    TrcKeyZSampling inputcs = tmpres;
	    TypeSet<int> inputoutput;
	    if ( !getInputOutput( inp, inputoutput ) )
		continue;

	    for ( int idy=0; idy<inputoutput.size(); idy++ )
	    {
		if ( !inputs_[inp] ) continue;

		computeDesInputCube(inp, out, inputcs, true);
		if ( !inputs_[inp]->getPossibleVolume( idy, inputcs ) )
		    continue;

		const BinID* stepout = reqStepout(inp,out);
		if ( stepout )
		{
		    int inlstepoutfact = desiredvolume_->hsamp_.step_.inl();
		    int crlstepoutfact = desiredvolume_->hsamp_.step_.crl();
		    inputcs.hsamp_.start_.inl() +=
			stepout->inl() * inlstepoutfact;
		    inputcs.hsamp_.start_.crl() +=
			stepout->crl() * crlstepoutfact;
		    inputcs.hsamp_.stop_.inl() -=
			stepout->inl() * inlstepoutfact;
		    inputcs.hsamp_.stop_.crl() -=
			stepout->crl() * crlstepoutfact;
		}

		const Interval<float>* zrg = reqZMargin(inp,out);
		if ( zrg )
		{
		    inputcs.zsamp_.start -= zrg->start;
		    inputcs.zsamp_.stop -= zrg->stop;
		}

		const Interval<int>* zrgsamp = reqZSampMargin(inp,out);
		if ( zrgsamp )
		{
		    inputcs.zsamp_.start -= zrgsamp->start*refstep_;
		    inputcs.zsamp_.stop -= zrgsamp->stop*refstep_;
		}

		res.limitToWithUdf( inputcs );
		isset = true;
	    }
	}
    }

    if ( !possiblevolume_ )
	possiblevolume_ = new TrcKeyZSampling;

    possiblevolume_->hsamp_ = res.hsamp_;
    possiblevolume_->zsamp_ = res.zsamp_;
    return isset;
}


int Provider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    BinID pos = startpos;

    if ( inputs_.size() < 1 )
    {
	pos.setUdf();
	if ( seldata_ && seldata_->type() == Seis::Table )
	{
	    Seis::SelData* nonconstsd = const_cast<Seis::SelData*>(seldata_);
	    mDynamicCastGet( Seis::TableSelData*, tabsel, nonconstsd )
	    if ( tabsel )
	    {
		if ( currentbid_.isUdf() )
		    pos = tabsel->binidValueSet().firstBinID();
		else
		{
		    BinIDValueSet::SPos oldpos =
			tabsel->binidValueSet().find( currentbid_ );
		    if ( tabsel->binidValueSet().next( oldpos, true ) )
			pos = tabsel->binidValueSet().getBinID( oldpos );
		}

		currentbid_ = pos;
		alreadymoved_ = true;
		return currentbid_.isUdf() ? 0 : 1;
	    }
	}
    }

    const bool docheck = pos.isUdf();
    if ( is2D() )
	prevtrcnr_ = currentbid_.crl();

    bool needmove = false;
    bool docontinue = true;
    ObjectSet<Provider> movinginputs;
    while ( docontinue )
    {
	needmove = docheck;
	for ( int idx=0; idx<inputs_.size(); idx++ )
	{
	    if ( !inputs_[idx] ) continue;

	    currentbid_ = inputs_[idx]->getCurrentPosition();
	    trcinfobid_ = inputs_[idx]->getTrcInfoBid();
	    if ( !docheck && currentbid_ == pos ) continue;
	    if ( !docheck && !trcinfobid_.isUdf() && trcinfobid_ == pos )
		continue;

	    needmove = true;
	    const int res = inputs_[idx]->moveToNextTrace(pos, firstcheck);
	    if ( res!=1 ) return res;

	    bool needmscprov = true;
	    if ( !inputs_[idx]->getMSCProvider( needmscprov ) && needmscprov )
		continue;

	    if ( !movinginputs.isPresent( inputs_[idx] ) )
		movinginputs += inputs_[idx];
	}
	if ( !needmove || docheck )
	    docontinue = false;

	if ( !docheck && firstcheck )
	{
	    bool allok = true;
	    for ( int idi=0; idi<inputs_.size(); idi++)
	    {
		if ( inputs_[idi]
		     && !inputs_[idi]->getTrcInfoBid().isUdf()
		     && inputs_[idi]->getTrcInfoBid() != pos )
		{
		    allok = false;
		    break;
		}
	    }

	    if ( !allok )
	    {
		BinID newstart = BinID::udf();
		computeNewStartPos( newstart );

		pos = newstart;
		firstcheck = false;
		resetMoved();
	    }
	}
    }

    if ( movinginputs.isEmpty() && needmove )
    {
	if ( inputs_.isEmpty() && !desc_.isStored() )
	{
	    if ( currentbid_.isUdf() )
		currentbid_ = desiredvolume_->hsamp_.start_;
	    else
	    {
		BinID prevbid = currentbid_;
		BinID step = getStepoutStep();
		if ( prevbid.crl() +step.crl() <=
		     desiredvolume_->hsamp_.stop_.crl() )
		    currentbid_.crl() = prevbid.crl() +step.crl();
		else if ( prevbid.inl() +step.inl() <=
			  desiredvolume_->hsamp_.stop_.inl())
		{
		    currentbid_.inl() = prevbid.inl() +step.inl();
		    currentbid_.crl() = desiredvolume_->hsamp_.start_.crl();
		}
		else
		    return 0;
	    }
	}
	else if ( needmove )
	    currentbid_.setUdf();

	setCurrentPosition( currentbid_ );
	alreadymoved_ = true;
	return 1;
    }

    if ( docheck )
    {
	const int res = alignInputs( movinginputs );
	if ( res != 1 )
	    return res;
    }

    if ( !movinginputs.isEmpty() )
    {
	currentbid_ = movinginputs[0]->getCurrentPosition();
	curtrcinfo_ = movinginputs[0]->getCurrentTrcInfo();
	trcinfobid_ = movinginputs[0]->getTrcInfoBid();
    }

    if ( docheck )
    {
	for ( int idx=0; idx<inputs_.size(); idx++ )
	{
	    if ( !inputs_[idx] ) continue;
	    if ( !inputs_[idx]->setCurrentPosition( currentbid_ ) )
		return -1;
	}
	setCurrentPosition( currentbid_ );
    }

    alreadymoved_ = true;
    return 1;
}


void Provider::resetMoved()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->resetMoved();

    alreadymoved_ = false;
}


void Provider::computeNewStartPos( BinID& newstart )
{
    const BinID step = getStepoutStep();
    for ( int idi=0; idi<inputs_.size(); idi++ )
    {
	BinID inputbid = BinID::udf();
	if ( inputs_[idi] && !inputs_[idi]->getTrcInfoBid().isUdf() )
	    inputbid = inputs_[idi]->getCurrentPosition();

	if ( inputbid.isUdf() )
	    continue;

	if ( newstart.isUdf() )
	    newstart = inputbid;
	else
	{
	    if ( is2D() )
		newstart = newstart.crl()<inputbid.crl() ? inputbid : newstart;
	    else
	    {
		newstart.inl() = step.inl()<0 ?
			       mMIN(newstart.inl(),inputbid.inl()):
			       mMAX(newstart.inl(),inputbid.inl());
		newstart.crl() = step.crl()<0 ?
			       mMIN(newstart.crl(),inputbid.crl()):
			       mMAX(newstart.crl(),inputbid.crl());
	    }
	}
    }
}


int Provider::alignInputs( ObjectSet<Provider>& movinginputs )
{
    updateCurrentInfo();

    bool inp1_is_on_newline = false;
    bool inp2_is_on_newline = false;
    for ( int inp1=0; inp1<movinginputs.size()-1; inp1++ )
    {
	if ( is2D() )
	    inp1_is_on_newline = movinginputs[inp1]->isNew2DLine();

	for ( int inp2=inp1+1; inp2<movinginputs.size(); inp2++ )
	{
	    bool inp1moved = false;
	    if ( is2D() )
		inp2_is_on_newline = movinginputs[inp2]->isNew2DLine();

	    int res = comparePosAndAlign(movinginputs[inp1], inp1_is_on_newline,
					 movinginputs[inp2], inp2_is_on_newline,
					 inp1moved );
	    if ( res != 1 ) return res;

	    if ( inp1moved )
	    {
		inp1 = -1;
		break;
	    }
	}
    }

    return 1;
}

//TODO: compare line name in 2d
int Provider::comparePosAndAlign( Provider* input1, bool inp1_is_on_newline,
	                          Provider* input2, bool inp2_is_on_newline,
	                          bool inp1moved )
{
    bool inp2moved = false;
    while ( true )
    {
	//TODO implement case !isondisc_ ?
	bool needmscp1 = true;
	bool needmscp2 = true;
	SeisMSCProvider* seismscprov1 = input1->getMSCProvider( needmscp1 );
	SeisMSCProvider* seismscprov2 = input2->getMSCProvider( needmscp2 );
	int compres = -1;

	if ( seismscprov1 && seismscprov2 )
	    compres = seismscprov1->comparePos( *seismscprov2 );
	else if ( !needmscp1 || !needmscp2 )
	{
	    const BinID inp1pos = input1->getCurrentPosition();
	    const BinID inp2pos = input2->getCurrentPosition();
	    if ( inp1pos == inp2pos )
		compres = 0;
	    else if ( !is2D() )
	    {
		if ( inp1pos.inl() != inp2pos.inl() )
		    compres = inp1pos.inl() > inp2pos.inl() ? 1 : -1;
		else
		    compres = inp1pos.crl() > inp2pos.crl() ? 1 : -1;
	    }
	}

	if ( compres == 0 )
	    break;

	bool bothnew = inp1_is_on_newline && inp2_is_on_newline;
	if ( ( compres==-1 && !inp1_is_on_newline ) ||
	     ( compres==-1 && bothnew ) ||
	     ( !bothnew && inp2_is_on_newline ) )
	{
	    inp1moved = true;
	    input1->resetMoved();
	    const int res = input1->moveToNextTrace();
	    if ( res != 1 ) return res;
	}
	else if ( compres == 1 || inp1_is_on_newline )
	{
	    inp2moved = true;
	    input2->resetMoved();
	    const int res = input2->moveToNextTrace();
	    if ( res != 1 ) return res;
	}
    }

    if ( inp1moved || inp2moved )
	updateCurrentInfo();

    return 1;
}


BinID Provider::getCurrentPosition() const
{
    return currentbid_;
}


Coord Provider::getCurrentCoord() const
{
    return getCoord( currentbid_ );
}


Coord Provider::getCoord( const BinID& bid ) const
{
    Coord crd;
    if ( is2D() )
    {
	const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
	if ( geom )
	    crd = geom->toCoord( bid );
    }
    else
	crd = SI().transform( bid );

    return crd;
}


void Provider::updateCurrentInfo()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	inputs_[idx]->updateCurrentInfo();
	if ( currentbid_ != inputs_[idx]->getCurrentPosition() )
	    currentbid_ = inputs_[idx]->getCurrentPosition();
	if ( inputs_[idx]->getCurrentTrcInfo() &&
		curtrcinfo_ != inputs_[idx]->getCurrentTrcInfo() )
	    curtrcinfo_ = inputs_[idx]->getCurrentTrcInfo();
	if ( trcinfobid_ != inputs_[idx]->getTrcInfoBid() )
	    trcinfobid_ = inputs_[idx]->getTrcInfoBid();
    }
}


bool Provider::setCurrentPosition( const BinID& bid )
{
    if ( currentbid_.isUdf() )
	currentbid_ = bid;
    else if ( bid != currentbid_ )
    {
	if ( inputs_.isEmpty() && !desc_.isStored())
	    currentbid_ = bid;
	else
	    return false;
    }

    if ( linebuffer_ )
    {
	if ( doNotReUseDH() )
	{
	    linebuffer_->removeAllExcept( currentbid_ );
	    return true;
	}

	const BinID step = getStepoutStep();
	BinID dir = BinID(1,1);
	dir.inl() *= step.inl()/abs(step.inl());
	dir.crl() *= step.crl()/abs(step.crl());
	const BinID lastbid = currentbid_ - desbufferstepout_*step;
	linebuffer_->removeBefore(lastbid, dir);
    // in every direction...
    }

    return true;
}


void Provider::addLocalCompZIntervals( const TypeSet< Interval<int> >& intvs )
{
    const float dz = mIsZero(refstep_,mDefEps) ? SI().zStep() : refstep_;
    const Interval<int> possintv( mNINT32(possiblevolume_->zsamp_.start/dz),
				  mNINT32(possiblevolume_->zsamp_.stop/dz) );

    const int nrintvs = intvs.size();
    if ( nrintvs < 1 )
	{ pErrMsg("Huh"); return; }
    const int nrinps = inputs_.size();

    Array2DImpl< BasicInterval<int> > inputranges( nrinps<1?1:nrinps, nrintvs );
    for ( int idx=0; idx<nrintvs; idx++ )
    {
	BasicInterval<int> reqintv = intvs[idx];
	if ( reqintv.start > possintv.stop || reqintv.stop < possintv.start )
	{
	    for ( int inp=0; inp<nrinps; inp++ )
		inputranges.set( inp, idx, Interval<int>(mUdf(int),mUdf(int)) );
	    continue;
	}

	if ( possintv.start > reqintv.start )
	    reqintv.start = possintv.start;
	if ( possintv.stop < reqintv.stop )
	    reqintv.stop = possintv.stop;

	if ( !isUsedMultTimes() )
	    localcomputezintervals_ += reqintv;
	else
	{
	    if ( localcomputezintervals_.size()<=idx )
		localcomputezintervals_ += reqintv;
	    else
		localcomputezintervals_[idx].include(reqintv);
	}

	fillInputRangesArray( inputranges, idx, reqintv );
    }

    for ( int inp=0; inp<nrinps; inp++ )
    {
	if ( !inputs_[inp] )
	    continue;

	TypeSet<Interval<int> > inpranges;
	for ( int idx=0; idx<nrintvs; idx++ )
	{
	    const BasicInterval<int> rg = inputranges.get( inp, idx );
	    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
		continue;
	    inpranges += rg;
	}
	if ( inpranges.size() )
	    inputs_[inp]->addLocalCompZIntervals( inpranges );
    }
}


#define mUseMargins(type,Ts,ts)\
	const Interval<type>* req##ts = reqZ##Ts##Margin( inp, out );\
	if ( req##ts ) zrg##ts = *req##ts;\
	const Interval<type>* des##ts = desZ##Ts##Margin( inp, out );\
	if ( des##ts ) zrg##ts.include( *des##ts );\

void Provider::fillInputRangesArray(
				Array2DImpl< BasicInterval<int> >& inputranges,
				int idx, const BasicInterval<int>& reqintv )
{
    const int nrinps = inputs_.size();
    const float dz = mIsZero(refstep_,mDefEps) ? SI().zStep() : refstep_;
    for ( int out=0; out<outputinterest_.size(); out++ )
    {
	if ( !outputinterest_[out] ) continue;

	for ( int inp=0; inp<nrinps; inp++ )
	{
	    if ( !inputs_[inp] )
		continue;

	    BasicInterval<int> inputrange( reqintv );
	    BasicInterval<float> zrg( 0, 0 );
	    mUseMargins(float,,);

	    BasicInterval<int> zrgsamp( 0, 0 );
	    mUseMargins(int,Samp,samp);

	    inputrange.start += mNINT32(zrg.start/dz);
	    inputrange.start += zrgsamp.start;
	    inputrange.stop += mNINT32(zrg.stop/dz);
	    inputrange.stop += zrgsamp.stop;

	    inputranges.set( inp, idx, inputrange );
	}
    }
}


const TypeSet< Interval<int> >& Provider::localCompZIntervals() const
{
    return localcomputezintervals_;
}


void Provider::resetZIntervals()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->resetZIntervals();

    for ( int idx=localcomputezintervals_.size(); idx>0; idx-- )
	localcomputezintervals_.removeSingle(idx-1);
}


BinID Provider::getStepoutStep() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] || !inputs_[idx]->needStoredInput() ) continue;
	BinID bid = inputs_[idx]->getStepoutStep();
	if ( bid.inl()!=0 && bid.crl()!=0 ) return bid;
    }

    for ( int idx=0; idx<parents_.size(); idx++ )
    {
	if ( !parents_[idx] ) continue;
	BinID bid = parents_[idx]->getStepoutStep();
	if ( bid.inl()!=0 && bid.crl()!=0 ) return bid;
    }

    return BinID( SI().inlStep(), SI().crlStep() );
}



const DataHolder* Provider::getData( const BinID& relpos, int idi )
{
    if ( idi < 0 || idi >= localcomputezintervals_.size() )
	return 0;

    const DataHolder* constres = getDataDontCompute(relpos);
    Interval<int> loczinterval( localcomputezintervals_[idi] );
    if ( constres && constres->z0_ == loczinterval.start
	    && constres->nrsamples_ == loczinterval.width()+1 )
	return constres;

    if ( !linebuffer_ )
	linebuffer_ = new DataHolderLineBuffer;
    DataHolder* outdata =
        linebuffer_->createDataHolder( currentbid_+relpos, loczinterval.start,
				      loczinterval.width()+1 );
    if ( !outdata || !getInputData(relpos, idi) )
    {
	if ( outdata ) linebuffer_->removeDataHolder( currentbid_+relpos );
	return 0;
    }

    const int nrsamples = outdata->nrsamples_;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	while ( outdata->nrSeries()<=idx )
	    outdata->add( true );

	if ( outputinterest_[idx]<=0 )
	{
	    if ( outdata->series(idx) )
		outdata->replace( idx, 0 );

	    continue;
	}

	if ( !outdata->series(idx) )
	{
	    ValueSeries<float>* valptr = 0;
	    float dummy;
	    const BinDataDesc outputformat = getOutputFormat( idx );
	    if ( outputformat==BinDataDesc(dummy) )
	    {
		float* ptr = new float[nrsamples];
		valptr =
		    new ArrayValueSeries<float,float>( ptr, true, nrsamples );
	    }
	    else
		valptr = new ConvMemValueSeries<float>(
				nrsamples, outputformat, true, 0 );

	    if ( !valptr )
	    {
		errmsg_ = tr("Failed to allocate memory. "
			  "Probably the data you are loading is too big.");
		return 0;
	    }

	    valptr->setAll( mUdf(float) );
	    outdata->replace( idx, valptr );
	}
    }

    const int z0 = outdata->z0_;
    if ( needinterp_ )
	outdata->extrazfromsamppos_ = getExtraZFromSampInterval( z0, nrsamples);

    bool success = false;
    if ( !parallel_ || !allowParallelComputation() )
    {
	setNrThreads( 1 );
	success = computeData( *outdata, relpos, z0, nrsamples, 0 );
	success = finalizeCalculation( success );
    }
    else
    {
	if ( !providertask_ )
	    providertask_ = new ProviderTask( *this );

	providertask_->setVars( outdata, relpos, z0, nrsamples );

	success = providertask_->execute();
    }

    if ( !success )
    {
	linebuffer_->removeDataHolder( currentbid_+relpos );
	return 0;
    }

    return outdata;
}


const DataHolder* Provider::getDataDontCompute( const BinID& relpos ) const
{
    return linebuffer_ ? linebuffer_->getDataHolder(currentbid_+relpos) : 0;
}


SeisMSCProvider* Provider::getMSCProvider( bool& needmscprov ) const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	SeisMSCProvider* res = inputs_[idx]->getMSCProvider( needmscprov );
	if ( res ) return res;
    }

    return 0;
}


bool Provider::checkInpAndParsAtStart()
{
    return true;
}


uiString Provider::prepare( Desc& desc )
{
    if ( !desc.needProvInit() )
	return uiString::emptyString();

    desc.setNeedProvInit( false );

    uiString errmsg;
    RefMan<Provider> prov = PF().create( desc );
    if ( prov && prov->isOK() )
	return uiString::emptyString();

    errmsg = uiString::emptyString();
    if ( prov )
	errmsg = prov->errMsg();
    if ( errmsg.isEmpty() )
    {
	errmsg = tr("Cannot initialize '%1' Attribute properly")
		    .arg( desc.attribName() );
    }

    return errmsg;
}


void Provider::setOutputInterestSize( bool preserve )
{
    if ( preserve )
    {
	int outintsz = outputinterest_.size();
	if ( outintsz == desc_.nrOutputs() ) return;

	if ( outintsz < desc_.nrOutputs() )
	{
	    TypeSet<int> addon(desc_.nrOutputs()-outputinterest_.size(),0);
	    outputinterest_.append( addon );
	}
	else
	    outputinterest_.removeRange( desc_.nrOutputs()-1, outintsz-1 );
    }
    else
	outputinterest_ = TypeSet<int>(desc_.nrOutputs(),0);
}


void Provider::enableAllOutputs( bool yn )
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	enableOutput( idx, yn );
}


int Provider::getDataIndex( int input ) const
{
    return desc_.getInput(input) ? desc_.getInput(input)->selectedOutput() : -1;
}


bool Provider::getInputData( const BinID&, int )
{
    return true;
}


bool Provider::getInputOutput( int input, TypeSet<int>& res ) const
{
    res.erase();

    Desc* inputdesc = desc_.getInput(input);
    if ( !inputdesc ) return false;

    res += inputdesc->selectedOutput();
    return true;
}


void Provider::setInput( int inp, Provider* np )
{
    if ( inputs_[inp] )
    {
	if ( inputs_[inp]->desc_.isSteering() )
	    initSteering();

	TypeSet<int> inputoutputs;
	if ( getInputOutput( inp, inputoutputs ) )
	{
	    for ( int idx=0; idx<inputoutputs.size(); idx++ )
		inputs_[inp]->enableOutput( inputoutputs[idx], false );
	}
	inputs_[inp]->unRef();
    }

    inputs_.replace( inp, np );
    if ( !inputs_[inp] )
	return;

    inputs_[inp]->ref();
    if ( inputs_[inp]->desc_.isSteering() )
	initSteering();

    TypeSet<int> inputoutputs;
    if ( getInputOutput( inp, inputoutputs ) )
    {
	for ( int idx=0; idx<inputoutputs.size(); idx++ )
	    inputs_[inp]->enableOutput( inputoutputs[idx], true );
	inputs_[inp]->updateInputReqs(-1);
    }

    updateInputReqs(inp);
    inputs_[inp]->updateStorageReqs();
    if ( inputs_[inp]->desc_.isSteering() )
    {
	inputs_[inp]->updateInputReqs(-1);
	inputs_[inp]->updateStorageReqs( true );
    }
}

BinDataDesc Provider::getOutputFormat( int output ) const
{
    float dummy;
    return BinDataDesc( dummy );
}


void Provider::updateStorageReqs( bool all )
{
    if ( !all )
    {
	for ( int idx=0; idx<inputs_.size(); idx++ )
	    if ( inputs_[idx] ) inputs_[idx]->updateStorageReqs( all );
    }
}


void Provider::computeDesInputCube( int inp, int out, TrcKeyZSampling& res,
				    bool usestepout ) const
{
    //Be careful if usestepout=true with des and req stepouts
    if ( seldata_ && seldata_->type() == Seis::Table )
    {
	Interval<float> zrg(0,0);
	mUseMargins(float,,)

	Interval<int> zrgsamp(0,0);
	mUseMargins(int,Samp,samp)

	zrg.include( Interval<float>( zrgsamp.start*refstep_,
				      zrgsamp.stop*refstep_ ) );

	Interval<float> extraz = Interval<float>(extraz_.start + zrg.start,
						 extraz_.stop + zrg.stop);
	const_cast<Provider*>(inputs_[inp])->setSelData( seldata_ );
	const_cast<Provider*>(inputs_[inp])->setExtraZ( extraz );
    }

    if ( !desiredvolume_ )
	const_cast<Provider*>(this)->desiredvolume_ = new TrcKeyZSampling(res);

    res = *desiredvolume_;

    if ( usestepout )
    {
	int inlstepoutfact = desiredvolume_->hsamp_.step_.inl();
	int crlstepoutfact = desiredvolume_->hsamp_.step_.crl();

	BinID stepout(0,0);
	const BinID* reqstepout = reqStepout( inp, out );
	if ( reqstepout ) stepout=*reqstepout;
	const BinID* desstepout = desStepout( inp, out );
	if ( desstepout )
	{
	    if ( stepout.inl() < desstepout->inl() )
		stepout.inl() = desstepout->inl();
	    if ( stepout.crl() < desstepout->crl() )
		stepout.crl() = desstepout->crl();
	}

	res.hsamp_.start_.inl() -= stepout.inl() * inlstepoutfact;
	res.hsamp_.start_.crl() -= stepout.crl() * crlstepoutfact;
	res.hsamp_.stop_.inl() += stepout.inl() * inlstepoutfact;
	res.hsamp_.stop_.crl() += stepout.crl() * crlstepoutfact;
    }

    Interval<float> zrg(0,0);
    mUseMargins(float,,)

    Interval<int> zrgsamp(0,0);
    mUseMargins(int,Samp,samp)
    zrg.include(Interval<float>( zrgsamp.start*refstep_,
				 zrgsamp.stop*refstep_ ));

    res.zsamp_.start += zrg.start;
    res.zsamp_.stop += zrg.stop;
}


void Provider::updateInputReqs( int inp )
{
    if ( inp == -1 )
    {
	for ( int idx=0; idx<inputs_.size(); idx++ )
	    updateInputReqs(idx);
	return;
    }

    TrcKeyZSampling inputcs;
    for ( int out=0; out<outputinterest_.size(); out++ )
    {
	if ( !outputinterest_[out] || !inputs_[inp] ) continue;

	BinID stepout(0,0);
	const BinID* req = reqStepout(inp,out);
	if ( req ) stepout = *req;
	const BinID* des = desStepout(inp,out);
	if ( des )
	{
	    stepout.inl() = mMAX(stepout.inl(),des->inl());
	    stepout.crl() = mMAX(stepout.crl(),des->crl() );
	}

	inputs_[inp]->setReqBufStepout( ( req ? *req : BinID::noStepout() ) +
				       reqbufferstepout_, true );
	inputs_[inp]->setDesBufStepout( stepout+desbufferstepout_ );
    }
}


const BinID* Provider::desStepout(int,int) const		{ return 0; }
const BinID* Provider::reqStepout(int,int) const		{ return 0; }
const Interval<float>* Provider::desZMargin(int,int) const	{ return 0; }
const Interval<float>* Provider::reqZMargin(int,int) const	{ return 0; }
const Interval<int>* Provider::desZSampMargin(int,int) const	{ return 0; }
const Interval<int>* Provider::reqZSampMargin(int,int) const	{ return 0; }


int Provider::getTotalNrPos( bool is2d )
{
    if ( seldata_ && seldata_->type() == Seis::Table )
    {
	mDynamicCastGet(const Seis::TableSelData*,tsd,seldata_)
	return mCast( int, tsd->binidValueSet().totalSize() );
    }
    if ( !possiblevolume_ || !desiredvolume_ )
	return false;

    TrcKeyZSampling cs = *desiredvolume_;
    if ( getDesc().isStored() )
    {
	cs.hsamp_.start_.inl() =
	    desiredvolume_->hsamp_.start_.inl() < cs.hsamp_.start_.inl() ?
	    cs.hsamp_.start_.inl() : desiredvolume_->hsamp_.start_.inl();
	cs.hsamp_.stop_.inl() =
	    desiredvolume_->hsamp_.stop_.inl() > cs.hsamp_.stop_.inl() ?
	    cs.hsamp_.stop_.inl() : desiredvolume_->hsamp_.stop_.inl();
	cs.hsamp_.stop_.crl() =
	    desiredvolume_->hsamp_.stop_.crl() > cs.hsamp_.stop_.crl() ?
	    cs.hsamp_.stop_.crl() : desiredvolume_->hsamp_.stop_.crl();
	cs.hsamp_.start_.crl() =
	    desiredvolume_->hsamp_.start_.crl() < cs.hsamp_.start_.crl() ?
	    cs.hsamp_.start_.crl() : desiredvolume_->hsamp_.start_.crl();
    }

    if ( is2d )
    {
	const Pos::GeomID geomid = getGeomID();
	const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry );
	cs.hsamp_.step_.crl() = geom2d ? geom2d->data().trcNrRange().step : 1;
	return cs.nrCrl();
    }

    return cs.nrInl() * cs.nrCrl();
}


void Provider::computeRefStep()
{
    for( int idx=0; idx<allexistingprov_.size(); idx++ )
    {
	float step = 0;
	bool isstored = allexistingprov_[idx]->getZStepStoredData(step);
	if ( isstored )
	    refstep_ = ( refstep_ != 0 && refstep_ < step )? refstep_ : step;

    }
}


void Provider::setRefStep( float step )
{
    refstep_ = step;
    for ( int idx=0; idx<allexistingprov_.size(); idx++ )
	const_cast<Provider*>(allexistingprov_[idx])->refstep_ = refstep_;
}


void Provider::computeRefZ0()
{
    for( int idx=0; idx<allexistingprov_.size(); idx++ )
    {
	float z0 = 0;
	bool isstored = allexistingprov_[idx]->getZ0StoredData(z0);
	if ( isstored )
	    refz0_ = ( refz0_ < z0 )? refz0_ : z0;
    }
}


void Provider::setRefZ0( float z0 )
{
    refz0_ = z0;
    for ( int idx=0; idx<allexistingprov_.size(); idx++ )
	const_cast<Provider*>(allexistingprov_[idx])->refz0_ = refz0_;
}


void Provider::setCurLineName( const char* linename )
{
    geomid_ = Survey::GM().getGeomID( linename );
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	inputs_[idx]->setCurLineName( linename );
    }
}


void Provider::adjust2DLineStoredVolume()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->adjust2DLineStoredVolume();
}


Pos::GeomID  Provider::getGeomID() const
{
    if ( geomid_ != Survey::GM().cUndefGeomID() )
	return geomid_;

    Pos::GeomID geomid = Survey::GM().cUndefGeomID();
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
        if ( !inputs_[idx] )
            continue;

        geomid = inputs_[idx]->getGeomID();
	if ( !Values::isUdf(geomid) )
            return geomid;
    }

    return geomid;
}


void Provider::setGeomID( Pos::GeomID geomid )
{
    geomid_ = geomid;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->setGeomID( geomid );
    }
}


void Provider::setSelData( const Seis::SelData* seldata )
{
    seldata_ = seldata;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->setSelData(seldata_);
    }
}


void Provider::setExtraZ( const Interval<float>& extraz )
{
    extraz_.include( extraz );
}


void Provider::setPossibleVolume( const TrcKeyZSampling& cs )
{
    if ( possiblevolume_ )
	delete possiblevolume_;

    possiblevolume_ = new TrcKeyZSampling(cs);
}


float Provider::getRefStep() const
{
    return !mIsZero(refstep_,mDefEps) ? refstep_ : SI().zStep();
}


bool Provider::zIsTime() const
{ return SI().zIsTime(); }

float Provider::inlDist() const
{ return SI().inlDistance(); }

float Provider::crlDist() const
{ return SI().crlDistance(); }

float Provider::lineDist() const
{ return SI().inlDistance(); }

float Provider::trcDist() const
{
    return is2D() && useInterTrcDist() ?
     getDistBetwTrcs(false, Survey::GM().getName(geomid_)) : SI().crlDistance();
}

uiString Provider::errMsg() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] && !inputs_[idx]->errMsg().isEmpty() )
	    return inputs_[idx]->errMsg();
    }

    return errmsg_;
}


void Provider::setUsedMultTimes()
{
    isusedmulttimes_ = true;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->setUsedMultTimes();
    }
}


void Provider::setNeedInterpol( bool yn )
{
    needinterp_ = yn;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->setNeedInterpol( yn );
    }
}


void Provider::resetDesiredVolume()
{
    if ( desiredvolume_ )
    {
	delete desiredvolume_;
	desiredvolume_ = 0;
    }
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->resetDesiredVolume();
    }
}


float Provider::getInterpolInputValue( const DataHolder& input, int inputidx,
				       float zval ) const
{
    ValueSeriesInterpolator<float> interp( input.nrsamples_-1 );
    const float samplepos = (zval/getRefStep()) - input.z0_
			    - input.extrazfromsamppos_/getRefStep();
    return interp.value( *input.series(inputidx), samplepos );
}


float Provider::getInterpolInputValue( const DataHolder& input, int inputidx,
				       float sampleidx, int z0 ) const
{
    ValueSeriesInterpolator<float> interp( input.nrsamples_-1 );
    const float samplepos = float(z0-input.z0_) + sampleidx
			    - input.extrazfromsamppos_/getRefStep();
    return interp.value( *input.series(inputidx), samplepos );
}


float Provider::getInputValue( const DataHolder& input, int inputidx,
			       int sampleidx, int z0 ) const
{
    if ( inputidx<0 || inputidx>=input.nrSeries() )
	return mUdf(float);

    float extraz = 0;
    if ( needinterp_ )
    {
	int intvidx = 0;
	for ( int idx=0; idx<localcomputezintervals_.size(); idx++ )
	    if ( localcomputezintervals_[idx].includes(z0,true) )
		intvidx = idx;

	if ( exactz_.validIdx(intvidx) )
	{
	    float exacttime = exactz_[intvidx];
	    extraz = getExtraZFromSampPos( exacttime );
	}
    }

    if ( needinterp_ && !mIsEqual(extraz,input.extrazfromsamppos_,mDefEps) )
	return getInterpolInputValue( input, inputidx,
				   (float)sampleidx + extraz/getRefStep(), z0 );
    else
    {
	const int sidx = z0 - input.z0_ + sampleidx;
	return input.series(inputidx) && sidx>=0 && sidx<input.nrsamples_
		    ? input.series(inputidx)->value( sidx ) : mUdf(float);
    }
}


void Provider::setOutputValue( const DataHolder& output, int outputidx,
			       int sampleidx, int z0, float val ) const
{
    if ( !isOutputEnabled(outputidx) )
	return;

    const int sidx = z0 - output.z0_ + sampleidx;
    if ( sidx<0 || sidx>output.nrsamples_ )
	return;

    output.series(outputidx)->setValue( sidx, val );
}


void Provider::prepareForComputeData()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] ) inputs_[idx]->prepareForComputeData();
}


void Provider::prepPriorToBoundsCalc()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] ) inputs_[idx]->prepPriorToBoundsCalc();
}


bool Provider::prepPriorToOutputSetup()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] ) inputs_[idx]->prepPriorToOutputSetup();

    return false;
}


void Provider::setExactZ( const TypeSet<float>& exactz )
{
    exactz_ = exactz;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setExactZ( exactz );
}


void Provider::getCompNames( BufferStringSet& nms ) const
{
    nms.erase();
    nms.add( desc_.attribName() );
}

void Provider::getCompOutputIDs( TypeSet<int>& ids ) const
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	ids += idx;
}


float Provider::getDistBetwTrcs( bool ismax, const char* linenm ) const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	const float distval = inputs_[idx]->getDistBetwTrcs( ismax, linenm );
	if ( !mIsUdf(distval) )
	    return distval;
    }

    return mUdf(float);
}


bool Provider::compDistBetwTrcsStats( bool force )
{
    bool allright = false;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] && inputs_[idx]->compDistBetwTrcsStats() )
	    allright = true;

    return allright;
}


//Cannot make it a virtual function in 6.0
BinID Provider::getElementStepout() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;
	mDynamicCastGet( StorageProvider*, inpatidx,
			 const_cast<Attrib::Provider*>(this)->inputs_[idx] );
	if ( !inpatidx )
	    return inputs_[idx]->getElementStepout();

	return inpatidx->getElementStepoutStoredSpecial();
    }

    return BinID(1,1);
}


bool Provider::needStoredInput() const
{
    bool needinput = false;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] && inputs_[idx]->needStoredInput() )
	{
	    needinput = true;
	    break;
	}
    }

    return needinput;
}


void Provider::setRdmPaths( const TypeSet<BinID>& truepath,
			    const TypeSet<BinID>& snappedpath )
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] )
	    inputs_[idx]->setRdmPaths( truepath, snappedpath );
    }
}


float Provider::getExtraZFromSampPos( float exacttime ) const
{
    return DataHolder::getExtraZFromSampPos( exacttime, getRefStep() );
}


float Provider::getExtraZFromSampInterval( int z0, int nrsamples ) const
{
    int intvidx = -1;
    for ( int idx=0; idx<localcomputezintervals_.size(); idx++ )
    {
	 if ( localcomputezintervals_[idx].includes( z0,true ) &&
	      localcomputezintervals_[idx].includes( z0+nrsamples-1,true ) )
	 {
	     intvidx = idx;
	     break;
	 }
    }

    return ( intvidx>=0 && exactz_.size()>intvidx )
		? getExtraZFromSampPos( exactz_[intvidx] ) : 0;
}


void Provider::stdPrepSteering( const BinID& so )
{
    for( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] && inputs_[idx]->getDesc().isSteering() )
	    inputs_[idx]->prepSteeringForStepout( so );
    }
}


float Provider::zFactor() const
{
    return (float) ( zIsTime() ?  ZDomain::Time()
			       : ZDomain::Depth() ).userFactor();
}


float Provider::dipFactor() const
{ return zIsTime() ? 1e6f: 1e3f; }


bool Provider::useInterTrcDist() const
{
    if ( inputs_.size() && inputs_[0] && inputs_[0]->getDesc().isStored() )
	return inputs_[0]->useInterTrcDist();

    return false;
}


float Provider::getApplicableCrlDist( bool dependoninput ) const
{
    if ( is2D() && ( !dependoninput || useInterTrcDist() ) )
	return getDistBetwTrcs( false, Survey::GM().getName(geomid_) );

    return crlDist();
}


void Provider::setDataUnavailableFlag( bool yn )
{ dataunavailableflag_ = yn; }


bool Provider::getDataUnavailableFlag() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx] && inputs_[idx]->getDataUnavailableFlag() )
	    return true;
    }

    return dataunavailableflag_;
}


} // namespace Attrib
