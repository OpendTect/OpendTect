/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id$";

#include "seistrctr.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "sorting.h"
#include "separstr.h"
#include "scaler.h"
#include "ptrman.h"
#include "survinfo.h"
#include "bufstringset.h"
#include "cubesampling.h"
#include "envvars.h"
#include "errh.h"
#include "file.h"
#include <math.h>


const char* SeisTrcTranslator::sKeyIs2D()	{ return "Is2D"; }
const char* SeisTrcTranslator::sKeyIsPS()	{ return "IsPS"; }
const char* SeisTrcTranslator::sKeyRegWrite()
					{ return "Enforce Regular Write"; }
const char* SeisTrcTranslator::sKeySIWrite()
					{ return "Enforce SurveyInfo Write"; }


SeisTrcTranslator::ComponentData::ComponentData( const SeisTrc& trc, int icomp,
						 const char* nm )
	: BasicComponentInfo(nm)
{
    datachar = trc.data().getInterpreter(icomp)->dataChar();
}


static bool defnowrreg = GetEnvVarYN("OD_NO_SEISWRITE_REGULARISATION");
static bool defsurvinfwr = GetEnvVarYN("OD_ENFORCE_SURVINFO_SEISWRITE");

SeisTrcTranslator::SeisTrcTranslator( const char* nm, const char* unm )
	: Translator(nm,unm)
	, conn(0)
	, errmsg(0)
	, inpfor_(0)
	, nrout_(0)
	, inpcds(0)
	, outcds(0)
	, seldata(0)
    	, prevnr_(mUdf(int))
    	, pinfo(*new SeisPacketInfo)
    	, trcblock_(*new SeisTrcBuf(false))
    	, lastinlwritten(SI().sampling(false).hrg.start.inl)
    	, read_mode(Seis::Prod)
    	, is_prestack(false)
    	, is_2d(false)
    	, enforce_regular_write(!defnowrreg) // default true
    	, enforce_survinfo_write(defsurvinfwr)// default false
	, compnms_(0)
	, warnings_(*new BufferStringSet)
{
}


SeisTrcTranslator::~SeisTrcTranslator()
{
    cleanUp();
    delete &trcblock_;
    delete &pinfo;
    delete &warnings_;
}


bool SeisTrcTranslator::is2D( const IOObj& ioobj, bool internal_only )
{
    const bool trok = *ioobj.group() == '2' || *ioobj.translator() == '2';
    return trok || internal_only ? trok : ioobj.pars().isTrue( sKeyIs2D() );
}


bool SeisTrcTranslator::isPS( const IOObj& ioobj )
{
    return *ioobj.group() == 'P' || *(ioobj.group()+3) == 'P';
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
}


bool SeisTrcTranslator::close()
{
    bool ret = true;
    if ( conn && !conn->forRead() )
	ret = writeBlock();
    delete conn; conn = 0;
    return ret;
}


bool SeisTrcTranslator::initRead( Conn* c, Seis::ReadMode rm )
{
    cleanUp();
    read_mode = rm;
    if ( !initConn(c,true)
      || !initRead_() )
    {
	delete conn; conn = 0;
	return false;
    }

    pinfo.zrg.start = insd.start;
    pinfo.zrg.step = insd.step;
    pinfo.zrg.stop = insd.start + insd.step * (innrsamples-1);
    return true;
}


bool SeisTrcTranslator::initWrite( Conn* c, const SeisTrc& trc )
{
    cleanUp();

    innrsamples = outnrsamples = trc.size();
    if ( innrsamples < 1 )
	{ errmsg = "Empty first trace"; return false; }

    insd = outsd = trc.info().sampling;

    if ( !initConn(c,false)
      || !initWrite_( trc ) )
    {
	delete conn; conn = 0;
	return false;
    }

    return true;
}


