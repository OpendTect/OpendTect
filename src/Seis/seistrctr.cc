/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: seistrctr.cc,v 1.6 2001-04-13 11:51:11 bert Exp $";

#include "seistrctr.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "iopar.h"
#include "sorting.h"
#include "separstr.h"


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


IOObjContext SeisTrcTranslator::ioContext()
{
    IOObjContext ctxt( Translator::groups()[listid] );
    ctxt.crlink = false;
    ctxt.newonlevel = 2;
    ctxt.needparent = false;
    ctxt.stdseltype = IOObjContext::Seis;
    ctxt.multi = true;

    return ctxt;
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
	, pinfo(*new SeisPacketInfo)
	, storediopar(*new IOPar)
	, useinpsd(false)
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


bool SeisTrcTranslator::commitSelections()
{
    errmsg = "No selected components found";
    const int sz = tarcds.size();
    if ( sz < 1 ) return false;

    int selnrs[sz], inpnrs[sz];
    int nrsel = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	selnrs[nrsel] = tarcds[idx]->destidx;
	if ( selnrs[idx] >= 0 )
	{
	    inpnrs[nrsel] = idx;
	    nrsel++;
	}
    }

    nrout_ = nrsel; if ( nrout_ < 1 ) nrout_ = 1;
    delete [] inpfor_; inpfor_ = new int [nrout_];
    if ( nrsel < 1 )
	{ inpfor_[0] = 0; return false; }

    sort_coupled( selnrs, inpnrs, nrsel );
    for ( int idx=0; idx<nrout_; idx++ )
	inpfor_[idx] = inpnrs[idx];

    errmsg = 0;
    return commitSelections_();
}


void SeisTrcTranslator::usePar( const IOPar* iopar )
{
    if ( !iopar ) return;

    int nr = 1;
    BufferString nrstr;
    while ( 1 )
    {
	nrstr = "."; nrstr += nr;

	BufferString keystr = "Positions"; keystr += nrstr;
	const char* res = iopar->find( (const char*)keystr );
	if ( !res && nr == 1 )
	     res = iopar->find( "Positions" );
	if ( !res ) break;
	storediopar.set( keystr, res );

	keystr = "Name"; keystr += nrstr;
	const char* nm = iopar->find( (const char*)keystr );
	storediopar.set( "Name", nm );

	keystr = "Index"; keystr += nrstr;
	storediopar.set( keystr, iopar->find( (const char*)keystr ) );
	keystr = "Data characteristics"; keystr += nrstr;
	storediopar.set( keystr, iopar->find( (const char*)keystr ) );
	nr++;
    }
}


void SeisTrcTranslator::useStoredPar()
{
    int nr = 1;
    BufferString nrstr;
    while ( 1 )
    {
	if ( cds.size() < nr ) break;
	TargetComponentData* tcd = tarcds[nr-1];
	nrstr = "."; nrstr += nr;

	BufferString keystr( "Name" ); keystr += nrstr;
	const char* nm = storediopar.find( (const char*)keystr );
	if ( nm ) tcd->setName( nm );

	keystr = "Positions"; keystr += nrstr;
	const char* res = storediopar.find( (const char*)keystr );
	if ( res )
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
		if ( *res ) posns.start = 0.001 * atof( res );
	    }
	    if ( sz > 1 )
	    {
		const char* res = fms[1];
		if ( *res ) posns.stop = 0.001 * atof( res );
	    }
	    if ( sz > 2 )
	    {
		const char* res = fms[2];
		if ( *res ) posns.step = 0.001 * atof( res );
	    }
	    tcd->sd.start = posns.start;
	    tcd->sd.step = posns.step;
	    tcd->nrsamples = posns.nrSteps();
	}

	keystr = "Index"; keystr += nrstr;
	res = storediopar.find( (const char*)keystr );
	if ( res )
	    tcd->destidx = atoi( res );

	keystr = "Data characteristics"; keystr += nrstr;
	res = storediopar.find( (const char*)keystr );
	if ( res )
	    tcd->datachar.set( res );

	nr++;
    }
}


void SeisTrcTranslator::addComp( const DataCharacteristics& dc,
				 const SamplingData<float>& s,
				 int ns, const char* nm )
{
    ComponentData* newcd = new ComponentData( nm );
    newcd->sd = s;
    newcd->nrsamples = ns;
    newcd->datachar = dc;
    newcd->datachar.littleendian = __islittle__;
    cds += newcd;
    tarcds += new TargetComponentData( *newcd, cds.size()-1 );
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
