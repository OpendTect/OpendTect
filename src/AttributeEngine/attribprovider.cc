/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
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

#include "binnedvalueset.h"
#include "convmemvalseries.h"
#include "cubesubsel.h"
#include "linesubsel.h"
#include "trckeyzsampling.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seismscprov.h"
#include "seisinfo.h"
#include "seistableseldata.h"
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


int minThreadSize() const { return provider_.minTaskSize(); }


void setVars( const DataHolder* res, const BinID& relpos, int z0,
	      int nrsamples )
{
    res_ = res;
    relpos_ = relpos;
    z0_ = z0;
    nrsamples_ = nrsamples;
}


od_int64 nrIterations() const { return nrsamples_; }


bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    if ( !res_ ) return true;
    return provider_.computeData( *res_, relpos_, mCast(int,start+z0_),
				  mCast(int,stop-start+1), threadid );
}


bool doPrepare( int nrthreads )
{ return provider_.setNrThreads( nrthreads ); }


bool doFinish( bool success )
{ return provider_.finalizeCalculation( success ); }

protected:

    Provider&			provider_;
    const DataHolder*		res_;
    BinID			relpos_;
    int				z0_;
    int				nrsamples_;
};


Provider* Provider::create( Desc& desc, uiRetVal& uirv )
{
    ObjectSet<Provider> existing;
    bool issame = false;
    Provider* prov = internalCreate( desc, existing, issame, uirv );
    if ( !prov ) return 0;

    prov->allexistingprov_ = existing;
    return prov;
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing,
				    bool& issame, uiRetVal& uirv )
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

    Provider* newprov = 0;
    if ( !desc.descSet() )
	{ uirv = tr("No attribute set specified"); return nullptr; }
    else if ( desc.nrInputs() < 1 )
    {
	if ( !desc.isStored() )
	    { uirv = mINTERNAL("All non-stored have inputs"); return nullptr; }
	newprov = new StorageProvider( desc );
    }
    else
    {
	newprov = PF().create( desc, uirv );
	if ( !newprov || !uirv.isOK() )
	{
	    if ( !newprov )
		uirv = mINTERNAL("Add error message");
	    else
		delete newprov;
	    return nullptr;
	}
    }

    newprov->ref();

    if ( desc.selectedOutput()!=-1 && existing.isEmpty() )
	newprov->enableOutput( desc.selectedOutput(), true );

    existing += newprov;

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	Desc* inputdesc = desc.getInput(idx);
	if ( !inputdesc )
	    continue;

	Provider* inputprovider =
			internalCreate( *inputdesc, existing, issame, uirv );
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
	    uirv = tr("%1: One of the inputs depends on itself")
			.arg( desc.userRef() );
	    return 0;
	}

	newprov->setInput( idx, inputprovider );
	inputprovider->addParent(newprov);
	issame = false;
    }

    if ( !newprov->checkInpAndParsAtStart() )
    {
	existing.removeRange( existing.indexOf(newprov), existing.size()-1 );
	uiRetVal provuirv = newprov->uirv_;
	if ( provuirv.isOK() )
	    { pFreeFnErrMsg("No error message. Hunt this down and add one."); }
	BufferString attribnm = newprov->desc_.attribName();
	if ( attribnm == StorageProvider::attribName() )
	    uirv = tr("Cannot load Stored Cube '%1'")
		     .arg( newprov->desc_.userRef() );
	else
	    uirv.set( tr("Attribute \"%1\" (%2) cannot be initialized")
		     .arg( newprov->desc_.userRef() ).arg( attribnm ) );
	uirv.add( provuirv );
	newprov->unRef();
	return 0;
    }

    newprov->unRefNoDelete();
    return newprov;
}


Provider::Provider( Desc& nd )
    : desc_( nd )
    , outputinterest_( nd.nrOutputs(), 0 )
    , reqbufferstepout_( 0, 0 )
    , desbufferstepout_( 0, 0 )
    , providertask_( 0 )
    , currentbid_( -1, -1 )
    , linebuffer_( 0 )
    , refzstep_( 0 )
    , alreadymoved_( 0 )
    , seldata_( 0 )
    , curtrcinfo_( 0 )
    , extraz_( 0, 0 )
    , trcinfobid_( -1, -1 )
    , prevtrcnr_( 0 )
    , refz0_( 0 )
    , useshortcuts_( false )
    , needinterp_( false )
    , isusedmulttimes_( false )
    , dataunavailableflag_( false )
{
    possiblesubsel_.setToAll( desc_.is2D() );
    desiredsubsel_ = possiblesubsel_;
    desc_.ref();
    inputs_.setNullAllowed( true );
    for ( int idx=0; idx<desc_.nrInputs(); idx++ )
	inputs_ += 0;
}