bool SeisTrcTranslator::commitSelections()
{
    errmsg = "No selected components found";
    const int sz = tarcds.size();
    if ( sz < 1 ) return false;

    outsd = insd; outnrsamples = innrsamples;
    if ( seldata && !mIsUdf(seldata->zRange().start) )
    {
	Interval<float> selzrg( seldata->zRange() );
	const Interval<float> sizrg( SI().sampling(false).zrg );
	if ( !mIsEqual(selzrg.start,sizrg.start,1e-8)
	  || !mIsEqual(selzrg.stop,sizrg.stop,1e-8) )
	{
// Does not work for Z-axis transformed scenes. Is it required?
//	    SI().snapZ( selzrg.start, -1 ); SI().snapZ( selzrg.stop, 1 );
	    outsd.start = selzrg.start;
	    const float fnrsteps = (selzrg.stop-selzrg.start) / outsd.step;
	    outnrsamples = mNINT32(fnrsteps) + 1;
	}
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
    enforceBounds();

    float fsampnr = (outsd.start - insd.start) / insd.step;
    samps.start = mNINT32( fsampnr );
    samps.stop = samps.start + outnrsamples - 1;

    is_2d = !conn || !conn->ioobj ? false : is2D( *conn->ioobj, false );
    return commitSelections_();
}


void SeisTrcTranslator::enforceBounds()
{
    // Ranges
    outsd.step = insd.step;
    float outstop = outsd.start + (outnrsamples - 1) * outsd.step;
    if ( outsd.start < insd.start )
	outsd.start = insd.start;
    const float instop = insd.start + (innrsamples - 1) * insd.step;
    if ( outstop > instop )
	outstop = instop;

    // Snap to samples
    float sampdist = (outsd.start - insd.start) / insd.step;
    int startsamp = (int)(sampdist + 0.0001);
    if ( startsamp < 0 ) startsamp = 0;
    if ( startsamp > innrsamples-1 ) startsamp = innrsamples-1;
    sampdist = (outstop - insd.start) / insd.step;
    int endsamp = (int)(sampdist + 0.9999);
    if ( endsamp < startsamp ) endsamp = startsamp;
    if ( endsamp > innrsamples-1 ) endsamp = innrsamples-1;

    outsd.start = insd.start + startsamp * insd.step;
    outnrsamples = endsamp - startsamp + 1;
}


bool SeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !inpfor_ && !commitSelections() )
	return false;

    if ( !inlCrlSorted() )
    {
	// No buffering: who knows what we'll get?
	dumpBlock();
	return writeTrc_( trc );
    }

    const bool haveprev = !mIsUdf( prevnr_ );
    const bool wrblk = haveprev && (is_2d ? prevnr_ > 99
				: prevnr_ != trc.info().binid.inl);
    if ( !is_2d )
	prevnr_ = trc.info().binid.inl;
    else if ( wrblk || !haveprev )
	prevnr_ = 1;
    else
	prevnr_++;

    if ( wrblk && !writeBlock() )
	return false;

    SeisTrc* newtrc; mTryAlloc( newtrc, SeisTrc(trc) );
    if ( !newtrc )
	{ errmsg = "Out of memory"; trcblock_.deepErase(); return false; }
    trcblock_.add( newtrc );

    return true;
}


bool SeisTrcTranslator::writeBlock()
{
    int sz = trcblock_.size();
    SeisTrc* firsttrc = sz ? trcblock_.get(0) : 0;
    if ( firsttrc )
	lastinlwritten = firsttrc->info().binid.inl;

    if ( sz && enforce_regular_write )
    {
	SeisTrcInfo::Fld keyfld = is_2d ? SeisTrcInfo::TrcNr
	    				: SeisTrcInfo::BinIDCrl;
	bool sort_asc = true;
	if ( is_2d )
	    sort_asc = trcblock_.get(0)->info().nr
		     < trcblock_.get(trcblock_.size()-1)->info().nr;
	    // for 2D we're buffering only 100 traces, not an entire line
	trcblock_.sort( sort_asc, keyfld );
	firsttrc = trcblock_.get( 0 );
	const int firstnr = is_2d ? firsttrc->info().nr
	    			 : firsttrc->info().binid.crl;
	int nrperpos = 1;
	if ( !is_2d )
	{
	    for ( int idx=1; idx<sz; idx++ )
	    {
		if ( trcblock_.get(idx)->info().binid.crl != firstnr )
		    break;
		nrperpos++;
	    }
	}
	trcblock_.enforceNrTrcs( nrperpos, keyfld );
	sz = trcblock_.size();
    }

    if ( !enforce_survinfo_write )
	return dumpBlock();

    StepInterval<int> inlrg, crlrg;
    SI().sampling(true).hrg.get( inlrg, crlrg );
    const int firstafter = crlrg.stop + crlrg.step;
    int stp = crlrg.step;
    int bufidx = 0;
    SeisTrc* trc = bufidx < sz ? trcblock_.get(bufidx) : 0;
    BinID binid( lastinlwritten, crlrg.start );
    SeisTrc* filltrc = 0;
    int nrwritten = 0;
    for ( ; binid.crl != firstafter; binid.crl += stp )
    {
	while ( trc && trc->info().binid.crl < binid.crl )
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
	nrwritten++;
    }

    delete filltrc;
    trcblock_.deepErase();
    blockDumped( nrwritten );
    return true;
}


