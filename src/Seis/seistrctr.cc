/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: seistrctr.cc,v 1.27 2002-01-24 21:41:56 bert Exp $";

#include "seistrctr.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "iopar.h"
#include "ioobj.h"
#include "sorting.h"
#include "separstr.h"
#include "scaler.h"
#include "ptrman.h"
#include "survinfo.h"


int SeisTrcTranslator::selector( const char* key )
{
    int retval = defaultSelector( classdef.name(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Integration Framework",key)
      || defaultSelector("Well group",key)
      || defaultSelector("Seismic directory",key) )
	return 1;

    return 0;
}


const IOObjContext& SeisTrcTranslator::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( Translator::groups()[listid] );
	ctxt->crlink = false;
	ctxt->needparent = false;
	ctxt->stdseltype = IOObjContext::Seis;
	ctxt->multi = true;
	if ( GetDgbApplicationCode() == mDgbApplCodeGDI )
	    ctxt->newonlevel = 2;
	else
	{
	    ctxt->newonlevel = 1;
	    ctxt->maychdir = false;
	}
    }

    return *ctxt;
}


SeisTrcTranslator::ComponentData::ComponentData( const SeisTrc& trc, int icomp,
						 const char* nm )
	: BasicComponentInfo(nm)
{
    sd = trc.samplingData( icomp );
    datachar = trc.data().getInterpreter(icomp)->dataChar();
    nrsamples = trc.size( icomp );
}


SeisTrcTranslator::SeisTrcTranslator( const char* nm, const ClassDef* cd )
	: Translator(nm,cd)
	, conn(0)
	, errmsg(0)
	, inpfor_(0)
	, nrout_(0)
	, inpcds(0)
	, outcds(0)
	, pinfo(*new SeisPacketInfo)
	, storediopar(*new IOPar)
	, useinpsd(false)
	, trcsel(0)
{
}


SeisTrcTranslator::~SeisTrcTranslator()
{
    cleanUp();
    delete &pinfo;
    delete &storediopar;
}


void SeisTrcTranslator::cleanUp()
{
    deepErase( cds );
    deepErase( tarcds );
    delete [] inpfor_; inpfor_ = 0;
    delete [] inpcds; inpcds = 0;
    delete [] outcds; outcds = 0;
    nrout_ = 0;
    conn = 0;
    errmsg = 0;
    useinpsd = false;
    pinfo = SeisPacketInfo();
}


bool SeisTrcTranslator::initRead( Conn& c )
{
    cleanUp();
    if ( !initConn(c,true)
      || !initRead_() ) return false;
    useStoredPar();
    return true;
}


bool SeisTrcTranslator::initWrite( Conn& c, const SeisTrc& trc )
{
    cleanUp();
    if ( !initConn(c,false)
      || !initWrite_( trc ) ) return false;
    useStoredPar();
    return true;
}


bool SeisTrcTranslator::commitSelections( const SeisTrc* trc )
{
    errmsg = "No selected components found";
    const int sz = tarcds.size();
    if ( sz < 1 ) return false;

    ArrPtrMan<int> selnrs = new int[sz];
    ArrPtrMan<int> inpnrs = new int[sz];
    int nrsel = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	int destidx = tarcds[idx]->destidx;
	if ( destidx >= 0 )
	{
	    selnrs[nrsel] = destidx;
	    inpnrs[nrsel] = idx;
	    nrsel++;
	}
    }

    nrout_ = nrsel;
    if ( nrout_ < 1 ) nrout_ = 1;
    delete [] inpfor_; inpfor_ = new int [nrout_];
    if ( nrsel < 1 )
	inpfor_[0] = 0;
    else if ( nrsel == 1 )
	inpfor_[0] = inpnrs[0];
    else
    {
	sort_coupled( (int*)selnrs, (int*)inpnrs, nrsel );
	for ( int idx=0; idx<nrout_; idx++ )
	    inpfor_[idx] = inpnrs[idx];
    }

    inpcds = new ComponentData* [nrout_];
    outcds = new TargetComponentData* [nrout_];
    for ( int idx=0; idx<nrout_; idx++ )
    {
	inpcds[idx] = cds[ selComp(idx) ];
	outcds[idx] = tarcds[ selComp(idx) ];
    }

    errmsg = 0;
    enforceBounds( trc );
    return commitSelections_();
}