Provider::~Provider()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->unRef();
    inputs_.erase();

    allexistingprov_.erase();
    desc_.unRef();
    delete providertask_;
    delete linebuffer_;
}


bool Provider::is2D() const
{
    return desc_.descSet() ? desc_.descSet()->is2D() : desc_.is2D();
}


bool Provider::isSingleTrace() const
{
    return desc_.isSingleTrace();
}


bool Provider::usesTracePosition() const
{
    return desc_.usesTracePosition();
}


void Provider::enableOutput( int outnr, bool yn )
{
    const int nrouts = outputinterest_.size();
    if ( outnr >= nrouts )
	{ pErrMsg( "Huh?" ); return; }

    const int startiout = outnr >= 0 ? outnr : 0;
    const int stopiout = outnr >= 0 ? outnr+1 : nrouts;
    for ( int iout=startiout; iout<stopiout; iout++ )
    {
	if ( yn )
	    outputinterest_[iout]++;
	else
	{
	    if ( outputinterest_[iout] < 1 )
		{ pErrMsg( "Huh2?"); return; }
	    outputinterest_[iout]--;
	}
    }
}


bool Provider::isOutputEnabled( int outnr ) const
{
    if ( outputinterest_.isEmpty() || outnr >= outputinterest_.size() )
	{ pErrMsg( "Huh?" ); return false; }
    else
    {
	if ( outnr < 0 )
	    outnr = 0;
	return outputinterest_[outnr];
    }
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


void Provider::setDesiredSubSel( const GeomSubSel& gss )
{
    if ( gss.is2D() != is2D() )
	{ pErrMsg( "Bad 2D vs 3D" ); }
    else
	setDesiredSubSel( FullSubSel(gss) );
}


void Provider::setPossibleSubSel( const FullSubSel& reqnewss )
{
    if ( reqnewss.is2D() == is2D() )
	possiblesubsel_ = reqnewss;
    else
    {
	pErrMsg( "Bad 2D vs 3D" );
	possiblesubsel_.setToAll( is2D() );
    }
}


void Provider::setDesiredSubSel( const FullSubSel& reqnewss )
{
    const FullSubSel* newss = &reqnewss;
    if ( newss->is2D() != is2D() )
    {
	pErrMsg( "Bad 2D vs 3D" );
	desiredsubsel_.setToAll( is2D() );
	newss = &desiredsubsel_;
    }

    if ( !isUsedMultTimes() )
	desiredsubsel_ = *newss;
    else
	desiredsubsel_.merge( *newss );

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;
	for ( int idy=0; idy<outputinterest_.size(); idy++ )
	{
	    if ( outputinterest_[idy]<1 || !inputs_[idx] )
		continue;

	    const bool isstored = inputs_[idx]->desc_.isStored();
	    FullSubSel fss;
	    computeDesInputSubSel( idx, idy, fss, !isstored );
	    inputs_[idx]->setDesiredSubSel( fss );
	}
    }
}


void Provider::applyMargins( const Interval<float>* zmargin,
		 const Interval<int>* zmargininsamps, FullSubSel& fss ) const
{
    Interval<float> relzrg( 0.f, 0.f );
    if ( zmargin )
    {
	relzrg = *zmargin;
	if ( zmargininsamps )
	    relzrg.include( zmargininsamps->start*refzstep_,
			    zmargininsamps->stop*refzstep_ );
    }
    else if ( zmargininsamps )
	relzrg = Interval<float>( zmargininsamps->start*refzstep_,
				  zmargininsamps->stop*refzstep_ );

    if ( zmargin || zmargininsamps )
    {
	const auto nrgeomids = fss.nrGeomIDs();
	for ( int igid=0; igid<nrgeomids; igid++ )
	{
	    auto zrg = fss.zRange( igid );
	    zrg.start -= relzrg.start;
	    zrg.stop -= relzrg.stop;
	    fss.setZRange( zrg, igid );
	}
    }
}


