/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribprovider.cc,v 1.121 2009-11-18 21:58:40 cvskris Exp $";

#include "attribprovider.h"
#include "attribstorprovider.h"

#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attriblinebuffer.h"
#include "attribparam.h"
#include "task.h"
#include "cubesampling.h"
#include "errh.h"
#include "seismscprov.h"
#include "seisinfo.h"
#include "seisselectionimpl.h"
#include "survinfo.h"
#include "valseriesinterpol.h"
#include "convmemvalseries.h"
#include "binidvalset.h"


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
    return provider_.computeData( *res_, relpos_, start+z0_, stop-start+1,
	    			  threadid );
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


Provider* Provider::create( Desc& desc, BufferString& errstr )
{
    ObjectSet<Provider> existing;
    bool issame = false;
    Provider* prov = internalCreate( desc, existing, issame, errstr );
    if ( !prov ) return 0;

    prov->allexistingprov = existing;
    return prov;
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing, 
				    bool& issame, BufferString& errstr )
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
	errstr = "No attribute set specified";
	return 0;
    }

    Provider* newprov = PF().create( desc );
    if ( !newprov )
    {
	if ( desc.errMsg().size() )
	{
	    errstr = desc.errMsg().buf();
	    errstr +=" for ";
	}
	else
	{
	    errstr = "error in definition";
	    errstr +=" of ";
	}
        errstr += desc.attribName(); 
	errstr += " "; errstr += "attribute.";
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
	    existing.remove(existing.indexOf(newprov), existing.size()-1 );
	    newprov->unRef();
	    return 0;
	}

	newprov->setInput( idx, inputprovider );
	inputprovider->addParent(newprov);
	issame = false;
    }

    if ( !newprov->checkInpAndParsAtStart() )
    {
	existing.remove( existing.indexOf(newprov), existing.size()-1 );
	BufferString attribnm = newprov->desc.attribName();
	if ( attribnm == StorageProvider::attribName() )
	{
	    errstr = "Cannot load Stored Cube '";
	    errstr += newprov->desc.userRef(); errstr += "'";
	}
	else
	{
	    errstr = "Attribute \""; errstr += newprov->desc.userRef(); 
	    errstr += "\" of type \""; errstr += attribnm;
	    errstr += "\" cannot be initialized";
	}
	newprov->unRef();
	return 0;
    }

    newprov->unRefNoDelete();
    return newprov;
}


Provider::Provider( Desc& nd )
    : desc( nd )
    , desiredvolume( 0 )
    , possiblevolume( 0 ) 
    , outputinterest( nd.nrOutputs(), 0 )
    , reqbufferstepout( 0, 0 )
    , desbufferstepout( 0, 0 )
    , providertask_( 0 )
    , currentbid( -1, -1 )
    , curlinekey_( 0, 0 )
    , linebuffer( 0 )
    , refstep( 0 )
    , alreadymoved(0)
    , isusedmulttimes(0)
    , seldata_(0)
    , curtrcinfo_(0)
    , extraz_(0,0)
    , trcinfobid( -1, -1 )
    , prevtrcnr( 0 )
    , needinterp( 0 )
    , useshortcuts_( 0 )
{
    desc.ref();
    inputs.allowNull(true);
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
	inputs += 0;
}


Provider::~Provider()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->unRef();
    inputs.erase();

    allexistingprov.erase();

    desc.unRef();

    delete providertask_;

    delete linebuffer;
    delete possiblevolume;
    delete desiredvolume;
}


bool Provider::isOK() const
{
    return errmsg.isEmpty(); /* Huh? &parser && parser.isOK(); */
}


Desc& Provider::getDesc()
{
    return desc;
}


const Desc& Provider::getDesc() const
{
    return const_cast<Provider*>(this)->getDesc();
}


void Provider::enableOutput( int out, bool yn )
{
    if ( out<0 || out >= outputinterest.size() )
	{ pErrMsg( "Huh?" ); return; }

    if ( yn )
	outputinterest[out]++;
    else
    {
	if ( !outputinterest[out] )
	{
	    pErrMsg( "Hue?");
	    return;
	}
	outputinterest[out]--;
    }
}


