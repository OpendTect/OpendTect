/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: seistrctr.cc,v 1.45 2004-07-19 13:21:49 bert Exp $";

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
    datachar = trc.data().getInterpreter(icomp)->dataChar();
}


SeisTrcTranslator::SeisTrcTranslator( const char* nm, const char* unm )
	: Translator(nm,unm)
	, conn(0)
	, errmsg(0)
	, inpfor_(0)
	, nrout_(0)
	, inpcds(0)
	, outcds(0)
	, pinfo(0)
	, seldata(0)
    	, prevnr_(mUndefIntVal)
    	, trcblock_(*new SeisTrcBuf)
    	, lastinlwritten(SI().range().start.inl)
    	, enforce_regular_write(true)
{
    if ( getenv("DTECT_DONT_ENFORCE_REGULAR_WRITE") )
	enforce_regular_write = false;
}


SeisTrcTranslator::~SeisTrcTranslator()
{
    cleanUp();
    delete pinfo;
    delete &trcblock_;
}


bool SeisTrcTranslator::is2D( const IOObj& ioobj )
{
    return *ioobj.translator() == '2';
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
    delete pinfo; pinfo = 0;
}


void SeisTrcTranslator::close()
{
    if ( conn && !conn->forRead() )
	writeBlock();
    delete conn; conn = 0;
}


SeisPacketInfo& SeisTrcTranslator::packetInfo()
{
    if ( !pinfo ) pinfo = new SeisPacketInfo;
    return *pinfo;
}


bool SeisTrcTranslator::initRead( Conn* c )
{
    cleanUp();
    pinfo = new SeisPacketInfo;
    if ( !initConn(c,true)
      || !initRead_() )
    {
	conn = 0;
	return false;
    }

    pinfo->zrg.start = insd.start;
    pinfo->zrg.step = insd.step;
    pinfo->zrg.stop = insd.start + insd.step * (innrsamples-1);
    return true;
}


bool SeisTrcTranslator::initWrite( Conn* c, const SeisTrc& trc )
{
    cleanUp();
    pinfo = new SeisPacketInfo;
    innrsamples = outnrsamples = trc.size(0);
    insd = outsd = trc.info().sampling;

    if ( !initConn(c,false)
      || !initWrite_( trc ) )
    {
	conn = 0;
	return false;
    }

    return true;
}


bool SeisTrcTranslator::commitSelections( const SeisTrc* trc )
{
    errmsg = "No selected components found";
    const int sz = tarcds.size();
    if ( sz < 1 ) return false;

    outsd = insd; outnrsamples = innrsamples;
    if ( seldata )
    {
	Interval<float> zrg( seldata->zRange() );
	outsd.start = zrg.start;
	outsd.step = seldata->zrg_.step;
	outnrsamples = (int)(((zrg.stop-zrg.start) / outsd.step) + .5) + 1;
    }

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
    if ( !pinfo ) pinfo = new SeisPacketInfo;

    enforceBounds( trc );
    return commitSelections_();
}


void SeisTrcTranslator::enforceBounds( const SeisTrc* trc )
{
    static const float eps = SeisTrc::snapdist;

    SamplingData<float> avsd;
    avsd.start = trc ? trc->startPos( 0 ) : insd.start;
    avsd.step = trc ? trc->info().sampling.step : insd.step;
    int sz = trc ? trc->size( 0 ) : innrsamples;

    Interval<float> reqintv;
    reqintv.start = outsd.start;
    reqintv.stop = reqintv.start + outsd.step * (outnrsamples-1);
    const float avstop = avsd.start + avsd.step * (sz-1);
    if ( reqintv.start > reqintv.stop ) Swap(reqintv.start,reqintv.stop);
    if ( reqintv.start < avsd.start ) reqintv.start = avsd.start;
    if ( reqintv.stop > avstop ) reqintv.stop = avstop;

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

    float reqstep = outsd.step;
    sampdist = reqstep / avsd.step;
    intdist = (int)(sampdist + eps);
    if ( intdist < 1 ) intdist = 1;
    outsd.step = avsd.step * intdist;
    outsd.start = reqintv.start;
    float fnrsamps = (reqintv.stop - reqintv.start) / outsd.step + 1;
    outnrsamples = (int)(fnrsamps + eps);
}


void SeisTrcTranslator::fillOffsAzim( SeisTrcInfo& ti, const Coord& gp,
				      const Coord& sp )
{
    static bool warnfail = getenv( "DTECT_WARN_BINID_SRCOORDS" );
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

    bool wrblk = prevnr_ != trc.info().binid.inl && !mIsUndefInt(prevnr_);
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
				 const char* nm, int dtype )
{
    ComponentData* newcd = new ComponentData( nm );
    newcd->datachar = dc;
    newcd->datatype = dtype;
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
	newtrc->data().addComponent( outnrsamples,
				     outcds[idx]->datachar, true );
	newtrc->info().sampling = outsd;
    }

    return newtrc;
}


bool SeisTrcTranslator::getRanges( const IOObj& ioobj, SeisSelData& sd )
{
    PtrMan<Translator> transl = ioobj.getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,tr,transl.ptr());
    if ( !tr ) return false;
    Conn* cnn = ioobj.getConn( Conn::Read );
    if ( !cnn || !tr->initRead(cnn) )
	{ delete cnn; return false; }

    const SeisPacketInfo& pinf = tr->packetInfo();
    sd.type_ = SeisSelData::Range;
    sd.inlrg_ = pinf.inlrg; sd.crlrg_ = pinf.crlrg; sd.zrg_ = pinf.zrg;
    return true;
}