void SeisTrcTranslator::enforceBounds( const SeisTrc* trc )
{
    const float eps = 1e-6;

    for ( int idx=0; idx<nrout_; idx++ )
    {
	SamplingData<float> sd;
	sd.start = trc ? trc->startPos( idx ) : inpcds[idx]->sd.start;
	sd.step = trc ? trc->info().sampling.step : inpcds[idx]->sd.step;
	int sz = trc ? trc->size( idx ) : inpcds[idx]->nrsamples;

	const float reqstop = outcds[idx]->sd.start
			    + outcds[idx]->sd.step * (outcds[idx]->nrsamples-1);
	const float avstop = sd.start + sd.step * (sz-1);
	bool neednewnrsamples = avstop+eps < reqstop;

	if ( outcds[idx]->sd.start+eps < sd.start )
	{
	    outcds[idx]->sd.start = sd.start;
	    neednewnrsamples = true;
	}
	else if ( mIS_ZERO(outcds[idx]->sd.start-sd.start) )
	    outcds[idx]->sd.start = sd.start;

	if ( outcds[idx]->sd.step+eps < sd.step )
	{
	    outcds[idx]->sd.step = sd.step;
	    neednewnrsamples = true;
	}
	else if ( mIS_ZERO(outcds[idx]->sd.step-sd.step) )
	    outcds[idx]->sd.step = sd.step;

	if ( neednewnrsamples )
	{
	    const float stop = avstop+eps < reqstop ? avstop : reqstop;
	    float fnrsamps = (stop - outcds[idx]->sd.start)
			     / outcds[idx]->sd.step + 1 + eps;
	    outcds[idx]->nrsamples = (int)fnrsamps;
	}
    }
}


void SeisTrcTranslator::usePar( const IOPar* iopar )
{
    if ( !iopar ) return;

    int nr = 0;
    BufferString nrstr;
    while ( 1 )
    {
	if ( !nr )	nrstr = "";
	else		{ nrstr = "."; nrstr += nr; }
	nr++;

	BufferString keystr = SurveyInfo::sKeyZRange; keystr += nrstr;
	const char* res = iopar->find( (const char*)keystr );
	if ( !res )
	{
	    if ( nr > 1 )	break;
	    else		continue;
	}
	storediopar.set( keystr, res );

	keystr = "Name"; keystr += nrstr;
	const char* nm = iopar->find( (const char*)keystr );
	storediopar.set( "Name", nm );

	keystr = "Index"; keystr += nrstr;
	storediopar.set( keystr, iopar->find( (const char*)keystr ) );
	keystr = "Data characteristics"; keystr += nrstr;
	storediopar.set( keystr, iopar->find( (const char*)keystr ) );
    }
}


