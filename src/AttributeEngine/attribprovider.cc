/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribprovider.cc,v 1.5 2005-02-03 15:35:02 kristofer Exp $";

#include "attribprovider.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attriblinebuffer.h"
#include "attribparam.h"
#include "basictask.h"
#include "cubesampling.h"
#include "errh.h"
#include "seisreq.h"
#include "survinfo.h"
#include "threadwork.h"


namespace Attrib
{

class ProviderBasicTask : public BasicTask
{
public:
		ProviderBasicTask( const Provider& p )
		    : provider( p ) {}

    void	setScope( const DataHolder* res_, const BinID& relpos_,
	    		  int t1_, int nrsamples_ )
		{ res = res_; relpos=relpos_; t1=t1_; nrsamples=nrsamples_; }

    int		nextStep()
    		{
		    if ( !res ) return 0;
		    return provider.computeData(*res,relpos,t1,nrsamples)?0:-1;
		}

protected:

    const Provider&		provider;
    const DataHolder*		res;
    BinID			relpos;
    int				t1;
    int				nrsamples;
};



Provider* Provider::create( Desc& desc )
{
    ObjectSet<Provider> existing;
    return internalCreate( desc, existing );
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing )
{
    for ( int idx=0; idx<existing.size(); idx++ )
    {
	if ( existing[idx]->getDesc().isIdenticalTo( desc, false ) )
	    return existing[idx];
    }

    if ( desc.nrInputs() && !desc.descSet() )
	return 0;

    Provider* res = PF().create( desc );
    if ( !res ) return 0;

    res->ref();
    existing += res;

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	Desc* inputdesc = desc.getInput(idx);
	if ( !inputdesc ) continue;

	Provider* inputprovider = internalCreate( *inputdesc, existing );
	if ( !inputprovider )
	{
	    existing.remove(existing.indexOf(res), existing.size()-1 );
	    res->unRef();
	    return 0;
	}

	res->setInput( idx, inputprovider );
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
    , outputinterest( nd.nrOutputs(), 0 )
    , bufferstepout( 0, 0 )
    , threadmanager( 0 )
    , currentbid( -1, -1 )
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

    desc.unRef();

    delete threadmanager;
    deepErase( computetasks );

    delete linebuffer;
}


bool Provider::isOK() const { return true; /*&parser && parser.isOK(); */}


Desc& Provider::getDesc() { return desc; }


const Desc& Provider::getDesc() const
{ return const_cast<Provider*>(this)->getDesc(); }


void Provider::enableOutput( int out, bool yn )
{
    if ( out<0||out>=outputinterest.size() )
    {
	pErrMsg( "Hue?");
	return;
    }

    if ( yn ) outputinterest[out]++;
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


void Provider::setBufferStepout( const BinID& ns )
{
    if ( ns.inl<=bufferstepout.inl && ns.crl<=bufferstepout.crl )
	return;

    bufferstepout.inl = mMAX( bufferstepout.inl, ns.inl );
    bufferstepout.crl = mMAX( bufferstepout.crl, ns.crl );

    updateInputReqs(-1);
}


const BinID& Provider::getBufferStepout() const { return bufferstepout; }


void Provider::setDesiredVolume( const CubeSampling& ndv )
{
    if ( !desiredvolume ) desiredvolume = new CubeSampling(ndv);
    else *desiredvolume = ndv;

    CubeSampling inputcs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	for ( int idy=0; idy<outputinterest.size(); idy++ )
	{
	    if ( outputinterest[idy]<1 ) continue;

	    if ( computeDesInputCube( idx, idy, inputcs ) )
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

bool Provider::getPossibleVolume( int output, CubeSampling& res ) const
{
    if ( inputs.size()==0 )
    {
	res.init(true);
	return true;
    }

    if ( !desiredvolume ) return false;

    TypeSet<int> outputs;
    if ( output!=-1 ) outputs += output;
    else
    {
	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( outputinterest[idx]>0 ) outputs += idx;
	}
    }

    CubeSampling inputcs;
    bool isset = false;
    for ( int idx=0; idx<outputs.size(); idx++ )
    {
	const int out = outputs[idx];
	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    TypeSet<int> inputoutput;
	    if ( !getInputOutput( inp, inputoutput ) )
		continue;
	    
	    for ( int idy=0; idy<inputoutput.size(); idy++ )
	    {
		if ( !inputs[inp]->getPossibleVolume( idy, inputcs ) ) 
		    continue;

		const BinID* stepout = reqStepout(inp,out);
		if ( stepout )
		{
		    inputcs.hrg.start.inl += stepout->inl;
		    inputcs.hrg.start.crl += stepout->crl;
		    inputcs.hrg.stop.inl -= stepout->inl;
		    inputcs.hrg.stop.crl -= stepout->crl;
		}

		const Interval<float>* zrg = reqZMargin(inp,out);
		if ( zrg )
		{
		    inputcs.zrg.start -= zrg->start;
		    inputcs.zrg.stop -= zrg->stop;
		}

		if ( !isset )
		{
		    res = inputcs;
		    isset = true;
		    continue;
		}

		res.hrg.start.inl=mMAX(res.hrg.start.inl,inputcs.hrg.start.inl);
		res.hrg.start.crl=mMAX(res.hrg.start.crl,inputcs.hrg.start.crl);
		res.hrg.stop.inl = mMIN( res.hrg.stop.inl,inputcs.hrg.stop.inl);
		res.hrg.stop.crl =mMIN( res.hrg.stop.crl, inputcs.hrg.stop.crl);
		res.zrg.start = mMAX( res.zrg.start, inputcs.zrg.start);
		res.zrg.stop = mMIN( res.zrg.stop, inputcs.zrg.stop);
	    }
	}
    }

    return isset;
}


int Provider::moveToNextTrace()
{

    ObjectSet<Provider> movinginputs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const int res = inputs[idx]->moveToNextTrace();
	if ( res!=1 ) return res;

	if ( !inputs[idx]->getSeisRequester() ) continue;
	movinginputs += inputs[idx];
    }

    if ( !movinginputs.size() )
    {
	currentbid = BinID(-1,-1);
	return true;
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
		if ( compres==-1 )
		{
		    idxmoved = true;
		    const int res = movinginputs[idx]->moveToNextTrace();
		    if ( res!=1 ) return res;
		}
		else if ( compres==1 )
		{
		    const int res = movinginputs[idy]->moveToNextTrace();
		    if ( res!=1 ) return res;
		}
		else 
		    break;
	    }

	    if ( idxmoved )
	    {
		idx=-1;
		break;
	    }
	}
    }

    currentbid = movinginputs[0]->getCurrentPosition();

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx]->setCurrentPosition(currentbid) )
	    return -1;
    }

    return 1;
}