bool Provider::isOutputEnabled( int out ) const
{
    if ( out<0 || out >= outputinterest.size() )
	return false;
    else
	return outputinterest[out];
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl <= prefix##bufferstepout.inl \
	    && ns.crl <= prefix##bufferstepout.crl ) \
	return; \
\
    if ( ns.inl > prefix##bufferstepout.inl ) \
    	prefix##bufferstepout.inl = ns.inl; \
    if ( ns.crl > prefix##bufferstepout.crl ) \
    	prefix##bufferstepout.crl = ns.crl;\
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


void Provider::setDesiredVolume( const CubeSampling& ndv )
{
    if ( !desiredvolume )
	desiredvolume = new CubeSampling(ndv);
    else
    {
	if ( !isUsedMultTimes() )
	    *desiredvolume = ndv;
	else
	{
	    desiredvolume->hrg.start.inl = 
		desiredvolume->hrg.start.inl < ndv.hrg.start.inl ?
		desiredvolume->hrg.start.inl : ndv.hrg.start.inl;
	    desiredvolume->hrg.stop.inl =
		desiredvolume->hrg.stop.inl > ndv.hrg.stop.inl ?
		desiredvolume->hrg.stop.inl : ndv.hrg.stop.inl;
	    desiredvolume->hrg.stop.crl =
		desiredvolume->hrg.stop.crl > ndv.hrg.stop.crl ?
		desiredvolume->hrg.stop.crl : ndv.hrg.stop.crl;
	    desiredvolume->hrg.start.crl =
		desiredvolume->hrg.start.crl < ndv.hrg.start.crl ?
		desiredvolume->hrg.start.crl : ndv.hrg.start.crl;
	    desiredvolume->zrg.start = desiredvolume->zrg.start < ndv.zrg.start?
		desiredvolume->zrg.start : ndv.zrg.start;
	    desiredvolume->zrg.stop = desiredvolume->zrg.stop > ndv.zrg.stop ?
		desiredvolume->zrg.stop : ndv.zrg.stop;
	}
    }

    CubeSampling inputcs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	for ( int idy=0; idy<outputinterest.size(); idy++ )
	{
	    if ( outputinterest[idy]<1 ) continue;

	    bool isstored = inputs[idx] ? inputs[idx]->desc.isStored() : false;
	    if ( computeDesInputCube( idx, idy, inputcs, !isstored ) )
		inputs[idx]->setDesiredVolume( inputcs );
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

bool Provider::getPossibleVolume( int output, CubeSampling& res )
{
    CubeSampling tmpres = res;
    if ( inputs.size()==0 )
    {
	const bool is2d = getDesc().descSet()->is2D();
	if ( !is2d ) res.init(true);
	if ( !possiblevolume )
	    possiblevolume = new CubeSampling;

	if ( is2d ) *possiblevolume = res;
	return true;
    }

    if ( !desiredvolume ) return false;

    TypeSet<int> outputs;
    if ( output != -1 )
	outputs += output;
    else
    {
	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( outputinterest[idx] > 0 )
		outputs += idx;
	}
    }

    bool isset = false;
    for ( int idx=0; idx<outputs.size(); idx++ )
    {
	const int out = outputs[idx];
	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    CubeSampling inputcs = tmpres;
	    TypeSet<int> inputoutput;
	    if ( !getInputOutput( inp, inputoutput ) )
		continue;
	    
	    for ( int idy=0; idy<inputoutput.size(); idy++ )
	    {
		if ( !computeDesInputCube(inp, out, inputcs, true)) continue;
		if ( !inputs[inp]->getPossibleVolume( idy, inputcs ) ) 
		    continue;

		const BinID* stepout = reqStepout(inp,out);
		if ( stepout )
		{
		    int inlstepoutfact = desiredvolume->hrg.step.inl;
		    int crlstepoutfact = desiredvolume->hrg.step.crl;
		    inputcs.hrg.start.inl += stepout->inl * inlstepoutfact;
		    inputcs.hrg.start.crl += stepout->crl * crlstepoutfact;
		    inputcs.hrg.stop.inl -= stepout->inl * inlstepoutfact;
		    inputcs.hrg.stop.crl -= stepout->crl * crlstepoutfact;
		}

		const Interval<float>* zrg = reqZMargin(inp,out);
		if ( zrg )
		{
		    inputcs.zrg.start -= zrg->start;
		    inputcs.zrg.stop -= zrg->stop;
		}

		const Interval<int>* zrgsamp = reqZSampMargin(inp,out);
		if ( zrgsamp )
		{
		    inputcs.zrg.start -= zrgsamp->start*refstep;
		    inputcs.zrg.stop -= zrgsamp->stop*refstep;
		}
		
		res.limitToWithUdf( inputcs );
		isset = true;
	    }
	}
    }

    if ( !possiblevolume )
	possiblevolume = new CubeSampling;
    
    possiblevolume->hrg = res.hrg;
    possiblevolume->zrg = res.zrg;
    return isset;
}


int Provider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved )
	return 1;

    if ( inputs.size() < 1 )
	startpos = BinID(-1,-1);
    
    bool docheck = startpos == BinID(-1,-1);
    
    if ( getDesc().descSet()->is2D() )
	prevtrcnr = currentbid.crl;

    bool needmove;
    bool docontinue = true;
    ObjectSet<Provider> movinginputs;
    while ( docontinue )
    {
	needmove = docheck;
	for ( int idx=0; idx<inputs.size(); idx++ )
	{
	    if ( !inputs[idx] ) continue;
	    
	    currentbid = inputs[idx]->getCurrentPosition();
	    trcinfobid = inputs[idx]->getTrcInfoBid();
	    if ( !docheck && currentbid == startpos ) continue;
	    if ( !docheck && trcinfobid != BinID(-1,-1) 
		 && trcinfobid == startpos )
		continue;
	    
	    needmove = true;
	    const int res = inputs[idx]->moveToNextTrace(startpos, firstcheck);
	    if ( res!=1 ) return res;

	    if ( !inputs[idx]->getMSCProvider() ) continue;
	    if ( movinginputs.indexOf( inputs[idx] ) < 0 )
		movinginputs += inputs[idx];
	}
	if ( !needmove || docheck ) 
	    docontinue = false;
	
	if ( !docheck && firstcheck )
	{
	    bool allok = true;
	    for ( int idi=0; idi<inputs.size(); idi++)
	    {
		if ( inputs[idi] && inputs[idi]->getTrcInfoBid() != BinID(-1,-1)
		     && inputs[idi]->getTrcInfoBid() != startpos )
		{
		    allok = false;
		    break;
		}
	    }
	    
	    if ( !allok )
	    {
		BinID newstart( BinID(-1,-1) );
		computeNewStartPos( newstart );
	
		startpos = newstart;
		firstcheck = false;
		resetMoved();
	    }
	}
    }

    if ( movinginputs.isEmpty() && needmove )
    {
	if ( inputs.isEmpty() && !desc.isStored() )
	{
	    if ( currentbid.inl == -1 && currentbid.crl == -1 )
	    {
		const bool is2d = getDesc().descSet()->is2D();
		currentbid.inl = is2d ? 0 : desiredvolume->hrg.start.inl;
		currentbid.crl = desiredvolume->hrg.start.crl;
	    }
	    else
	    {
		BinID prevbid = currentbid;
		BinID step = getStepoutStep();
		if ( prevbid.crl +step.crl <= desiredvolume->hrg.stop.crl )
		    currentbid.crl = prevbid.crl +step.crl;
		else if ( prevbid.inl +step.inl <= desiredvolume->hrg.stop.inl )
		{
		    currentbid.inl = prevbid.inl +step.inl;
		    currentbid.crl = desiredvolume->hrg.start.crl;
		}
		else
		    return 0;
	    }
	}
	else if ( needmove )
	    currentbid = BinID(-1,-1);

	setCurrentPosition(currentbid);
	alreadymoved = true;
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
	currentbid = movinginputs[0]->getCurrentPosition();
	curtrcinfo_ = movinginputs[0]->getCurrentTrcInfo();
	trcinfobid = movinginputs[0]->getTrcInfoBid();
    }

    if ( docheck )
    {
	for ( int idx=0; idx<inputs.size(); idx++ )
	{
	    if ( !inputs[idx] ) continue;
	    if ( !inputs[idx]->setCurrentPosition( currentbid ) )
		return -1;
	}
	setCurrentPosition( currentbid );
    }

    alreadymoved = true;
    return 1;
}


void Provider::resetMoved()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	    inputs[idx]->resetMoved();

    alreadymoved = false;
}