bool Provider::calcPossibleSubSel( int output, const FullSubSel& desss )
{
    desiredsubsel_ = desss;
    possiblesubsel_.setToAll( is2D() );
    if ( inputs_.isEmpty() )
	{ possiblesubsel_ = desss; return true; }

    TypeSet<int> outputs;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
	if ( output < 0 || outputinterest_[idx] > 0 )
	    outputs += idx;

    bool isset = false;
    bool havesetpss = false;
    for ( const auto out : outputs )
    {
	for ( int iinp=0; iinp<inputs_.size(); iinp++ )
	{
	    const auto* inpprov = inputs_[iinp];
	    if ( !inpprov )
		continue;
	    TypeSet<int> inputoutput;
	    if ( !getInputOutput( iinp, inputoutput ) )
		continue;

	    for ( int ioutinp=0; ioutinp<inputoutput.size(); ioutinp++ )
	    {
		FullSubSel inpfss;
		computeDesInputSubSel( iinp, out, inpfss, true );
		if ( !inputs_[iinp]->calcPossibleSubSel( ioutinp, inpfss ) )
		    continue;

		const BinID* stepout = reqStepout(iinp,out);
		if ( stepout )
		{
		    if ( inpfss.is2D() )
			inpfss.subSel2D().addStepout( -stepout->crl() );
		    else
			inpfss.subSel3D().addStepout( -stepout->inl(),
						      -stepout->crl() );
		}

		applyMargins( reqZMargin(iinp,out), reqZSampMargin(iinp,out),
			      inpfss );
		if ( havesetpss )
		    possiblesubsel_.limitTo( inpfss );
		else
		    possiblesubsel_ = inpfss;
		isset = true;
	    }
	}
    }

    return isset;
}


static int subSelExtreme( const Survey::FullSubSel& ss, bool min )
{
    return min ? ss.trcNrRange().start : ss.trcNrRange().stop;
}


int Provider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    BinID pos = startpos;

    if ( inputs_.isEmpty() )
    {
	pos = BinID(-1,-1);
	if ( seldata_ && seldata_->type() == Seis::Table )
	{
	    auto& bvs = seldata_->asTable()->binidValueSet();
	    if ( currentbid_ == BinID(-1,-1) )
		pos = bvs.firstBinID();
	    else
	    {
		BinnedValueSet::SPos oldpos = bvs.find( currentbid_ );
		if ( bvs.next(oldpos,true) )
		    pos = bvs.getBinID( oldpos );
	    }
	    currentbid_ = pos;
	    alreadymoved_ = true;
	    return currentbid_ == BinID(-1,-1) ? 0 : 1;
	}
    }

    bool docheck = pos == BinID(-1,-1);

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
	    if ( !inputs_[idx] )
		continue;

	    currentbid_ = inputs_[idx]->getCurrentPosition();
	    trcinfobid_ = inputs_[idx]->getTrcInfoBid();
	    if ( !docheck && currentbid_ == pos )
		continue;
	    if ( !docheck && trcinfobid_ != BinID(-1,-1) && trcinfobid_ == pos )
		continue;

	    needmove = true;
	    const int res = inputs_[idx]->moveToNextTrace(pos, firstcheck);
	    if ( res!=1 )
		return res;

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
		     && inputs_[idi]->getTrcInfoBid() != BinID(-1,-1)
		     && inputs_[idi]->getTrcInfoBid() != pos )
		{
		    allok = false;
		    break;
		}
	    }

	    if ( !allok )
	    {
		BinID newstart( BinID(-1,-1) );
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
	    if ( currentbid_.inl() == -1 && currentbid_.crl() == -1 )
	    {
		if ( desiredsubsel_.is2D() )
		    currentbid_.crl() = subSelExtreme( desiredsubsel_, true );
		else
		    currentbid_ = desiredsubsel_.cubeSubSel().origin();
	    }
	    else
	    {
		BinID prevbid = currentbid_;
		BinID step = getStepoutStep();
		if ( prevbid.crl() +step.crl() <=
		     subSelExtreme(desiredsubsel_,false) )
		    currentbid_.crl() = prevbid.crl() +step.crl();
		else if ( !desiredsubsel_.is2D()
			&& prevbid.inl()+step.inl() <=
			  desiredsubsel_.cubeSubSel().inlSubSel().posStop() )
		{
		    currentbid_.inl() = prevbid.inl() + step.inl();
		    currentbid_.crl() = subSelExtreme( desiredsubsel_, true );
		}
		else
		    return 0;
	    }
	}
	else if ( needmove )
	    currentbid_ = BinID(-1,-1);

	setCurrentPosition(currentbid_);
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
	    if ( !inputs_[idx] )
		continue;
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
	BinID inputbid(BinID(-1,-1));
	if ( inputs_[idi] && inputs_[idi]->getTrcInfoBid() != BinID(-1,-1) )
	    inputbid = inputs_[idi]->getCurrentPosition();

	if ( inputbid == BinID(-1,-1) ) continue;
	if ( newstart == BinID(-1,-1) )
	{
	    newstart = inputbid;
	}
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
	Seis::MSCProvider* seismscprov1 = input1->getMSCProvider( needmscp1 );
	Seis::MSCProvider* seismscprov2 = input2->getMSCProvider( needmscp2 );
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