bool SeisTrcTranslator::dumpBlock()
{
    bool rv = true;
    const int sz = trcblock_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !writeTrc_(*trcblock_.get(idx)) )
	    { rv = false; break; }
    }
    trcblock_.deepErase();
    blockDumped( sz );
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
    BufferString str( "Component " );
    if ( !nm || !*nm )
    {
	if ( compnms_ && cds.size() < compnms_->size() )
	    nm = compnms_->get(cds.size()).buf();
	else
	{
	    str += cds.size() + 1;
	    nm = str.buf();
	}
    }

    ComponentData* newcd = new ComponentData( nm );
    newcd->datachar = dc;
    newcd->datatype = dtype;
    cds += newcd;
    bool isl = newcd->datachar.littleendian_;
    newcd->datachar.littleendian_ = __islittle__;
    tarcds += new TargetComponentData( *newcd, cds.size()-1 );
    newcd->datachar.littleendian_ = isl;
}


bool SeisTrcTranslator::initConn( Conn* c, bool forread )
{
    close(); errmsg = "";
    if ( !c )
	{ errmsg = "Translator: No connection established"; return false; }

    mDynamicCastGet(StreamConn*,strmconn,c)
    if ( strmconn )
    {
	const char* fnm = strmconn->streamData().fileName();
	if ( c->bad() && !File::isDirectory(fnm) )
	{
	    static BufferString emsg; emsg.setEmpty();
	    emsg.add( "Cannot open file: " ).add( fnm );
	    errmsg = emsg.buf(); return false;
	}
    }

    delete conn; conn = c;
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


void SeisTrcTranslator::setComponentNames( const BufferStringSet& bss )
{
    delete compnms_; compnms_ = new BufferStringSet( bss );
}


void SeisTrcTranslator::getComponentNames( BufferStringSet& bss ) const
{
    bss.erase();
    if ( cds.size() == 1 )	//TODO display comp name only if more than 1 
    {
	bss.add("");
	return;
    }

    for ( int idx=0; idx<cds.size(); idx++ )
	bss.add( cds[idx]->name() );
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


bool SeisTrcTranslator::getRanges( const MultiID& ky, CubeSampling& cs,
				   const char* lk )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    return ioobj ? getRanges( *ioobj, cs, lk ) : false;
}


bool SeisTrcTranslator::getRanges( const IOObj& ioobj, CubeSampling& cs,
				   const char* lk )
{
    PtrMan<Translator> transl = ioobj.getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,tr,transl.ptr());
    if ( !tr ) return false;
    PtrMan<Seis::SelData> sd = 0;
    if ( lk && *lk )
    {
	sd = Seis::SelData::get( Seis::Range );
	sd->lineKey() = lk;
	tr->setSelData( sd );
    }
    Conn* cnn = ioobj.getConn( Conn::Read );
    if ( !cnn || !tr->initRead(cnn,Seis::PreScan) )
	return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    cs.hrg.set( pinf.inlrg, pinf.crlrg );
    cs.zrg = pinf.zrg;
    return true;
}


void SeisTrcTranslator::usePar( const IOPar& iop )
{
    iop.getYN( sKeyIs2D(), is_2d );
    iop.getYN( sKeyIsPS(), is_prestack );
    iop.getYN( sKeyRegWrite(), enforce_regular_write );
    iop.getYN( sKeySIWrite(), enforce_survinfo_write );
}


bool SeisTrcTranslator::haveWarnings() const
{
    return !warnings_.isEmpty();
}


void SeisTrcTranslator::addWarn( int nr, const char* msg )
{
    if ( !msg || !*msg || warnnrs_.indexOf(nr) >= 0 ) return;
    warnnrs_ += nr;
    warnings_.add( msg );
}