void Provider::computeNewStartPos( BinID& newstart )
{
    const BinID step = getStepoutStep();
    for ( int idi=0; idi<inputs.size(); idi++ )
    {
	BinID inputbid(BinID(-1,-1));
	if ( inputs[idi] && inputs[idi]->getTrcInfoBid() != BinID(-1,-1) )
	    inputbid = inputs[idi]->getCurrentPosition();
	
	if ( inputbid == BinID(-1,-1) ) continue;
	if ( newstart == BinID(-1,-1) )
	{
	    newstart = inputbid;
	}
	else
	{
	    if ( desc.descSet()->is2D() )
		newstart = newstart.crl<inputbid.crl ? inputbid : newstart; 
	    else
	    {
		newstart.inl = step.inl<0 ? 
			       mMIN(newstart.inl,inputbid.inl):
			       mMAX(newstart.inl,inputbid.inl);
		newstart.crl = step.crl<0 ? 
			       mMIN(newstart.crl,inputbid.crl):
			       mMAX(newstart.crl,inputbid.crl);
	    }
	}
    }
}


int Provider::alignInputs( ObjectSet<Provider>& movinginputs )
{
    updateCurrentInfo();

    bool inp1_is_on_newline = false;
    bool inp2_is_on_newline = false;
    const bool is2d = getDesc().descSet()->is2D();
    for ( int inp1=0; inp1<movinginputs.size()-1; inp1++ )
    {
	if ( is2d ) inp1_is_on_newline = movinginputs[inp1]->isNew2DLine();

	for ( int inp2=inp1+1; inp2<movinginputs.size(); inp2++ )
	{
	    bool inp1moved = false;
	    if ( is2d ) inp2_is_on_newline = movinginputs[inp2]->isNew2DLine();

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
	int compres = input1->getMSCProvider()->
	    			comparePos( *input2->getMSCProvider() );

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
    return currentbid;
}


void Provider::updateCurrentInfo()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	inputs[idx]->updateCurrentInfo();
	if ( currentbid != inputs[idx]->getCurrentPosition() )
	    currentbid = inputs[idx]->getCurrentPosition();
	if ( inputs[idx]->getCurrentTrcInfo() && 
		curtrcinfo_ != inputs[idx]->getCurrentTrcInfo() )
	    curtrcinfo_ = inputs[idx]->getCurrentTrcInfo();
	if ( trcinfobid != inputs[idx]->getTrcInfoBid() )
	    trcinfobid = inputs[idx]->getTrcInfoBid();
    }
}