BinID Provider::getCurrentPosition() const { return currentbid; }


bool Provider::setCurrentPosition( const BinID& bid )
{
    if ( currentbid==BinID(-1,-1) )
    {
	currentbid = bid;
    }
    else if ( bid!=currentbid )
    {
	pErrMsg("This should never happen");
	return false;
    }

    //TODO Remove old buffers
    localcomputezinterval.start = INT_MAX;
    localcomputezinterval.stop = INT_MIN;

    return true;
}


void Provider::addLocalCompZInterval( const Interval<int>& ni )
{
    localcomputezinterval.include( ni, false );

    const float dz = SI().zRange(true).step;

    for ( int out=0; out<outputinterest.size(); out++ )
    {
	if ( !outputinterest[out] ) continue;

	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    Interval<int> inputrange( ni );
	    Interval<float> zrg(0,0);
	    const Interval<float>* req =  reqZMargin(inp,out);
	    if ( req ) zrg =  *req;
	    const Interval<float>* des =  desZMargin(inp,out);
	    if ( req )  zrg.include( *req );

	    inputrange.start += (int) (zrg.start/dz-0.5);
	    inputrange.stop += (int) (zrg.stop/dz+0.5);

	    inputs[inp]->addLocalCompZInterval(inputrange);
	}
    }
}


const Interval<int>& Provider::localCompZInterval() const
{ return localcomputezinterval; }


