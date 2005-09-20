/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribprovider.cc,v 1.37 2005-09-20 15:10:04 cvshelene Exp $";

#include "attribprovider.h"
#include "attribstorprovider.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attriblinebuffer.h"
#include "attribparam.h"
#include "basictask.h"
#include "cubesampling.h"
#include "errh.h"
#include "seisreq.h"
#include "seistrcsel.h"
#include "survinfo.h"
#include "threadwork.h"
#include "arrayndimpl.h"


namespace Attrib
{

class ProviderBasicTask : public BasicTask
{
public:

ProviderBasicTask( const Provider& p ) : provider( p )	{}

void setScope( const DataHolder* res_, const BinID& relpos_, int z0_,
	       int nrsamples_ )
{
    res = res_;
    relpos = relpos_;
    z0 = z0_;
    nrsamples = nrsamples_;
}

int nextStep()
{
    if ( !res ) return 0;
    return provider.computeData(*res,relpos,z0,nrsamples)?0:-1;
}

protected:

    const Provider&		provider;
    const DataHolder*		res;
    BinID			relpos;
    int				z0;
    int				nrsamples;

};


Provider* Provider::create( Desc& desc )
{
    ObjectSet<Provider> existing;
    bool issame = false;
    Provider* prov = internalCreate( desc, existing, issame );
    if ( !prov ) return 0;

    prov->allexistingprov = existing;
    prov->computeRefZStep( existing );
    prov->propagateZRefStep( existing );
    return prov;
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing, 
				    bool& issame )
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
	return 0;

    Provider* res = PF().create( desc );
    if ( !res ) return 0;

    res->ref();
    
    if ( desc.selectedOutput()!=-1 && !existing.size() )
	res->enableOutput( desc.selectedOutput(), true );

    existing += res;

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	Desc* inputdesc = desc.getInput(idx);
	if ( !inputdesc ) continue;

	Provider* inputprovider = internalCreate(*inputdesc, existing, issame);
	if ( !inputprovider )
	{
	    existing.remove(existing.indexOf(res), existing.size()-1 );
	    res->unRef();
	    return 0;
	}

	bool alreadythere = false;
	if ( issame )
	{
	    for ( int idy=0; idy<desc.nrInputs()-1; idy++ )
	    {
		if ( res->getInputs().size()<idy)
		if ( res->getInputs()[idy]->getDesc().
				isIdenticalTo( *inputdesc, false ) )
		    alreadythere = true;
	    }
	}
		    
	if ( !alreadythere )
	{
	    res->setInput( idx, inputprovider );
	    inputprovider->addParent(res);
	}
	issame = false;
    }

    if ( !res->init() )
    {
	existing.remove(existing.indexOf(res), existing.size()-1 );
	res->unRef();
	return 0;
    }
    
    res->unRefNoDelete();
    return res;
}


Provider::Provider( Desc& nd )
    : desc( nd )
    , desiredvolume( 0 )
    , possiblevolume( 0 ) 
    , outputinterest( nd.nrOutputs(), 0 )
    , bufferstepout( 0, 0 )
    , threadmanager( 0 )
    , currentbid( -1, -1 )
    , curlinekey_( 0, 0 )
    , linebuffer( 0 )
    , refstep( 0 )
    , alreadymoved(0)
    , isusedmulttimes(0)
    , seldata_(*new SeisSelData)
    , curtrcinfo_(0)
{
    mRefCountConstructor;
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

    delete threadmanager;
    deepErase( computetasks );

    delete linebuffer;
    delete possiblevolume;
    delete desiredvolume;
    delete &seldata_;
}