bool Provider::setCurrentPosition( const BinID& bid )
{
    if ( currentbid == BinID(-1,-1) )
	currentbid = bid;
    else if ( bid != currentbid )
    {
	if ( inputs.isEmpty() && !desc.isStored())
	    currentbid = bid;
	else
	    return false;
    }
    
    if ( linebuffer )
    {
	if ( doNotReUseDH() )
	{
	    linebuffer->removeAllExcept( currentbid );
	    return true;
	}
	
	const BinID step = getStepoutStep();
	BinID dir = BinID(1,1);
	dir.inl *= step.inl/abs(step.inl); dir.crl *= step.crl/abs(step.crl);
	const BinID lastbid = currentbid - desbufferstepout*step;
	linebuffer->removeBefore(lastbid, dir);
    // in every direction...
    }

    return true;
}


void Provider::addLocalCompZIntervals( const TypeSet< Interval<int> >& intvs )
{
    const float dz = mIsZero(refstep,mDefEps) ? SI().zStep() : refstep;
    const Interval<int> possintv( mNINT(possiblevolume->zrg.start/dz),
	    			  mNINT(possiblevolume->zrg.stop/dz) );

    Array2DImpl< Interval<int> > inputranges( inputs.size(), intvs.size() );
    for ( int idx=0; idx<intvs.size(); idx++ )
    {
	Interval<int> reqintv = intvs[idx];
	if ( reqintv.start > possintv.stop || reqintv.stop < possintv.start )
	{
	    for ( int inp=0; inp<inputs.size(); inp++ )
		inputranges.set( inp, idx, Interval<int>(mUdf(int),mUdf(int)) );
	    continue;
	}

	if ( possintv.start > reqintv.start )
	    reqintv.start = possintv.start;
	if ( possintv.stop < reqintv.stop )
	    reqintv.stop = possintv.stop;

	if ( !isUsedMultTimes() )
	    localcomputezintervals += reqintv;
	else
	{
	    if ( localcomputezintervals.size()<=idx )
		localcomputezintervals += reqintv;
	    else
		localcomputezintervals[idx].include(reqintv);
	}

	fillInputRangesArray( inputranges, idx, reqintv );
    }

    for ( int inp=0; inp<inputs.size(); inp++ )
    {
	if ( !inputs[inp] )
	    continue;

	TypeSet<Interval<int> > inpranges;
	for ( int idx=0; idx<intvs.size(); idx++ )
	{
	    const Interval<int> rg = inputranges.get( inp, idx );
	    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
		continue;
	    inpranges += rg;
	}
	inputs[inp]->addLocalCompZIntervals( inpranges );
    }
}


