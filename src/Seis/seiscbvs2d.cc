/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seiscbvs2d.cc,v 1.59 2012/02/17 23:11:56 cvsnanne Exp $";

#include "seiscbvs2d.h"
#include "seiscbvs2dlinegetter.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "posinfo2d.h"
#include "cbvsio.h"
#include "executor.h"
#include "survinfo.h"
#include "keystrs.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "errh.h"
#include "ptrman.h"


int SeisCBVS2DLineIOProvider::factid
	= (S2DLIOPs() += new SeisCBVS2DLineIOProvider).size() - 1;


static const BufferString& gtFileName( const char* fnm )
{
    static BufferString ret;
    ret = fnm;
    if ( ret.isEmpty() ) return ret;

    FilePath fp( ret );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    ret = fp.fullPath();

    return ret;
}

static const BufferString& gtFileName( const IOPar& iop )
{
    return gtFileName( iop.find( sKey::FileName ).str() );
}

const char* SeisCBVS2DLineIOProvider::getFileName( const IOPar& iop )
{
    return gtFileName(iop).buf();
}


SeisCBVS2DLineIOProvider::SeisCBVS2DLineIOProvider()
    	: Seis2DLineIOProvider(CBVSSeisTrcTranslator::translKey() )
{
}


bool SeisCBVS2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    return Seis2DLineIOProvider::isUsable(iop) && iop.find( sKey::FileName );
}


bool SeisCBVS2DLineIOProvider::isEmpty( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return true;

    const BufferString& fnm = gtFileName( iop );
    return fnm.isEmpty() || File::isEmpty(fnm);
}


static CBVSSeisTrcTranslator* gtTransl( const char* fnm, bool infoonly,
					BufferString* msg=0 )
{
    return CBVSSeisTrcTranslator::make( fnm, infoonly, true, msg );
}


bool SeisCBVS2DLineIOProvider::getTxtInfo( const IOPar& iop,
		BufferString& uinf, BufferString& stdinf ) const
{
    if ( !isUsable(iop) ) return true;

    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( gtFileName(iop), true );
    if ( !tr ) return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    uinf = pinf.usrinfo;
    stdinf = pinf.stdinfo;
    return true;
}


bool SeisCBVS2DLineIOProvider::getRanges( const IOPar& iop,
		StepInterval<int>& trcrg, StepInterval<float>& zrg ) const
{
    if ( !isUsable(iop) ) return true;

    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( gtFileName(iop), true );
    if ( !tr ) return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    trcrg = pinf.crlrg; zrg = pinf.zrg;
    return true;
}


void SeisCBVS2DLineIOProvider::removeImpl( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return;
    const BufferString& fnm = gtFileName(iop);
    FilePath parfp( fnm );
    parfp.setExtension( "par" );
    File::remove( parfp.fullPath() );
    File::remove( fnm.buf() );
}


#undef mErrRet
#define mErrRet(s) { msg = s; return -1; }

SeisCBVS2DLineGetter::SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b,
					    int ntps, const Seis::SelData& sd )
    	: Executor("Load 2D line")
	, tbuf(b)
	, curnr(0)
	, totnr(0)
	, fname(fnm)
	, msg("Reading traces")
	, seldata(0)
	, trcstep(1)
	, linenr(CBVSIOMgr::getFileNr(fnm))
	, trcsperstep(ntps)
{
    tr = gtTransl( fname, false, &msg );
    if ( !tr ) return;

    if ( !sd.isAll() && sd.type() == Seis::Range )
    {
	seldata = sd.clone();
	tr->setSelData( seldata );
    }
}


SeisCBVS2DLineGetter::~SeisCBVS2DLineGetter()
{
    delete tr;
    delete seldata;
}


void SeisCBVS2DLineGetter::addTrc( SeisTrc* trc )
{
    const int tnr = trc->info().binid.crl;
    if ( !isEmpty(seldata) )
    {
	if ( seldata->type() == Seis::Range )
	{
	    const BinID bid( seldata->inlRange().start, tnr );
	    if ( !seldata->isOK(bid) )
		{ delete trc; return; }
	}
    }

    trc->info().nr = tnr;
    trc->info().binid = SI().transform( trc->info().coord );
    tbuf.add( trc );
}


int SeisCBVS2DLineGetter::nextStep()
{
    if ( !tr ) return -1;

    if ( curnr == 0 )
    {
	totnr = tr->packetInfo().crlrg.nrSteps() + 1;
	if ( !isEmpty(seldata) )
	{
	    const BinID tstepbid( 1, trcstep );
	    const int nrsel = seldata->expectedNrTraces( true, &tstepbid );
	    if ( nrsel < totnr ) totnr = nrsel;
	}
    }

    int lastnr = curnr + trcsperstep;
    for ( ; curnr<lastnr; curnr++ )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr->read(*trc) )
	{
	    delete trc;
	    const char* emsg = tr->errMsg();
	    if ( emsg && *emsg )
		mErrRet(emsg)
	    return 0;
	}
	addTrc( trc );

	for ( int idx=1; idx<trcstep; idx++ )
	{
	    if ( !tr->skip() )
		return 0;
	}
    }

    return 1;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }


bool SeisCBVS2DLineIOProvider::getGeometry( const IOPar& iop,
					    PosInfo::Line2DData& geom ) const
{
    geom.setEmpty();
    BufferString fnm = gtFileName(iop);
    if ( !isUsable(iop) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return false;
    }

    BufferString errmsg;
    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( fnm, false, &errmsg );
    if ( !tr )
    {
	ErrMsg( errmsg );
	return false;
    }

    const CBVSInfo& cbvsinf = tr->readMgr()->info();
    TypeSet<Coord> coords; TypeSet<BinID> binids;
    tr->readMgr()->getPositions( coords );
    tr->readMgr()->getPositions( binids );

    StepInterval<float> zrg( cbvsinf.sd.start, 0, cbvsinf.sd.step );
    zrg.stop = cbvsinf.sd.start + (cbvsinf.nrsamples-1) * cbvsinf.sd.step;
    geom.setZRange( zrg );
    const int sz = mMIN(coords.size(),binids.size());
    for ( int idx=0; idx<sz; idx++ )
    {
	PosInfo::Line2DPos p( binids[idx].crl );
	p.coord_ = coords[idx];
	geom.add( p );
    }

    return true;
}


Executor* SeisCBVS2DLineIOProvider::getFetcher( const IOPar& iop,
						SeisTrcBuf& tbuf, int ntps,
						const Seis::SelData* sd )
{
    const BufferString& fnm = gtFileName(iop);
    if ( !isUsable(iop) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return 0;
    }

    return new SeisCBVS2DLineGetter( fnm, tbuf, ntps,
	    		sd ? *sd : *Seis::SelData::get(Seis::Range));
}


#undef mErrRet
#define mErrRet(s) { pErrMsg( s ); return 0; }

Seis2DLinePutter* SeisCBVS2DLineIOProvider::getReplacer(
				const IOPar& iop )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    const char* res = iop.find( sKey::FileName );
    if ( !res )
	mErrRet("Knurft")

    return new SeisCBVS2DLinePutter( res, iop );
}


Seis2DLinePutter* SeisCBVS2DLineIOProvider::getAdder( IOPar& iop,
						      const IOPar* previop,
						      const char* lsetnm )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    BufferString fnm = iop.find( sKey::FileName ).str();
    if ( fnm.isEmpty() )
    {
	if ( previop )
	    fnm = CBVSIOMgr::baseFileName(previop->find(sKey::FileName)).buf();
	else
	{
	    if ( lsetnm && *lsetnm )
		fnm = lsetnm;
	    else
		fnm = iop.name();
	    fnm += ".cbvs";
	    cleanupString( fnm.buf(), false, true, true );
	}
	const char* prevfnm = previop ? previop->find(sKey::FileName) : 0;
	const int prevlnr = CBVSIOMgr::getFileNr( prevfnm );
	fnm = CBVSIOMgr::getFileName( fnm, previop ? prevlnr+1 : 0 );
	iop.set( sKey::FileName, fnm );
    }

    return new SeisCBVS2DLinePutter( fnm.buf(), iop );
}


//-------------------SeisCBVS2DLinePutter-----------------

SeisCBVS2DLinePutter::SeisCBVS2DLinePutter( const char* fnm, const IOPar& iop )
    	: nrwr(0)
	, fname(gtFileName(fnm))
	, tr(CBVSSeisTrcTranslator::getInstance())
	, preseldt(DataCharacteristics::Auto)
{
    tr->set2D( true );
    bid.inl = CBVSIOMgr::getFileNr( fnm );
    DataCharacteristics::parseEnumUserType(
	    iop.find(sKey::DataStorage), preseldt );
}


SeisCBVS2DLinePutter::~SeisCBVS2DLinePutter()
{
    delete tr;
}


bool SeisCBVS2DLinePutter::put( const SeisTrc& trc )
{
    SeisTrcInfo& info = const_cast<SeisTrcInfo&>( trc.info() );
    bid.crl = info.nr;
    const BinID oldbid = info.binid;
    info.binid = bid;

    if ( nrwr == 0 )
    {
	tr->setIs2D( true );
	bool res = tr->initWrite(new StreamConn(fname.buf(),Conn::Write),trc);
	if ( !res )
	{
	    info.binid = oldbid;
	    errmsg_ = "Cannot open 2D line file:\n";
	    errmsg_ += tr->errMsg();
	    return false;
	}
	if ( preseldt != DataCharacteristics::Auto )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
				= tr->componentInfo();
	    DataCharacteristics dc( preseldt );
	    for ( int idx=0; idx<ci.size(); idx++ )
	    {
		SeisTrcTranslator::TargetComponentData& cd = *ci[idx];
		cd.datachar = dc;
	    }
	}
    }

    tr->setIs2D( true );
    bool res = tr->write(trc);
    info.binid = oldbid;
    if ( res )
	nrwr++;
    else
    {
	errmsg_ = "Cannot write "; errmsg_ += nrwr + 1;
	errmsg_ += getRankPostFix( nrwr + 1 );
	errmsg_ += " trace to 2D line file:\n";
	errmsg_ += tr->errMsg();
	return false;
    }
    return true;
}

bool SeisCBVS2DLinePutter::close()
{
    if ( !tr ) return true;
    tr->setIs2D( true );
    bool ret = tr->close();
    if ( ret ) errmsg_ = tr->errMsg();
    return ret; 
}

