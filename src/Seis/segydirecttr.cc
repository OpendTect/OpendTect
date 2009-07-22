/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

static const char* rcsID = "$Id: segydirecttr.cc,v 1.10 2009-07-22 16:01:34 cvsbert Exp $";

#include "segydirecttr.h"
#include "segydirectdef.h"
#include "posinfo.h"
#include "filepath.h"
#include "segytr.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "ioobj.h"
#include "ptrman.h"
#include "dirlist.h"


class SEGYDirectPSIOProvider : public SeisPSIOProvider
{
public:
			SEGYDirectPSIOProvider()
			    	: SeisPSIOProvider("SEGYDirect")
    			{}
    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new SEGYDirect3DPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return new SEGYDirect2DPSReader(dirnm,lnm); }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    bool		getLineNames(const char*,BufferStringSet&) const;
    static int		factid;
};


bool SEGYDirectPSIOProvider::getLineNames( const char* dirnm,
					   BufferStringSet& nms ) const
{
    DirList dl( dirnm, DirList::FilesOnly, "*.sgydef" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	FilePath fp( dl.fullPath(idx) );
	fp.setExtension( 0 );
	nms.add( fp.fileName() );
    }

    return true;
}


// This adds the SEG-Y direct pre-stack seismics data storage to the factory
int SEGYDirectPSIOProvider::factid = SPSIOPF().add(new SEGYDirectPSIOProvider);


static SEGYSeisTrcTranslator* createTranslator( const SEGY::DirectDef& def,
						int filenr )
{
    const SEGY::FileDataSet* fds = def.dataSet();
    if ( !fds || filenr < 0 || fds->size() <= filenr )
	return 0;

    SEGY::FileSpec fs( (*fds)[filenr]->fname_ );
    PtrMan<IOObj> ioobj = fs.getIOObj();
    if ( !ioobj ) return 0;
    ioobj->pars() = fds->pars();

    SEGYSeisTrcTranslator* ret = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    ret->usePar( fds->pars() );
    if ( !ret->initRead(ioobj->getConn(Conn::Read)) )
	{ delete ret; return 0; }
    if ( !ret->commitSelections() )
	{ delete ret; return 0; }

    return ret;
}


SEGYDirect3DPSReader::SEGYDirect3DPSReader( const char* fnm )
    : posdata_(*new PosInfo::CubeData)
    , def_(*new SEGY::DirectDef(fnm))
    , tr_(0)
    , curfilenr_(-1)
{
    errmsg_ = def_.errMsg();
    def_.getPosData( posdata_ );
}


SEGYDirect3DPSReader::~SEGYDirect3DPSReader()
{
    delete &posdata_;
    delete &def_;
}


SeisTrc* SEGYDirect3DPSReader::getTrace( int filenr, int trcidx, int nr,
					 const BinID& bid ) const
{
    if ( !errmsg_.isEmpty() )
	return 0;

    if ( filenr != curfilenr_ )
    {
	delete tr_;
	tr_ = createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }
    if ( !tr_ || !tr_->goToTrace(trcidx+nr) )
	return 0;

    SeisTrc* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().binid != bid )
	{ delete trc; return 0; }
    if ( tr_->read(*trc) )
	return trc;

    delete trc; return 0;
}


SeisTrc* SEGYDirect3DPSReader::getTrace( const BinID& bid, int nr ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid), false );
    return ti.isValid() ? getTrace(ti.filenr_,ti.trcidx_,nr,bid) : 0;
}


bool SEGYDirect3DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    if ( !errmsg_.isEmpty() )
	return false;

    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid), false );
    if ( !ti.isValid() )
	return false;

    SeisTrc* trc = getTrace( ti.filenr_, ti.trcidx_, 0, bid );
    if ( !trc ) return false;

    tb.deepErase();
    for ( int itrc=1; trc; itrc++ )
    {
	tb.add( trc );
	trc = getTrace( ti.filenr_, ti.trcidx_, itrc, bid );
    }
    return true;
}



SEGYDirect2DPSReader::SEGYDirect2DPSReader( const char* dirnm, const char* lnm )
    : SeisPS2DReader(lnm)
    , posdata_(*new PosInfo::Line2DData)
    , def_(*new SEGY::DirectDef(SEGY::DirectDef::get2DFileName(dirnm,lnm)))
    , tr_(0)
    , curfilenr_(-1)
{
    def_.getPosData( posdata_ );
}


SEGYDirect2DPSReader::~SEGYDirect2DPSReader()
{
    delete &posdata_;
    delete &def_;
}


SeisTrc* SEGYDirect2DPSReader::getTrace( int filenr, int trcidx, int nr,
					 int trcnr ) const
{
    if ( filenr != curfilenr_ )
    {
	delete tr_;
	tr_ = createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }
    if ( !tr_ || !tr_->goToTrace(trcidx+nr) )
	return 0;

    SeisTrc* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().nr != trcnr )
	{ delete trc; return 0; }
    if ( tr_->read(*trc) )
	return trc;

    delete trc; return 0;
}


SeisTrc* SEGYDirect2DPSReader::getTrace( const BinID& bid, int nr ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl), false );
    return ti.isValid() ? getTrace(ti.filenr_,ti.trcidx_,nr,bid.crl) : 0;
}


bool SEGYDirect2DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl), false );
    if ( !ti.isValid() )
	return 0;

    SeisTrc* trc = getTrace( ti.filenr_, ti.trcidx_, 0, bid.crl );
    if ( !trc ) return false;

    tb.deepErase();
    for ( int itrc=1; trc; itrc++ )
    {
	tb.add( trc );
	trc = getTrace( ti.filenr_, ti.trcidx_, itrc, bid.crl );
    }
    return true;
}