#define mUseMargins(type,Ts,ts)\
	const Interval<type>* req##ts = reqZ##Ts##Margin( inp, out );\
	if ( req##ts ) zrg##ts = *req##ts;\
	const Interval<type>* des##ts = desZ##Ts##Margin( inp, out );\
	if ( des##ts ) zrg##ts.include( *des##ts );\

void Provider::fillInputRangesArray( Array2DImpl< Interval<int> >& inputranges, 
				     int idx, const Interval<int>& reqintv )
{
    const float dz = mIsZero(refstep,mDefEps) ? SI().zStep() : refstep;
    for ( int out=0; out<outputinterest.size(); out++ )
    {
	if ( !outputinterest[out] ) continue;

	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    Interval<int> inputrange( reqintv );
	    Interval<float> zrg( 0, 0 );
	    mUseMargins(float,,);

	    Interval<int> zrgsamp( 0, 0 );
	    mUseMargins(int,Samp,samp);

	    inputrange.start += mNINT(zrg.start/dz);
	    inputrange.start += zrgsamp.start;
	    inputrange.stop += mNINT(zrg.stop/dz);
	    inputrange.stop += zrgsamp.stop;

	    inputranges.set( inp, idx, inputrange );
	}
    }
}


const TypeSet< Interval<int> >& Provider::localCompZIntervals() const
{
    return localcomputezintervals;
}


void Provider::resetZIntervals()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	    inputs[idx]->resetZIntervals();

    for ( int idx=localcomputezintervals.size(); idx>0; idx-- )
	localcomputezintervals.remove(idx-1);
}
    

BinID Provider::getStepoutStep() const
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] || !inputs[idx]->needStoredInput() ) continue;
	BinID bid = inputs[idx]->getStepoutStep();
	if ( bid.inl!=0 && bid.crl!=0 ) return bid;
    }

    for ( int idx=0; idx<parents.size(); idx++ )
    {
	if ( !parents[idx] ) continue;
	BinID bid = parents[idx]->getStepoutStep();
	if ( bid.inl!=0 && bid.crl!=0 ) return bid;
    }

    return BinID( SI().inlStep(), SI().crlStep() );
}



const DataHolder* Provider::getData( const BinID& relpos, int idi )
{
    if ( idi < 0 || idi >= localcomputezintervals.size() )
	return 0;

    const DataHolder* constres = getDataDontCompute(relpos);
    Interval<int> loczinterval( localcomputezintervals[idi] );
    if ( constres && constres->z0_ == loczinterval.start 
	    && constres->nrsamples_ == loczinterval.width()+1 )
	return constres;

    if ( !linebuffer )
	linebuffer = new DataHolderLineBuffer;
    DataHolder* outdata =
        linebuffer->createDataHolder( currentbid+relpos, loczinterval.start,
				      loczinterval.width()+1 );
    if ( !outdata || !getInputData(relpos, idi) )
    {
	if ( outdata ) linebuffer->removeDataHolder( currentbid+relpos );
	return 0;
    }
    
    const int nrsamples = outdata->nrsamples_;
    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	while ( outdata->nrSeries()<=idx )
	    outdata->add( true );
	
	if ( outputinterest[idx]<=0 ) 
	{
	    if ( outdata->series(idx) )
		outdata->replace( idx, 0 );

	    continue;
	}

	if ( !outdata->series(idx) )
	{
	    ValueSeries<float>* valptr = 0;
	    float dummy;
	    static const BinDataDesc floatdatadesc( dummy );
	    const BinDataDesc outputformat = getOutputFormat( idx );
	    if ( outputformat==floatdatadesc )
	    {
		float* ptr = new float[nrsamples];
		valptr =
		    new ArrayValueSeries<float,float>( ptr, true, nrsamples );
	    }
	    else
		valptr = new ConvMemValueSeries<float>( nrsamples,outputformat);

	    outdata->replace( idx, valptr );
	}
    }

    const int z0 = outdata->z0_;
    if ( needinterp )
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
	linebuffer->removeDataHolder( currentbid+relpos );
	return 0;
    }

    return outdata;
}