void Provider::updateCurrentInfo()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;
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
    if ( currentbid_ == BinID(-1,-1) )
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
    const float dz = mIsZero(refzstep_,mDefEps) ? SI().zStep() : refzstep_;
    const Interval<int> possintv( mNINT32(possiblesubsel_.zRange().start/dz),
				  mNINT32(possiblesubsel_.zRange().stop/dz) );

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
    const float dz = mIsZero(refzstep_,mDefEps) ? SI().zStep() : refzstep_;
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
	if ( !inputs_[idx] || !inputs_[idx]->needStoredInput() )
	    continue;
	BinID bid = inputs_[idx]->getStepoutStep();
	if ( bid.inl()!=0 && bid.crl()!=0 )
	    return bid;
    }

    for ( int idx=0; idx<parents_.size(); idx++ )
    {
	if ( !parents_[idx] )
	    continue;
	BinID bid = parents_[idx]->getStepoutStep();
	if ( bid.inl()!=0 && bid.crl()!=0 )
	    return bid;
    }

    return BinID( SI().inlStep(), SI().crlStep() );
}



const DataHolder* Provider::getData( const BinID& relpos, int idi )
{
    if ( idi < 0 || idi >= localcomputezintervals_.size() )
	return nullptr;

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
	if ( outdata )
	    linebuffer_->removeDataHolder( currentbid_+relpos );
	return nullptr;
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
		valptr =
		    new ConvMemValueSeries<float>( nrsamples, outputformat, 0 );

	    if ( !valptr )
		return nullptr;

	    valptr->setAll( mUdf(float) );
	    outdata->replace( idx, valptr );
	}
    }

    const int z0 = outdata->z0_;
    if ( needinterp_ )
	outdata->extrazfromsamppos_ = getExtraZFromSampInterval( z0, nrsamples);

    bool success = false;
    if ( !allowParallelComputation() )
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
	return nullptr;
    }

    return outdata;
}


const DataHolder* Provider::getDataDontCompute( const BinID& relpos ) const
{
    return linebuffer_ ? linebuffer_->getDataHolder(currentbid_+relpos) : 0;
}


Seis::MSCProvider* Provider::getMSCProvider( bool& needmscprov ) const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;
	Seis::MSCProvider* res = inputs_[idx]->getMSCProvider( needmscprov );
	if ( res )
	    return res;
    }

    return 0;
}


bool Provider::checkInpAndParsAtStart()
{
    return true;
}


uiRetVal Provider::prepare( Desc& desc )
{
    uiRetVal uirv;
    if ( !desc.needProvInit() )
	return uirv;

    desc.setNeedProvInit( false );
    RefMan<Provider> prov = PF().create( desc, uirv );
    if ( prov )
	return uirv;

    if ( uirv.isOK() )
	uirv = tr("Cannot initialize '%1' Attribute properly")
		    .arg( desc.attribName() );

    return uirv;
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
    if ( !inputdesc )
	return false;

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
	    if ( inputs_[idx] )
		inputs_[idx]->updateStorageReqs( all );
    }
}


