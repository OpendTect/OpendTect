/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

static const char* rcsID = "$Id: segydirecttr.cc,v 1.15 2011-03-16 12:10:40 cvsbert Exp $";

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
    const FixedString filename = def.fileName( filenr ); 
    if ( !filename )
	return 0;

    SEGY::FileSpec fs( filename );
    PtrMan<IOObj> ioobj = fs.getIOObj();
    if ( !ioobj ) return 0;
    ioobj->pars() = *def.segyPars();

    SEGYSeisTrcTranslator* ret = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    ret->usePar( *def.segyPars() );
    if ( !ret->initRead(ioobj->getConn(Conn::Read)) )
	{ delete ret; return 0; }
    if ( !ret->commitSelections() )
	{ delete ret; return 0; }

    return ret;
}


SEGYDirect3DPSReader::SEGYDirect3DPSReader( const char* fnm )
    : def_(*new SEGY::DirectDef(fnm))
    , tr_(0)
    , curfilenr_(-1)
{
    errmsg_ = def_.errMsg();
}


SEGYDirect3DPSReader::~SEGYDirect3DPSReader()
{
    delete &def_;
    delete tr_;
}


const PosInfo::CubeData& SEGYDirect3DPSReader::posData() const 
{ return def_.cubeData(); }


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
    , def_(*new SEGY::DirectDef(SEGY::DirectDef::get2DFileName(dirnm,lnm)))
    , tr_(0)
    , curfilenr_(-1)
{
}


SEGYDirect2DPSReader::~SEGYDirect2DPSReader()
{
    delete &def_;
    delete tr_;
}


const PosInfo::Line2DData& SEGYDirect2DPSReader::posData() const 
{ return def_.lineData(); }


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


SEGYDirectSeisTrcTranslator::SEGYDirectSeisTrcTranslator( const char* s1,
							  const char* s2 )
    : SeisTrcTranslator(s1,s2)
    , def_(0)
    , tr_(0)
{
}


SEGYDirectSeisTrcTranslator::~SEGYDirectSeisTrcTranslator()
{
    cleanUp();
}


void SEGYDirectSeisTrcTranslator::cleanUp()
{
    delete def_; def_ = 0;
    delete tr_; tr_ = 0;
}


bool SEGYDirectSeisTrcTranslator::commitSelections_()
{
    return false;
}


bool SEGYDirectSeisTrcTranslator::initRead_()
{
    return false;
}


bool SEGYDirectSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    return false;
}


bool SEGYDirectSeisTrcTranslator::read( SeisTrc& trc )
{
    return false;
}


bool SEGYDirectSeisTrcTranslator::skip( int ntrcs )
{
    return false;
}


bool SEGYDirectSeisTrcTranslator::goTo( const BinID& bid )
{
    return false;
}


void SEGYDirectSeisTrcTranslator::usePar( const IOPar& iop )
{
}