void SeisTrcTranslator::useStoredPar()
{
    if ( cds.size() < 1 ) return;

    int nr = 0;
    BufferString nrstr;
    while ( 1 )
    {
	if ( cds.size() < nr ) break;
	if ( !nr )
	    nrstr = "";
	else
	    { nrstr = "."; nrstr += nr; }
	TargetComponentData* tcd = tarcds[nr ? nr-1 : 0];
	nr++;

	BufferString keystr( "Name" ); keystr += nrstr;
	const char* nm = storediopar.find( (const char*)keystr );
	if ( nm && *nm ) tcd->setName( nm );

	keystr = SurveyInfo::sKeyZRange; keystr += nrstr;
	const char* res = storediopar.find( (const char*)keystr );
	if ( res && *res )
	{
	    FileMultiString fms( res );
	    const int sz = fms.size();
	    StepInterval<float> posns;
	    posns.start = tcd->sd.start;
	    posns.stop = tcd->sd.start + tcd->sd.step * (tcd->nrsamples-1);
	    posns.step = tcd->sd.step;
	    if ( sz > 0 )
	    {
		const char* res = fms[0];
		if ( *res ) posns.start = atof( res );
	    }
	    if ( sz > 1 )
	    {
		const char* res = fms[1];
		if ( *res ) posns.stop = atof( res );
	    }
	    if ( sz > 2 )
	    {
		const char* res = fms[2];
		if ( *res ) posns.step = atof( res );
	    }
	    tcd->sd.start = posns.start;
	    tcd->sd.step = posns.step;
	    tcd->nrsamples = posns.nrSteps() + 1;
	}

	keystr = "Index"; keystr += nrstr;
	res = storediopar.find( (const char*)keystr );
	if ( res && *res )
	    tcd->destidx = atoi( res );

	keystr = "Data characteristics"; keystr += nrstr;
	res = storediopar.find( (const char*)keystr );
	if ( res && *res )
	    tcd->datachar.set( res );
    }
}


void SeisTrcTranslator::prepareComponents( SeisTrc& trc,
					   const int* actualsz ) const
{
    for ( int idx=0; idx<nrout_; idx++ )
    {
        TraceData& td = trc.data();
        if ( td.nrComponents() <= idx )
            td.addComponent( actualsz[idx], tarcds[ inpfor_[idx] ]->datachar );
        else
        {
            td.setComponent( tarcds[ inpfor_[idx] ]->datachar, idx );
            td.reSize( actualsz[idx], idx );
        }
    }
}



void SeisTrcTranslator::addComp( const DataCharacteristics& dc,
				 const SamplingData<float>& s,
				 int ns, const char* nm, const LinScaler* sc,
				 int dtype )
{
    ComponentData* newcd = new ComponentData( nm );
    newcd->sd = s;
    newcd->nrsamples = ns;
    newcd->datachar = dc;
    newcd->datatype = dtype;
    newcd->scaler = sc ? (LinScaler*)sc->duplicate() : 0;
    cds += newcd;
    bool isl = newcd->datachar.littleendian;
    newcd->datachar.littleendian = __islittle__;
    tarcds += new TargetComponentData( *newcd, cds.size()-1 );
    newcd->datachar.littleendian = isl;
}


bool SeisTrcTranslator::initConn( Conn& c, bool forread )
{
    conn = 0; errmsg = 0;
    if ( ((forread && c.forRead()) || (!forread && c.forWrite()) )
      && &c.classDef() == &connClassDef() )
	conn = &c;
    else
    {
	errmsg = "Internal error: Bad connection established";
	return false;
    }

    errmsg = 0;
    return true;
}


SeisTrc* SeisTrcTranslator::getEmpty()
{
    DataCharacteristics dc;
    if ( tarcds.size() )
    {
	if ( !inpfor_ ) commitSelections();
	dc = tarcds[selComp()]->datachar;
    }
    else
	toSupported( dc );
    return new SeisTrc( 0, dc );
}


bool SeisTrcTranslator::getRanges( const IOObj& ioobj, BinIDSampler& bs,
				   StepInterval<float>& zrg )
{
    PtrMan<Translator> transl = ioobj.getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,tr,transl.ptr());
    if ( !tr ) return false;
    PtrMan<Conn> conn = ioobj.getConn( Conn::Read );
    if ( !conn || !tr->initRead(*conn) ) return false;

    const SeisPacketInfo& pinfo = tr->packetInfo();
    bs.copyFrom( pinfo.binidsampling );
    bs.step = pinfo.binidsampling.step;

    assign( zrg, SI().zRange() );
    const ObjectSet<TargetComponentData>& cinfo = tr->componentInfo();
    if ( !cinfo.size() ) return true;

    const TargetComponentData& cd = *cinfo[0];
    zrg = cd.sd.interval( cd.nrsamples );
    return true;
}
