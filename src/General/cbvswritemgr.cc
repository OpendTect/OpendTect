/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS pack writer
-*/

static const char* rcsID = "$Id: cbvswritemgr.cc,v 1.2 2001-05-25 18:25:52 bert Exp $";

#include "cbvswritemgr.h"
#include "cbvswriter.h"
#include <fstream.h>


BufferString CBVSIOMgr::getFileName( const char* basefname, int curnr )
{
    if ( curnr < 1 ) return basefname;

    BufferString fname( basefname );

    char* ptr = strrchr( fname.buf(), '.' );
    BufferString ext;
    if ( ptr ) { ext = ptr; *ptr = '\0'; }
    fname += curnr < 10 ? "^0" : "^";
    fname += curnr; fname += ext;

    return fname;
}


CBVSWriteMgr::CBVSWriteMgr( const char* fnm, const CBVSInfo& i,
			    const CBVSInfo::ExplicitData* e )
	: CBVSIOMgr(fnm)
	, writer_(0)
	, info_(i)
{
    ostream* strm = mkStrm();
    if ( !strm ) return;

    writer_ = new CBVSWriter( strm, info_, e );
    writer_->setByteThreshold( 1900000000 );
}


ostream* CBVSWriteMgr::mkStrm()
{
    fname_ = getFileName( curnr_ );
    ostream* strm = new ofstream( (const char*)fname_ );
    if ( !strm || !*strm )
    {
	errmsg_ = "Cannot open '"; errmsg_ += fname_; errmsg_ += "' for write";
	delete strm; strm = 0;
    }
    return strm;
}


CBVSWriteMgr::~CBVSWriteMgr()
{
    close();
    delete writer_;
}


void CBVSWriteMgr::close()
{
    if ( writer_ ) writer_->close();
}


const char* CBVSWriteMgr::errMsg_() const
{
    return writer_ ? writer_->errMsg() : 0;
}


int CBVSWriteMgr::nrComponents() const
{
    return writer_ ? writer_->nrComponents() : 0;
}


const BinID& CBVSWriteMgr::binID() const
{
    static BinID binid00(0,0);
    return writer_ ? writer_->binID() : binid00;
}


unsigned long CBVSWriteMgr::bytesPerFile() const
{
    return writer_ ? writer_->byteThreshold() : 0;
}


void CBVSWriteMgr::setBytesPerFile( unsigned long b )
{
    if ( writer_ ) writer_->setByteThreshold( b );
}


bool CBVSWriteMgr::put( void** data )
{
    if ( !writer_ ) return false;

    int ret = writer_->put( data );
    if ( ret == 1 )
    {
	curnr_++;
	ostream* strm = mkStrm();
	if ( !strm ) return false;

	if ( info_.geom.fullyrectandreg )
	    info_.geom.start.inl = writer_->survGeom().stop.inl
				 + info_.geom.step.inl;
	CBVSWriter* oldwriter = writer_;
	writer_ = new CBVSWriter( strm, *oldwriter, info_ );
	delete oldwriter;

	ret = writer_->put( data );
    }

    return ret == -1 ? false : true;
}