const DataHolder* Provider::getData( const BinID& relpos )
{
    const DataHolder* constres = getDataDontCompute(relpos);
    if ( constres )
    {
	//Todo check range
	return constres;
    }

    if ( !linebuffer ) linebuffer = new DataHolderLineBuffer;

    DataHolder* outdata =
        linebuffer->createDataHolder( currentbid+relpos,
				      localcomputezinterval.start,
				      localcomputezinterval.width()+1 );

    if ( !outdata ) return 0;

    if ( !getInputData(relpos) )
	return 0;

    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	while ( outdata->size()<=idx ) (*outdata) += 0;
	if ( outputinterest[idx]<=0 ) 
	{
	    if ( (*outdata)[idx] )
	    {
		delete (*outdata)[idx];
		outdata->replace( 0, idx );
	    }

	    continue;
	}

	if ( !(*outdata)[idx] )
	{
	    float* ptr = new float[outdata->nrsamples];
	    outdata->replace( new SingleDataHolderPtrImpl<float>(ptr), idx );
	}
    }

    const int t1 = outdata->t1;
    const int nrsamples = outdata->nrsamples;

    bool success = false;
    if ( threadmanager )
    {
	if ( !computetasks.size() )
	{
	    for ( int idx=0; idx<threadmanager->nrThreads(); idx++)
		computetasks += new ProviderBasicTask(*this);
	}

	//TODO Divide task

	success = threadmanager->addWork(computetasks);
    }
    else
	success = computeData( *outdata, relpos, t1, nrsamples );

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


SeisRequester* Provider::getSeisRequester()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	SeisRequester* res = inputs[idx]->getSeisRequester();
	if ( res ) return res;
    }

    return 0;
}


bool Provider::init() { return true; }


bool Provider::getInputData( const BinID& )
{ return true; }


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
	TypeSet<int> inputoutputs;
	if ( getInputOutput( inp, inputoutputs ) )
	{
	    for ( int idx=0; idx<inputoutputs.size(); idx++ )
		inputs[inp]->enableOutput( idx, false );
	}
	inputs[inp]->unRef();
    }

    inputs.replace( np, inp );
    if ( !inputs[inp] )
	return;

    inputs[inp]->ref();
    TypeSet<int> inputoutputs;
    if ( getInputOutput( inp, inputoutputs ) )
    {
	for ( int idx=0; idx<inputoutputs.size(); idx++ )
	    inputs[inp]->enableOutput( idx, true );
    }

    updateInputReqs(inp);
}


bool Provider::computeDesInputCube( int inp, int out, CubeSampling& res ) const
{
    if ( !desiredvolume )
	return false;

    res = *desiredvolume;

    BinID stepout(0,0);
    const BinID* reqstepout = reqStepout(inp,out);
    if ( reqstepout ) stepout=*reqstepout;
    const BinID* desstepout = desStepout(inp,out);
    if ( desstepout )
    {
	stepout.inl = mMAX(stepout.inl,desstepout->inl);
	stepout.crl = mMAX(stepout.crl,desstepout->crl );
    }

    res.hrg.start.inl -= stepout.inl;
    res.hrg.start.crl -= stepout.crl;
    res.hrg.stop.inl += stepout.inl;
    res.hrg.stop.crl += stepout.crl;

    Interval<float> zrg(0,0);
    const Interval<float>* reqzrg = reqZMargin(inp,out);
    if ( reqzrg ) zrg=*reqzrg;
    const Interval<float>* deszrg = desZMargin(inp,out);
    if ( deszrg ) zrg.include( *deszrg );
    
    res.zrg.start += zrg.start;
    res.zrg.stop += zrg.stop;

    return true;
}


void Provider::updateInputReqs(int inp)
{
    if ( inp==-1 )
    {
	for ( int idx=0; idx<inputs.size(); idx++ )
	    updateInputReqs(idx);

	return;
    }

    CubeSampling inputcs;
    for ( int out=0; out<outputinterest.size(); out++ )
    {
	if ( !outputinterest[out] ) continue;

	if ( computeDesInputCube( inp, out, inputcs ) )
	    inputs[inp]->setDesiredVolume( inputcs );

	BinID stepout(0,0);
	const BinID* req = reqStepout(inp,out);
	if ( req ) stepout=*req;
	const BinID* des = desStepout(inp,out);
	if ( des )
	{
	    stepout.inl= mMAX(stepout.inl,des->inl);
	    stepout.crl =  mMAX(stepout.crl,des->crl );
	}

	inputs[inp]->setBufferStepout( stepout+bufferstepout );
    }
}

const BinID* Provider::desStepout(int,int) const { return 0; }
const BinID* Provider::reqStepout(int,int) const { return 0; }
const Interval<float>* Provider::desZMargin(int,int) const { return 0; }
const Interval<float>* Provider::reqZMargin(int,int) const { return 0; }

}; //namespace