void Provider::computeDesInputSubSel( int inp, int out, FullSubSel& fss,
					bool usestepout ) const
{
    //Be careful if usestepout=true with des and req stepouts
    if ( seldata_ && seldata_->type() == Seis::Table )
    {
	Interval<float> zrg(0,0);
	mUseMargins(float,,)

	Interval<int> zrgsamp(0,0);
	mUseMargins(int,Samp,samp)

	zrg.include( Interval<float>( zrgsamp.start*refzstep_,
				      zrgsamp.stop*refzstep_ ) );

	Interval<float> extraz = Interval<float>(extraz_.start + zrg.start,
						 extraz_.stop + zrg.stop);
	const_cast<Provider*>(inputs_[inp])->setSelData( seldata_ );
	const_cast<Provider*>(inputs_[inp])->setExtraZ( extraz );
    }

    fss = desiredsubsel_;

    if ( usestepout )
    {
	BinID stepout(0,0);
	const BinID* reqstepout = reqStepout( inp, out );
	if ( reqstepout )
	    stepout = *reqstepout;
	const BinID* desstepout = desStepout( inp, out );
	if ( desstepout )
	{
	    if ( stepout.inl() < desstepout->inl() )
		stepout.inl() = desstepout->inl();
	    if ( stepout.crl() < desstepout->crl() )
		stepout.crl() = desstepout->crl();
	}
	if ( is2D() )
	    fss.subSel2D().addStepout( stepout.crl() );
	else
	    fss.subSel3D().addStepout( stepout.inl(), stepout.crl() );
    }

    applyMargins( reqZMargin(inp,out), reqZSampMargin(inp,out), fss );
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
	if ( !outputinterest_[out] || !inputs_[inp] )
	    continue;

	BinID stepout(0,0);
	const BinID* req = reqStepout(inp,out);
	if ( req )
	    stepout = *req;
	const BinID* des = desStepout(inp,out);
	if ( des )
	{
	    stepout.inl() = mMAX(stepout.inl(),des->inl());
	    stepout.crl() = mMAX(stepout.crl(),des->crl() );
	}

	inputs_[inp]->setReqBufStepout( ( req ? *req : BinID(0,0) ) +
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


int Provider::totalNrPos()
{
    if ( seldata_ && seldata_->type() == Seis::Table )
	return (int)seldata_->asTable()->binidValueSet().totalSize();

    FullSubSel fss( desiredsubsel_ );
    fss.limitTo( possiblesubsel_ );

    return (int)fss.horSubSel().totalSize();
}


void Provider::computeRefZStep()
{
    for( int idx=0; idx<allexistingprov_.size(); idx++ )
    {
	float step = allexistingprov_[idx]->zStepStoredData();
	if ( !mIsUdf(step) && step > 0 )
	    refzstep_ = refzstep_ != 0 && refzstep_ < step ? refzstep_ : step;
    }
}


void Provider::setRefZStep( float step )
{
    refzstep_ = step;
    for ( int idx=0; idx<allexistingprov_.size(); idx++ )
	const_cast<Provider*>(allexistingprov_[idx])->refzstep_ = refzstep_;
}


void Provider::computeRefZ0()
{
    for( int idx=0; idx<allexistingprov_.size(); idx++ )
    {
	float z0 = allexistingprov_[idx]->z0StoredData();
	if ( !mIsUdf(z0) )
	    refz0_ = refz0_ < z0 ? refz0_ : z0;
    }
}


void Provider::setRefZ0( float z0 )
{
    refz0_ = z0;
    for ( int idx=0; idx<allexistingprov_.size(); idx++ )
	const_cast<Provider*>(allexistingprov_[idx])->refz0_ = refz0_;
}


void Provider::setGeomID( GeomID geomid )
{
    desiredsubsel_.setGeomID( geomid );
    possiblesubsel_.setGeomID( geomid );

    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setGeomID( geomid );
}


void Provider::adjust2DLineStoredSubSel()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->adjust2DLineStoredSubSel();
}


Pos::GeomID Provider::geomID() const
{
    GeomID geomid( possiblesubsel_.geomID(0) );
    if ( !geomid.isValid() )
    {
	for ( int idx=0; idx<inputs_.size(); idx++ )
	{
	    if ( !inputs_[idx] )
		continue;

	    geomid = inputs_[idx]->geomID();
	    if ( geomid.isValid() )
		break;
	}
    }
    return geomid;
}


void Provider::setSelData( const Seis::SelData* seldata )
{
    seldata_ = seldata;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setSelData(seldata_);
}


void Provider::setExtraZ( const Interval<float>& extraz )
{
    extraz_.include( extraz );
}


float Provider::refZStep() const
{
    return !mIsZero(refzstep_,mDefEps) ? refzstep_ : SI().zStep();
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
    return is2D() && useInterTrcDist() ?  getDistBetwTrcs(false)
				       : SI().crlDistance();
}


uiRetVal Provider::errMsg() const
{
    uiRetVal uirv( uirv_ );

    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] && !inputs_[idx]->errMsg().isEmpty() )
	    uirv.add( inputs_[idx]->errMsg() );

    return uirv;
}