const DataHolder* Provider::getDataDontCompute( const BinID& relpos ) const
{
    return linebuffer ? linebuffer->getDataHolder(currentbid+relpos) : 0;
}


SeisMSCProvider* Provider::getMSCProvider() const
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	SeisMSCProvider* res = inputs[idx]->getMSCProvider();
	if ( res ) return res;
    }

    return 0;
}


bool Provider::checkInpAndParsAtStart()
{
    return true;
}


const char* Provider::prepare( Desc& desc )
{
    if ( !desc.needProvInit() )
	return 0;

    desc.setNeedProvInit( false );

    static BufferString errmsg;
    RefMan<Provider> prov = PF().create( desc );
    if ( prov && prov->isOK() )
	return 0;

    errmsg = "";
    if ( prov )
	errmsg = prov->errMsg();
    if ( errmsg.isEmpty() )
    {
	errmsg = "Cannot initialise '"; errmsg += desc.attribName();
	errmsg += "' Attribute properly";
    }
    return errmsg.buf();
}


void Provider::setOutputInterestSize( bool preserve )
{
    if ( preserve )
    {
	int outintsz = outputinterest.size();
	if ( outintsz == desc.nrOutputs() ) return;

	if ( outintsz < desc.nrOutputs() )
	{
	    TypeSet<int> addon(desc.nrOutputs()-outputinterest.size(),0);
	    outputinterest.append( addon );
	}
	else
	    outputinterest.remove( desc.nrOutputs()-1, outintsz-1 );
    }
    else
	outputinterest = TypeSet<int>(desc.nrOutputs(),0);
}


void Provider::enableAllOutputs( bool yn )
{
    for ( int idx=0; idx<nrOutputs(); idx++ )
	enableOutput( idx, yn );
}


bool Provider::computeData( const DataHolder& output, const BinID& relpos,
			    int t0,int nrsamples, int threadid ) const
{
    return computeData( output, relpos, t0, nrsamples );
}


int Provider::getDataIndex( int input ) const
{
    return desc.getInput(input) ? desc.getInput(input)->selectedOutput() : -1;
}


bool Provider::getInputData( const BinID&, int )
{
    return true;
}


bool Provider::getInputOutput( int input, TypeSet<int>& res ) const
{
    res.erase();

    Desc* inputdesc = desc.getInput(input);
    if ( !inputdesc ) return false;

    res += inputdesc->selectedOutput();
    return true;
}


void Provider::setInput( int inp, Provider* np )
{
    if ( inputs[inp] )
    {
	if ( inputs[inp]->desc.isSteering() )
	    initSteering();

	TypeSet<int> inputoutputs;
	if ( getInputOutput( inp, inputoutputs ) )
	{
	    for ( int idx=0; idx<inputoutputs.size(); idx++ )
		inputs[inp]->enableOutput( inputoutputs[idx], false );
	}
	inputs[inp]->unRef();
    }

    inputs.replace( inp, np );
    if ( !inputs[inp] )
	return;

    inputs[inp]->ref();
    if ( inputs[inp]->desc.isSteering() )
	initSteering();
    
    TypeSet<int> inputoutputs;
    if ( getInputOutput( inp, inputoutputs ) )
    {
	for ( int idx=0; idx<inputoutputs.size(); idx++ )
	    inputs[inp]->enableOutput( inputoutputs[idx], true );
	inputs[inp]->updateInputReqs(-1);
    }

    updateInputReqs(inp);
    inputs[inp]->updateStorageReqs();
    if ( inputs[inp]->desc.isSteering() )
    {
	inputs[inp]->updateInputReqs(-1);
	inputs[inp]->updateStorageReqs(-1);
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
	for ( int idx=0; idx<inputs.size(); idx++ )
	    if ( inputs[idx] ) inputs[idx]->updateStorageReqs( all );
    }
}


