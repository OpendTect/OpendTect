/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: seistrctr.cc,v 1.37 2003-10-15 15:15:54 bert Exp $";

#include "seistrctr.h"
#include "seisfact.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seisbuf.h"
#include "iopar.h"
#include "ioobj.h"
#include "sorting.h"
#include "separstr.h"
#include "scaler.h"
#include "ptrman.h"
#include "survinfo.h"
#include "errh.h"
#include <math.h>


int SeisTrcTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Integration Framework",key)
      || defaultSelector("Well group",key)
      || defaultSelector("Seismic directory",key) )
	return 1;

    return 0;
}


const IOObjContext& SeisTrcTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->needparent = false;
	ctxt->stdseltype = IOObjContext::Seis;
	ctxt->multi = true;
	ctxt->newonlevel = 1;
	ctxt->maychdir = false;
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


SeisTrcTranslator::SeisTrcTranslator( const char* nm, const char* unm )
	: Translator(nm,unm)
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
    	, prevnr_(-999)
    	, trcblock_(*new SeisTrcBuf)
    	, lastinlwritten(SI().range().start.inl)
    	, enforce_regular_write(true)
{
    if ( getenv("dGB_DONT_ENFORCE_REGULAR_WRITE") )
	enforce_regular_write = false;
}


SeisTrcTranslator::~SeisTrcTranslator()
{
    cleanUp();
    delete &pinfo;
    delete &storediopar;
    delete &trcblock_;
}


void SeisTrcTranslator::cleanUp()
{
    close();

    deepErase( cds );
    deepErase( tarcds );
    delete [] inpfor_; inpfor_ = 0;
    delete [] inpcds; inpcds = 0;
    delete [] outcds; outcds = 0;
    nrout_ = 0;
    errmsg = 0;
    useinpsd = false;
    pinfo = SeisPacketInfo();
}


void SeisTrcTranslator::close()
{
    if ( conn && !conn->forRead() )
	writeBlock();
    delete conn; conn = 0;
}


bool SeisTrcTranslator::initRead( Conn* c )
{
    cleanUp();
    if ( !initConn(c,true)
      || !initRead_() )
    {
	conn = 0;
	return false;
    }

    useStoredPar();
    return true;
}


bool SeisTrcTranslator::initWrite( Conn* c, const SeisTrc& trc )
{
    cleanUp();
    if ( !initConn(c,false)
      || !initWrite_( trc ) )
    {
	conn = 0;
	return false;
    }

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
    static const float eps = SeisTrc::snapdist;

    for ( int idx=0; idx<nrout_; idx++ )
    {
	SamplingData<float> avsd;
	avsd.start = trc ? trc->startPos( idx ) : inpcds[idx]->sd.start;
	avsd.step = trc ? trc->info().sampling.step : inpcds[idx]->sd.step;
	int sz = trc ? trc->size( idx ) : inpcds[idx]->nrsamples;

	Interval<float> reqintv;
	reqintv.start = outcds[idx]->sd.start;
	reqintv.stop = reqintv.start
		     + outcds[idx]->sd.step * (outcds[idx]->nrsamples-1);
	const float avstop = avsd.start + avsd.step * (sz-1);
	if ( reqintv.start > reqintv.stop ) Swap(reqintv.start,reqintv.stop);

	if ( reqintv.start < avsd.start )
	    reqintv.start = avsd.start;
	if ( reqintv.stop > avstop )
	    reqintv.stop = avstop;

	// If requested start not on a sample, make sure it is
	// First, the start time:
	float sampdist = (reqintv.start - avsd.start) / avsd.step;
	int intdist = mNINT(sampdist);
	if ( !trc )
	{
	    sampdist -= intdist;
	    if ( sampdist < -eps || sampdist > eps )
	    {
		// Reading and not on sample: read more to allow interpolation
		sampdist += intdist - 1.5;
		intdist = sampdist < 0 ? 0 : mNINT(sampdist);
	    }
	}
	reqintv.start = avsd.start + avsd.step * intdist;

	// Then, the stop time:
	sampdist = (avstop - reqintv.stop) / avsd.step;
	intdist = mNINT(sampdist);
	if ( !trc )
	{
	    sampdist -= intdist;
	    if ( sampdist < -eps || sampdist > eps )
	    {
		// Reading and not on sample: read more to allow interpolation
		sampdist += intdist - 1.5;
		intdist = sampdist < 0 ? 0 : mNINT(sampdist);
	    }
	}
	reqintv.stop = avstop - avsd.step * intdist;

	if ( reqintv.start > reqintv.stop ) Swap(reqintv.start,reqintv.stop);

	float reqstep = outcds[idx]->sd.step;
	sampdist = reqstep / avsd.step;
	intdist = (int)(sampdist + eps);
	if ( intdist < 1 ) intdist = 1;
	outcds[idx]->sd.step = avsd.step * intdist;
	outcds[idx]->sd.start = reqintv.start;
	float fnrsamps = (reqintv.stop - reqintv.start) / outcds[idx]->sd.step
	    		+ 1;
	outcds[idx]->nrsamples = (int)(fnrsamps + eps);
    }
}