bool Provider::isOK() const
{
    return true; /* Huh? &parser && parser.isOK(); */
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


void Provider::setBufferStepout( const BinID& ns )
{
    if ( ns.inl <= bufferstepout.inl && ns.crl <= bufferstepout.crl )
	return;

    if ( ns.inl > bufferstepout.inl ) bufferstepout.inl = ns.inl;
    if ( ns.crl > bufferstepout.crl ) bufferstepout.crl = ns.crl;

    updateInputReqs(-1);
}


const BinID& Provider::getBufferStepout() const
{
    return bufferstepout;
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

	    bool isstored = inputs[idx]? inputs[idx]->desc.isStored() : false;
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
	res.init(true);
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

//		if ( !isset )
//		{
//		    res = inputcs;
//		    isset = true;
//		    continue;
//		}

#		define mAdjustIf(v1,op,v2) \
				    if ( mIsUdf(v1) || v1 op v2 ) v1 = v2;
		mAdjustIf(res.hrg.start.inl,<,inputcs.hrg.start.inl);
		mAdjustIf(res.hrg.start.crl,<,inputcs.hrg.start.crl);
		mAdjustIf(res.zrg.start,<,inputcs.zrg.start);
		mAdjustIf(res.hrg.stop.inl,>,inputcs.hrg.stop.inl);
		mAdjustIf(res.hrg.stop.crl,>,inputcs.hrg.stop.crl);
		mAdjustIf(res.zrg.stop,>,inputcs.zrg.stop);
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


int Provider::moveToNextTrace()
{
    if ( alreadymoved )
	return 1;

    ObjectSet<Provider> movinginputs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	const int res = inputs[idx]->moveToNextTrace();
	if ( res!=1 ) return res;

	if ( !inputs[idx]->getSeisRequester() ) continue;
	movinginputs += inputs[idx];
    }

    if ( !movinginputs.size() )
    {
	if ( !inputs.size() && !desc.isStored() )
	{
	    if ( currentbid.inl == -1 && currentbid.crl == -1 )
	    {
		currentbid.inl = desiredvolume->hrg.start.inl;
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
	else
	{
	    currentbid = BinID(-1,-1);
	}

	setCurrentPosition(currentbid);
	return 1;
    }

    for ( int idx=0; idx<movinginputs.size()-1; idx++ )
    {
	for ( int idy=idx+1; idy<movinginputs.size(); idy++ )
	{
	    bool idxmoved = false;

	    while ( true )
	    {
		int compres = movinginputs[idx]->getSeisRequester()->comparePos(
				    *movinginputs[idy]->getSeisRequester() );
		if ( compres == -1 )
		{
		    idxmoved = true;
		    movinginputs[idx]->resetMoved();
		    const int res = movinginputs[idx]->moveToNextTrace();
		    if ( res != 1 ) return res;
		}
		else if ( compres == 1 )
		{
		    movinginputs[idy]->resetMoved();
		    const int res = movinginputs[idy]->moveToNextTrace();
		    if ( res != 1 ) return res;
		}
		else 
		    break;
	    }

	    if ( idxmoved )
	    {
		idx = -1;
		break;
	    }
	}
    }

    currentbid = movinginputs[0]->getCurrentPosition();
    curtrcinfo_ = movinginputs[0]->getCurrentTrcInfo();

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	if ( !inputs[idx]->setCurrentPosition(currentbid) )
	    return -1;
    }
    setCurrentPosition( currentbid );

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


BinID Provider::getCurrentPosition() const
{
    return currentbid;
}


bool Provider::setCurrentPosition( const BinID& bid )
{
    if ( currentbid == BinID(-1,-1) )
	currentbid = bid;
    else if ( bid != currentbid )
    {
	pErrMsg( "Huh? (should never happen)");
	return false;
    }
    
    if ( linebuffer )
    {
	const BinID step = getStepoutStep();
	BinID dir = BinID(1,1);
	dir.inl *= step.inl/abs(step.inl); dir.crl *= step.crl/abs(step.crl);
	const BinID lastbid = currentbid - dir*bufferstepout*step;
	linebuffer->removeBefore(lastbid, dir);
    // in every direction...
    }

    return true;
}


void Provider::addLocalCompZIntervals( const TypeSet< Interval<int> >& intvs )
{
    const float surveystep = SI().zStep();
    const float dz = mIsZero(refstep,mDefEps) ? surveystep : refstep;
    const Interval<int> possintv( mNINT(possiblevolume->zrg.start/dz),
	    			  mNINT(possiblevolume->zrg.stop/dz) );

    Array2DImpl< Interval<int> > inputranges( inputs.size(), intvs.size() );
    for ( int idx=0; idx<intvs.size(); idx++ )
    {
	Interval<int> reqintv = intvs[idx];
	if ( !mIsEqual(dz,surveystep,mDefEps) )
	    reqintv.scale( mNINT(surveystep/refstep) );

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

	for ( int out=0; out<outputinterest.size(); out++ )
	{
	    if ( !outputinterest[out] ) continue;

	    for ( int inp=0; inp<inputs.size(); inp++ )
	    {
		if ( !inputs[inp] )
		    continue;

		Interval<int> inputrange( reqintv );
		Interval<float> zrg( 0, 0 );
		const Interval<float>* req = reqZMargin( inp, out );
		if ( req ) zrg = *req;
		const Interval<float>* des = desZMargin( inp, out );
		if ( des ) zrg.include( *des );

		inputrange.start += mNINT(zrg.start/dz);
		inputrange.stop += mNINT(zrg.stop/dz);

		inputranges.set( inp, idx, inputrange );
	    }
	}
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
	if ( !inputs[idx] ) continue;
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
    if ( constres && constres->z0_ == localcomputezintervals[idi].start 
	    && constres->nrsamples_ == localcomputezintervals[idi].width()+1 )
	return constres;

    if ( !linebuffer )
	linebuffer = new DataHolderLineBuffer;
    DataHolder* outdata =
        linebuffer->createDataHolder( currentbid+relpos,
				      localcomputezintervals[idi].start,
				      localcomputezintervals[idi].width()+1 );
    if ( !outdata || !getInputData(relpos, idi) )
	return 0;
    
    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	while ( outdata->nrSeries()<=idx )
	    outdata->add();
	
	if ( outputinterest[idx]<=0 ) 
	{
	    if ( outdata->series(idx) && outdata->series(idx)->arr() )
		outdata->replace( idx, 0 );

	    continue;
	}

	if ( !outdata->series(idx)->arr() )
	{
	    float* ptr = new float[outdata->nrsamples_];
	    outdata->replace( idx, new ArrayValueSeries<float>(ptr) );
	}
    }

    const int z0 = outdata->z0_;
    const int nrsamples = outdata->nrsamples_;

    bool success = false;
    if ( !threadmanager )
	success = computeData( *outdata, relpos, z0, nrsamples );
    else
    {
	if ( !computetasks.size() )
	{
	    for ( int idx=0; idx<threadmanager->nrThreads(); idx++)
		computetasks += new ProviderBasicTask(*this);
	}

	//TODO Divide task
	success = threadmanager->addWork( computetasks );
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


SeisRequester* Provider::getSeisRequester() const
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	SeisRequester* res = inputs[idx]->getSeisRequester();
	if ( res ) return res;
    }

    return 0;
}


bool Provider::init()
{
    return true;
}


void Provider::setOutputInterestSize()
{
    outputinterest = TypeSet<int>(desc.nrOutputs(),0);
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
    if ( seldata_.type_ == Seis::Table )
    {
	Interval<float> zrg(0,0);
	const Interval<float>* reqzrg = reqZMargin(inp,out);
	if ( reqzrg ) zrg=*reqzrg;
	const Interval<float>* deszrg = desZMargin(inp,out);
	if ( deszrg ) zrg.include( *deszrg );
	seldata_.extraz_ = zrg;
	inputs[inp]->setSelData(seldata_);
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
    const Interval<float>* reqzrg = reqZMargin(inp,out);
    if ( reqzrg ) zrg=*reqzrg;
    const Interval<float>* deszrg = desZMargin(inp,out);
    if ( deszrg ) zrg.include( *deszrg );
    
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
	    inputs[inp]->setBufferStepout( stepout+bufferstepout );
    }
}


const BinID* Provider::desStepout(int,int) const		{ return 0; }
const BinID* Provider::reqStepout(int,int) const		{ return 0; }
const Interval<float>* Provider::desZMargin(int,int) const	{ return 0; }
const Interval<float>* Provider::reqZMargin(int,int) const	{ return 0; }


int Provider::getTotalNrPos( bool is2d )
{
    if ( seldata_.type_ == Seis::Table )
    {
	return seldata_.table_.totalSize();
    }
    if ( !possiblevolume )
	return false;

    CubeSampling cs = *possiblevolume;
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


void Provider::computeRefZStep( const ObjectSet<Provider>& existing )
{
    for( int idx=0; idx<existing.size(); idx++ )
    {
	float step = 0;
	bool isstored = existing[idx]->getZStepStoredData(step);
	if ( isstored )
	    refstep = ( refstep != 0 && refstep < step )? refstep : step;
	    
    }
}


void Provider::propagateZRefStep( const ObjectSet<Provider>& existing )
{
    for ( int idx=0; idx<existing.size(); idx++ )
	existing[idx]->refstep = refstep;
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


void Provider::setSelData( const SeisSelData& seldata )
{
    seldata_ = seldata;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setSelData(seldata_);
    }
}


void Provider::setPossibleVolume( const CubeSampling& cs)
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
    return SI().transform( BinID(0,0) ).distance( SI().transform(BinID(1,0)) );
}


float Provider::crldist() const
{
    return SI().transform( BinID(0,0) ).distance( SI().transform(BinID(0,1)) );
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

}; // namespace Attrib