bool Provider::computeDesInputCube( int inp, int out, CubeSampling& res, 
					bool usestepout ) const
{
    //Be careful if usestepout=true with des and req stepouts
    if ( seldata_ && seldata_->type() == Seis::Table )
    {
	Interval<float> zrg(0,0);
	mUseMargins(float,,)

	Interval<int> zrgsamp(0,0);
	mUseMargins(int,Samp,samp)
	
	zrg.include( Interval<float>( zrgsamp.start*refstep,
		    		      zrgsamp.stop*refstep ) );

	Interval<float> extraz = Interval<float>(extraz_.start + zrg.start,
						 extraz_.stop + zrg.stop);
	const_cast<Provider*>(inputs[inp])->setSelData( seldata_ );
	const_cast<Provider*>(inputs[inp])->setExtraZ( extraz );
    }
    
    if ( !desiredvolume )
	return false;

    res = *desiredvolume;

    if ( usestepout )
    {
	int inlstepoutfact = desiredvolume->hrg.step.inl;
	int crlstepoutfact = desiredvolume->hrg.step.crl;

	BinID stepout(0,0);
	const BinID* reqstepout = reqStepout( inp, out );
	if ( reqstepout ) stepout=*reqstepout;
	const BinID* desstepout = desStepout( inp, out );
	if ( desstepout )
	{
	    if ( stepout.inl < desstepout->inl ) stepout.inl = desstepout->inl;
	    if ( stepout.crl < desstepout->crl ) stepout.crl = desstepout->crl;
	}

	res.hrg.start.inl -= stepout.inl * inlstepoutfact;
	res.hrg.start.crl -= stepout.crl * crlstepoutfact;
	res.hrg.stop.inl += stepout.inl * inlstepoutfact;
	res.hrg.stop.crl += stepout.crl * crlstepoutfact;
    }
   
    Interval<float> zrg(0,0);
    mUseMargins(float,,)

    Interval<int> zrgsamp(0,0);
    mUseMargins(int,Samp,samp)
    zrg.include(Interval<float>( zrgsamp.start*refstep, zrgsamp.stop*refstep ));
    
    res.zrg.start += zrg.start;
    res.zrg.stop += zrg.stop;

    return true;
}


void Provider::updateInputReqs( int inp )
{
    if ( inp == -1 )
    {
	for ( int idx=0; idx<inputs.size(); idx++ )
	    updateInputReqs(idx);
	return;
    }

    CubeSampling inputcs;
    for ( int out=0; out<outputinterest.size(); out++ )
    {
	if ( !outputinterest[out] ) continue;

	bool isstored = inputs[inp] ? inputs[inp]->desc.isStored() : false;
	if ( computeDesInputCube( inp, out, inputcs, !isstored ) )
	    inputs[inp]->setDesiredVolume( inputcs );

	BinID stepout(0,0);
	const BinID* req = reqStepout(inp,out);
	if ( req ) stepout = *req;
	const BinID* des = desStepout(inp,out);
	if ( des )
	{
	    stepout.inl = mMAX(stepout.inl,des->inl);
	    stepout.crl = mMAX(stepout.crl,des->crl );
	}

	if ( inputs[inp] )
	{
	    inputs[inp]->setReqBufStepout( ( req ? *req : BinID(0,0) ) + 
		    			   reqbufferstepout, true );
	    inputs[inp]->setDesBufStepout( stepout+desbufferstepout );
	}
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
	return tsd->binidValueSet().totalSize();
    }
    if ( !possiblevolume || !desiredvolume )
	return false;

    CubeSampling cs = *desiredvolume;
    if ( getDesc().isStored() )
    {
	cs.hrg.start.inl =
	    desiredvolume->hrg.start.inl < cs.hrg.start.inl ?
	    cs.hrg.start.inl : desiredvolume->hrg.start.inl;
	cs.hrg.stop.inl =
	    desiredvolume->hrg.stop.inl > cs.hrg.stop.inl ?
	    cs.hrg.stop.inl : desiredvolume->hrg.stop.inl;
	cs.hrg.stop.crl =
	    desiredvolume->hrg.stop.crl > cs.hrg.stop.crl ?
	    cs.hrg.stop.crl : desiredvolume->hrg.stop.crl;
	cs.hrg.start.crl =
	    desiredvolume->hrg.start.crl < cs.hrg.start.crl ?
	    cs.hrg.start.crl : desiredvolume->hrg.start.crl;
    }
    return is2d ? cs.nrCrl() : cs.nrInl() * cs.nrCrl();
}


void Provider::computeRefStep()
{
    for( int idx=0; idx<allexistingprov.size(); idx++ )
    {
	float step = 0;
	bool isstored = allexistingprov[idx]->getZStepStoredData(step);
	if ( isstored )
	    refstep = ( refstep != 0 && refstep < step )? refstep : step;
	    
    }
}


void Provider::setRefStep( float step )
{
    refstep = step;
    for ( int idx=0; idx<allexistingprov.size(); idx++ )
	const_cast<Provider*>(allexistingprov[idx])->refstep = refstep;
}


