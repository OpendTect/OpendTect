/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seiscbvsps.cc,v 1.2 2004-12-30 15:04:40 bert Exp $";

#include "seiscbvsps.h"
#include "seiscbvs.h"
#include "seisbuf.h"
#include "filepath.h"
#include "filegen.h"
#include "survinfo.h"
#include "sortedlist.h"
#include "dirlist.h"


SeisCBVSPSIO::SeisCBVSPSIO( const char* dirnm )
    	: dirnm_(dirnm)
{
    BufferString& sm = const_cast<BufferString&>( selmask_ );
    sm = "*."; sm += CBVSSeisTrcTranslator::sKeyDefExtension;
}


SeisCBVSPSIO::~SeisCBVSPSIO()
{
}


SeisCBVSPSReader::SeisCBVSPSReader( const char* dirnm )
    	: SeisCBVSPSIO(dirnm)
    	, inls_(*new SortedList<int>(false))
{
    if ( !File_isDirectory(dirnm_) )
    {
	errmsg_ = "Directory '"; errmsg_ += dirnm_;
	errmsg_ += "' does not exist";
	return;
    }

    DirList dl( dirnm_, DirList::FilesOnly, selmask_.buf() );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm( dl.get(idx) );
	char* ptr = fnm.buf();
	while ( *ptr && !isdigit(*ptr) ) ptr++;
	while ( *ptr && isdigit(*ptr) ) ptr++;
	*ptr = '\0';
	if ( fnm == "" ) continue;
	inls_ += atoi( ptr );
    }

    if ( inls_.size() < 1 )
    {
	errmsg_ = "Directory '"; errmsg_ += dirnm_;
	errmsg_ += "' contains no usable pre-stack data files";
	return;
    }
}


SeisCBVSPSReader::~SeisCBVSPSReader()
{
    delete &inls_;
}


bool SeisCBVSPSReader::getGather( const BinID& bid, SeisTrcBuf& gath ) const
{
    int inlidx = inls_.indexOf( bid.inl );
    if ( inlidx < 0 )
	{ errmsg_ = "Inline not present"; return false; }

    FilePath fp( dirnm_ );
    BufferString fnm = inls_[inlidx]; fnm += ext();
    fp.add( fnm );

    errmsg_ = "";
    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::make( fp.fullPath(),
	    					false, false, &errmsg_ );
    if ( !tr )
	return false;

    if ( !tr->goTo( BinID(bid.crl,0) ) )
	{ errmsg_ = "Crossline not present"; return false; }

    const Coord coord = SI().transform( bid );
    while ( true )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr->read(*trc) )
	{
	    delete trc;
	    errmsg_ = tr->errMsg();
	    return errmsg_ == "";
	}
	else if ( trc->info().binid.inl != bid.crl )
	    { delete trc; return true; }

	trc->info().nr = trc->info().binid.crl;
	trc->info().binid = bid; trc->info().coord = coord;
	gath.add( trc );
    }

    // Not reached
    return true;
}


SeisCBVSPSWriter::SeisCBVSPSWriter( const char* dirnm )
    	: SeisCBVSPSIO(dirnm)
    	, reqdtype_(DataCharacteristics::Auto)
    	, curinl_(mUndefIntVal)
    	, tr_(0)
	, nringather_(0)
{
    if ( !File_isDirectory(dirnm_) )
    {
	if ( File_exists(dirnm_) )
	{
	    errmsg_ = "Existing file '";
	    errmsg_ += dirnm_; errmsg_ += "'. Remove or rename.";
	    return;
	}
	File_createDir(dirnm_,0);
    }
}


SeisCBVSPSWriter::~SeisCBVSPSWriter()
{
    close();
}


void SeisCBVSPSWriter::close()
{
    delete tr_; tr_ = 0;
}


bool SeisCBVSPSWriter::newInl( const SeisTrc& trc )
{
    if ( mIsUndefInt(curinl_) )
    {
	if ( reqdtype_ == DataCharacteristics::Auto )
	    dc_ = trc.data().getInterpreter()->dataChar();
	else
	    dc_ = DataCharacteristics( reqdtype_ );
    }

    const BinID& trcbid = trc.info().binid;
    curinl_ = trcbid.inl;

    BufferString fnm = curinl_; fnm += ext();
    FilePath fp( dirnm_ ); fp.add( fnm );
    fnm = fp.fullPath();

    if ( tr_ ) delete tr_;
    tr_ = CBVSSeisTrcTranslator::getInstance();
    if ( !tr_->initWrite(new StreamConn(fnm,Conn::Write),trc) )
    {
	errmsg_ = "Trying to start writing to '";
	errmsg_ += fnm; errmsg_ += "':\n";
	errmsg_ += tr_->errMsg();
	return false;
    }
    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci= tr_->componentInfo();
    for ( int idx=0; idx<ci.size(); idx++ )
	ci[idx]->datachar = dc_;

    nringather_ = 0;
    return true;
}


bool SeisCBVSPSWriter::put( const SeisTrc& trc )
{
    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    const BinID trcbid = ti.binid;
    if ( trcbid.inl != curinl_ )
    {
	if ( !newInl(trc) )
	    return false;
    }

    ti.binid = BinID( trcbid.crl, nringather_ );
    bool res = tr_->write( trc );
    ti.binid = trcbid;
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}