void Provider::setUsedMultTimes()
{
    isusedmulttimes_ = true;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setUsedMultTimes();
}


void Provider::setNeedInterpol( bool yn )
{
    needinterp_ = yn;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setNeedInterpol( yn );
}


void Provider::resetDesiredSubSel()
{
    desiredsubsel_.setToAll( is2D() );
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->resetDesiredSubSel();
}


float Provider::getInterpolInputValue( const DataHolder& input, int inputidx,
				       float zval ) const
{
    ValueSeriesInterpolator<float> interp( input.nrsamples_-1 );
    const float samplepos = (zval/refZStep()) - input.z0_
			    - input.extrazfromsamppos_/refZStep();
    return interp.value( *input.series(inputidx), samplepos );
}


float Provider::getInterpolInputValue( const DataHolder& input, int inputidx,
				       float sampleidx, int z0 ) const
{
    ValueSeriesInterpolator<float> interp( input.nrsamples_-1 );
    const float samplepos = float(z0-input.z0_) + sampleidx
			    - input.extrazfromsamppos_/refZStep();
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
				   (float)sampleidx + extraz/refZStep(), z0 );
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
	if ( inputs_[idx] )
	    inputs_[idx]->prepareForComputeData();
}


void Provider::prepPriorToBoundsCalc()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->prepPriorToBoundsCalc();
}


bool Provider::prepPriorToOutputSetup()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->prepPriorToOutputSetup();

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


float Provider::getDistBetwTrcs( bool ismax ) const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;
	const float distval = inputs_[idx]->getDistBetwTrcs( ismax );
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


bool Provider::needStoredInput() const
{
    bool needinput = false;
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] && inputs_[idx]->needStoredInput() )
	    { needinput = true; break; }

    return needinput;
}


void Provider::setRdmPaths( const TypeSet<BinID>& truepath,
			    const TypeSet<BinID>& snappedpath )
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] )
	    inputs_[idx]->setRdmPaths( truepath, snappedpath );
}


float Provider::getExtraZFromSampPos( float exacttime ) const
{
    return DataHolder::getExtraZFromSampPos( exacttime, refZStep() );
}


float Provider::getExtraZFromSampInterval( int z0, int nrsamples ) const
{
    int intvidx = -1;
    for ( int idx=0; idx<localcomputezintervals_.size(); idx++ )
	 if ( localcomputezintervals_[idx].includes( z0,true ) &&
	      localcomputezintervals_[idx].includes( z0+nrsamples-1,true ) )
	     { intvidx = idx; break; }

    return ( intvidx>=0 && exactz_.size()>intvidx )
		? getExtraZFromSampPos( exactz_[intvidx] ) : 0;
}


void Provider::stdPrepSteering( const BinID& so )
{
    for( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] && inputs_[idx]->desc_.isSteering() )
	    inputs_[idx]->prepSteeringForStepout( so );
}


float Provider::zFactor() const
{
    return (float)(zIsTime() ? ZDomain::Time()
			     : ZDomain::Depth() ).userFactor();
}


float Provider::dipFactor() const
{ return zIsTime() ? 1e6f: 1e3f; }


bool Provider::useInterTrcDist() const
{
    if ( inputs_.size() && inputs_[0] && inputs_[0]->desc_.isStored() )
	return inputs_[0]->useInterTrcDist();

    return false;
}


float Provider::getApplicableCrlDist( bool dependoninput ) const
{
    if ( is2D() && ( !dependoninput || useInterTrcDist() ) )
	return getDistBetwTrcs( false );

    return crlDist();
}


void Provider::setDataUnavailableFlag( bool yn ) const
{
    mNonConst(dataunavailableflag_) = yn;
}


bool Provider::getDataUnavailableFlag() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	if ( inputs_[idx] && inputs_[idx]->getDataUnavailableFlag() )
	    return true;

    return dataunavailableflag_;
}


} // namespace Attrib