void Provider::setCurLineKey( const char* linename )
{
    BufferString attrname;
    if ( !desc.isStored() )
	attrname = desc.userRef();
    else
    {
	const ValParam* idpar = desc.getValParam( StorageProvider::keyStr() );
	LineKey lk( idpar->getStringValue() );
	attrname = lk.attrName();
    }

    curlinekey_.setLineName( linename );
    curlinekey_.setAttrName( attrname );
    for ( int idx=0; idx<inputs.size(); idx++ )
    {   
	if ( !inputs[idx] ) continue;
	inputs[idx]->setCurLineKey( curlinekey_.lineName() );
    }
}


void Provider::adjust2DLineStoredVolume()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	    inputs[idx]->adjust2DLineStoredVolume();
}


void Provider::setSelData( const Seis::SelData* seldata )
{
    seldata_ = seldata;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setSelData(seldata_);
    }
}


void Provider::setExtraZ( const Interval<float>& extraz )
{
    extraz_.include( extraz );
}


void Provider::setPossibleVolume( const CubeSampling& cs )
{
    if ( possiblevolume )
	delete possiblevolume;

    possiblevolume = new CubeSampling(cs);
}


float Provider::getRefStep() const
{ 
    return !mIsZero(refstep,mDefEps) ? refstep : SI().zStep();
}


bool Provider::zIsTime() const 
{
    return SI().zIsTime();
}


float Provider::inldist() const
{
    return SI().inlDistance();
}


float Provider::crldist() const
{
    return SI().crlDistance();
}


BufferString Provider::errMsg() const
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->errMsg().size() )
	    return inputs[idx]->errMsg();
    }
    
    return errmsg;
}


void Provider::setUsedMultTimes()
{
    isusedmulttimes = true;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setUsedMultTimes();
    }
}


void Provider::setNeedInterpol( bool yn )
{
    needinterp = yn;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setNeedInterpol( yn );
    }
}


void Provider::resetDesiredVolume()
{
    if ( desiredvolume )
    {
	delete desiredvolume; 
	desiredvolume = 0;
    }
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->resetDesiredVolume();
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
    float extraz = 0;
    if ( needinterp )
    {
	int intvidx = 0;
	for ( int idx=0; idx<localcomputezintervals.size(); idx++ )
	    if ( localcomputezintervals[idx].includes(z0) )
		intvidx = idx;

	float exacttime = exactz_[intvidx];
	extraz = getExtraZFromSampPos( exacttime );
    }

    if ( needinterp && !mIsEqual(extraz,input.extrazfromsamppos_,mDefEps) )
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
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->prepareForComputeData();
}


void Provider::prepPriorToBoundsCalc()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->prepPriorToBoundsCalc();
}


bool Provider::prepPriorToOutputSetup()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->prepPriorToOutputSetup();

    return false;
}


void Provider::setExactZ( const TypeSet<float>& exactz )
{
    exactz_ = exactz;
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	    inputs[idx]->setExactZ( exactz );
}


void Provider::getCompNames( BufferStringSet& nms ) const
{
    nms.erase();
    nms.add( desc.attribName() );
}


float Provider::getMaxDistBetwTrcs() const
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	{
	    float tmp = inputs[idx]->getMaxDistBetwTrcs();
	    if ( !mIsUdf(tmp) ) return tmp;
	}

    return mUdf(float);
}


bool Provider::needStoredInput() const
{
    bool needinput = false;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->needStoredInput() )
	{
	    needinput = true;
	    break;
	}
    }

    return needinput;
}


void Provider::setRdmPaths( TypeSet<BinID>* truepath,
			    TypeSet<BinID>* snappedpath )
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setRdmPaths( truepath, snappedpath );
    }
}


float Provider::getExtraZFromSampPos( float exacttime ) const
{
    return DataHolder::getExtraZFromSampPos( exacttime, getRefStep() );
}


float Provider::getExtraZFromSampInterval( int z0, int nrsamples ) const
{
    int intvidx = -1;
    for ( int idx=0; idx<localcomputezintervals.size(); idx++ )
    {
	 if ( localcomputezintervals[idx].includes( z0 ) &&
	      localcomputezintervals[idx].includes( z0+nrsamples-1 ) )
	 {
	     intvidx = idx;
	     break;
	 }
    }

    return ( intvidx>=0 && exactz_.size()>intvidx )
		? getExtraZFromSampPos( exactz_[intvidx] ) : 0;
}


}; // namespace Attrib