void SeisTrcTranslator::fillOffsAzim( SeisTrcInfo& ti, const Coord& gp,
				      const Coord& sp )
{
    static bool warnfail = getenv( "dGB_WARN_BINID_SRCOORDS" );
    ti.offset = gp.distance( sp );
    ti.azimuth = atan2( gp.y - sp.y, gp.x - sp.x );
    if ( warnfail )
    {
	Coord pos( .5 * (gp.x + sp.x), .5 * (gp.y + sp.y) );
	BinID bid( SI().transform(pos) );
	if ( bid != ti.binid )
	{
	    BufferString msg( "S/R posns don't match Inl/Crl. Hdr=" );
	    msg += ti.binid.inl; msg += "/"; msg += ti.binid.crl;
	    msg += " S/R="; msg += bid.inl; msg += "/"; msg += bid.crl;
	    ErrMsg( msg );
	}
    }
}


bool SeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !inpfor_ ) commitSelections( &trc );

    if ( !inlCrlSorted() )
    {
	// No buffering: who knows what we'll get?
	dumpBlock();
	return writeTrc_( trc );
    }

    bool wrblk = prevnr_ != trc.info().binid.inl && prevnr_ != -999;
    prevnr_ = trc.info().binid.inl;
    if ( wrblk && !writeBlock() )
	return false;

    SeisTrc* newtrc = new SeisTrc(trc);
    if ( !newtrc )
	{ errmsg = "Out of memory"; return false; }
    trcblock_.add( newtrc );

    return true;
}


bool SeisTrcTranslator::writeBlock()
{
    int sz = trcblock_.size();
    SeisTrc* firsttrc = sz ? trcblock_.get(0) : 0;
    const int firstcrl = sz ? firsttrc->info().binid.crl : -1;
    StepInterval<int> crlrg(0,0,SI().getStep(false,false));
    bool upwrd = sz < 2 || firstcrl < trcblock_.get(sz-1)->info().binid.crl;

    if ( prepareWriteBlock(crlrg,upwrd) )
    {
	if ( firsttrc )
	    lastinlwritten = firsttrc->info().binid.inl;

	if ( sz && enforce_regular_write )
	{
	    trcblock_.sort( upwrd );
	    int nrperpos = 1;
	    for ( int idx=1; idx<sz; idx++ )
	    {
		if ( trcblock_.get(idx)->info().binid.crl != firstcrl )
		    break;
		nrperpos++;
	    }
	    trcblock_.enforceNrTrcs( nrperpos );
	    sz = trcblock_.size();
	}

	if ( !crlrg.start && !crlrg.stop )
	    dumpBlock();
	else
	{
	    const int firstafter = upwrd ? crlrg.stop + crlrg.step
					 : crlrg.start + crlrg.step;
	    int stp = upwrd ? crlrg.step : -crlrg.step;
	    int bufidx = 0;
	    SeisTrc* trc = bufidx < sz ? trcblock_.get(bufidx) : 0;
	    BinID binid( lastinlwritten, upwrd ? crlrg.start : crlrg.stop );
	    SeisTrc* filltrc = 0;
	    for ( binid.crl; binid.crl != firstafter; binid.crl += stp )
	    {
		while ( trc
		     && ( (upwrd && trc->info().binid.crl < binid.crl)
		       || (!upwrd && trc->info().binid.crl > binid.crl) ) )
		{
		    bufidx++;
		    trc = bufidx < sz ? trcblock_.get(bufidx) : 0;
		}
		if ( trc )
		{
		    if ( !writeTrc_(*trc) )
			return false;
		}
		else
		{
		    if ( !filltrc )
			filltrc = getFilled( binid );
		    else
		    {
			filltrc->info().binid = binid;
			filltrc->info().coord = SI().transform(binid);
		    }
		    if ( !writeTrc_(*filltrc) )
			return false;
		}
	    }
	    delete filltrc;
	    trcblock_.deepErase();
	}
    }

    return true;
}


