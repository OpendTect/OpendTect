/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seispsioprov.cc,v 1.14 2008-01-17 14:36:26 cvsbert Exp $";

#include "seispsioprov.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seispscubetr.h"
#include "seispsfact.h"
#include "seiscbvsps.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "segposinfo.h"
#include "filegen.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"


const char* SeisPSIOProvider::sKeyCubeID = "=Cube.ID";


SeisPSIOProviderFactory& SPSIOPF()
{
    static SeisPSIOProviderFactory* theinst = 0;
    if ( !theinst ) theinst = new SeisPSIOProviderFactory;
    return *theinst;
}


const SeisPSIOProvider* SeisPSIOProviderFactory::provider( const char* t ) const
{
    if ( provs_.isEmpty() )	return 0;
    else if ( !t )		return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( !strcmp(t,provs_[idx]->type()) )
	    return provs_[idx];

    return 0;
}


SeisPS3DReader* SeisPSIOProviderFactory::getReader( const IOObj& ioobj,
						    int inl ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS3DReader* reader =
	prov ? prov->makeReader( ioobj.fullUserExpr(true), inl ) : 0;

    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPS2DReader* SeisPSIOProviderFactory::get2DReader( const IOObj& ioobj ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS2DReader* reader =
	prov ? prov->make2DReader( ioobj.fullUserExpr(true) ) : 0;

    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPSWriter* SeisPSIOProviderFactory::getWriter( const IOObj& ioobj ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer =
	prov ? prov->makeWriter( ioobj.fullUserExpr(false) ) : 0;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}


SeisTrc* SeisPSReader::getTrace( const BinID& bid, int trcnr ) const
{
    SeisTrcBuf buf( true );
    if ( !getGather(bid,buf) || buf.size() <= trcnr )
	return false;
    return buf.remove( trcnr );
}


mDefSimpleTranslatorSelector(SeisPS,sKeySeisPSTranslatorGroup)
mDefSimpleTranslatorioContext(SeisPS,Seis)


bool CBVSSeisPSTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File_exists(fnm) )
	File_remove( fnm, File_isDirectory(fnm) );
    const char* res = ioobj->pars().find( SeisPSIOProvider::sKeyCubeID );
    if ( res )
	IOM().permRemove( MultiID(res) );
    return !File_exists(fnm);
}


SeisPSCubeSeisTrcTranslator::SeisPSCubeSeisTrcTranslator( const char* nm,
							  const char* unm )
	: SeisTrcTranslator(nm,unm)
    	, trc_(*new SeisTrc)
    	, psrdr_(0)
    	, inforead_(false)
    	, posdata_(*new PosInfo::CubeData)
{
}


SeisPSCubeSeisTrcTranslator::~SeisPSCubeSeisTrcTranslator()
{
    delete psrdr_;
    delete &trc_;
    delete &posdata_;
}


const char* SeisPSCubeSeisTrcTranslator::connType() const
{
    return XConn::sType;
}


bool SeisPSCubeSeisTrcTranslator::initRead_()
{
    mDynamicCastGet(StreamConn*,sconn,conn->conn())
    if ( !sconn )
	{ errmsg = "Wrong connection from Object Manager"; return false; }
    const char* typ = sconn->ioobj ? sconn->ioobj->translator() : "CBVS";
    if ( sconn->ioobj )
	psrdr_ = SPSIOPF().getReader( *sconn->ioobj );
    else
	psrdr_ = new SeisCBVSPS3DReader( sconn->fileName() );
    conn->close();
    errmsg = psrdr_->errMsg();
    if ( errmsg && *errmsg )
	return false;

    posdata_ = psrdr_->posData();
    posdata_.getInlRange( pinfo.inlrg );
    posdata_.getCrlRange( pinfo.crlrg );
    pinfo.inlrg.sort(); pinfo.crlrg.sort();
    curbinid_.inl = pinfo.inlrg.start;
    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;

    if ( !doRead(trc_) )
	return false;
    insd = trc_.info().sampling;
    innrsamples = trc_.size();
    for ( int idx=0; idx<trc_.nrComponents(); idx++ )
	addComp( trc_.data().getInterpreter(idx)->dataChar() );

    curbinid_.inl = pinfo.inlrg.start;
    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !posdata_.includes(bid.inl,bid.crl) ) return false;
    curbinid_ = bid; curbinid_.crl -= pinfo.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::toNext()
{
    for ( int crl=curbinid_.crl+pinfo.crlrg.step; crl<=pinfo.crlrg.stop;
	    crl+=pinfo.crlrg.step )
    {
	if ( posdata_.includes(curbinid_.inl,crl) )
	{
	    BinID bid( curbinid_.inl, crl );
	    if ( !seldata || seldata->isOK(BinID(curbinid_.inl,crl)) )
		{ curbinid_.crl = crl; return true; }
	}
    }

    curbinid_.inl += pinfo.inlrg.step;
    if ( curbinid_.inl > pinfo.inlrg.stop )
	return false;

    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;
    return toNext();
}


bool SeisPSCubeSeisTrcTranslator::doRead( SeisTrc& trc )
{
    if ( !toNext() ) return false;
    SeisTrc* newtrc = psrdr_->getTrace( curbinid_ );
    if ( !newtrc ) return false;
    if ( !seldata || seldata->isAll() )
	trc = *newtrc;
    else
    {
	trc.info() = newtrc->info();
	const Interval<float> zrg( seldata->zRange() );
	trc.info().sampling.start = zrg.start;
	const float sr = trc.info().sampling.step;
	const int nrsamps = (int)(zrg.width() / sr + 1.5);
	trc.reSize( nrsamps, false );
	for ( int idx=0; idx<nrsamps; idx++ )
	    trc.set( idx, newtrc->getValue( zrg.start + idx * sr, 0 ), 0 );
    }
    delete newtrc;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::readInfo( SeisTrcInfo& inf )
{
    if ( inforead_ ) return true;
    if ( !doRead(trc_) ) return false;
    inforead_ = true;
    inf = trc_.info();
    return true;
}


bool SeisPSCubeSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( inforead_ )
	{ inforead_ = false; trc = trc_; return true; }
    inforead_ = false;
    return doRead( trc );
}


bool SeisPSCubeSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( !toNext() ) return false;
    }
    return true;
}