bool SeisTrcTranslator::dumpBlock()
{
    bool rv = true;
    for ( int idx=0; idx<trcblock_.size(); idx++ )
    {
	if ( !writeTrc_(*trcblock_.get(idx)) )
	    { rv = false; break; }
    }
    trcblock_.deepErase();
    return rv;
}


void SeisTrcTranslator::usePar( const IOPar& iopar )
{
    int nr = 0;
    BufferString nrstr;
    while ( 1 )
    {
	if ( !nr )	nrstr = "";
	else		{ nrstr = "."; nrstr += nr; }
	nr++;

	BufferString keystr = SurveyInfo::sKeyZRange; keystr += nrstr;
	const char* res = iopar.find( (const char*)keystr );
	if ( !res )
	{
	    if ( nr > 1 )	break;
	    else		continue;
	}
	storediopar.set( keystr, res );

	keystr = "Name"; keystr += nrstr;
	const char* nm = iopar.find( (const char*)keystr );
	storediopar.set( "Name", nm );

	keystr = "Index"; keystr += nrstr;
	storediopar.set( keystr, iopar.find( (const char*)keystr ) );
	keystr = "Data characteristics"; keystr += nrstr;
	storediopar.set( keystr, iopar.find( (const char*)keystr ) );
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


void SeisTrcTranslator::prepareComponents( SeisTrc& trc, int actualsz ) const
{
    for ( int idx=0; idx<nrout_; idx++ )
    {
        TraceData& td = trc.data();
        if ( td.nrComponents() <= idx )
            td.addComponent( actualsz, tarcds[ inpfor_[idx] ]->datachar );
        else
        {
            td.setComponent( tarcds[ inpfor_[idx] ]->datachar, idx );
            td.reSize( actualsz, idx );
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


bool SeisTrcTranslator::initConn( Conn* c, bool forread )
{
    close(); errmsg = "";

    if ( !c )
    {
	errmsg = "Translator error: No connection established";
	return false;
    }

    if ( ((forread && c->forRead()) || (!forread && c->forWrite()) )
      && !strcmp(c->connType(),connType()) )
	conn = c;
    else
    {
	errmsg = "Translator error: Bad connection established";
	return false;
    }

    return true;
}


SeisTrc* SeisTrcTranslator::getEmpty()
{
    DataCharacteristics dc;
    if ( outcds )
	dc = outcds[0]->datachar;
    else if ( tarcds.size() && inpfor_ )
	dc = tarcds[selComp()]->datachar;
    else
	toSupported( dc );

    return new SeisTrc( 0, dc );
}


SeisTrc* SeisTrcTranslator::getFilled( const BinID& binid )
{
    if ( !outcds )
	return 0;

    SeisTrc* newtrc = new SeisTrc;
    newtrc->info().binid = binid;
    newtrc->info().coord = SI().transform( binid );

    newtrc->data().delComponent(0);
    for ( int idx=0; idx<nrout_; idx++ )
    {
	newtrc->data().addComponent( outcds[idx]->nrsamples,
				     outcds[idx]->datachar, true );
	newtrc->info().sampling = outcds[idx]->sd;
    }

    return newtrc;
}


bool SeisTrcTranslator::getRanges( const IOObj& ioobj, BinIDSampler& bs,
				   StepInterval<float>& zrg )
{
    PtrMan<Translator> transl = ioobj.getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,tr,transl.ptr());
    if ( !tr ) return false;
    Conn* cnn = ioobj.getConn( Conn::Read );
    if ( !cnn || !tr->initRead(cnn) )
	{ delete cnn; return false; }

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
