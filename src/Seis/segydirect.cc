/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.7 2008-11-26 14:43:46 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyfiledata.h"
#include "seisposindexer.h"
#include "idxable.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"

const char* sKeyDirectDef = "DirectSEG-Y";
static const char* sKeyFileType = "SEG-Y Direct Definition";
static const char* sKeyNrFiles = "Number of files";

namespace SEGY
{

class PosKeyList : public Seis::PosKeyList
{
public:

PosKeyList()
{
    setFDS( 0 );
}

void setFDS( const FileDataSet* fds )
{
    fds_ = fds; totsz_ = 0; cumszs_.erase();
    if ( !fds_ ) return;

    for ( int idx=0; idx<fds_->size(); idx++ )
    {
	const int sz = (*fds_)[idx]->size();
	cumszs_ += totsz_;
	totsz_ += sz;
    }
}

od_int64 size() const
{
    return totsz_;
}

Seis::PosKey key( od_int64 nr ) const
{
    if ( !fds_ ) return Seis::PosKey::undef();

    int idx;
    IdxAble::findPos( cumszs_.arr(), cumszs_.size(), nr, -1, idx );
    const int relidx = nr - cumszs_[idx];
    const SEGY::TraceInfo& ti = *(*(*fds_)[idx])[relidx];

    return ti.isUsable() ? ti.pos_ : Seis::PosKey::undef();
}

    TypeSet<od_int64>	cumszs_;
    od_int64		totsz_;
    const FileDataSet*	fds_;

};

}


#define mDefMembInitList \
    : fds_(0) \
    , myfds_(0) \
    , curfidx_(-1) \
    , keylist_(*new SEGY::PosKeyList) \
    , indexer_(*new Seis::PosIndexer(keylist_))

SEGY::DirectDef::DirectDef()
    mDefMembInitList
{
}


SEGY::DirectDef::DirectDef( const char* fnm )
    mDefMembInitList
{
    readFromFile( fnm );
}


SEGY::DirectDef::~DirectDef()
{
    delete myfds_;
    delete &keylist_;
    delete &indexer_;
}


void SEGY::DirectDef::setData( FileDataSet* fds )
{
    if ( fds != myfds_ )
	delete myfds_;
    fds_ = myfds_ = fds;
    keylist_.setFDS( fds_ ); indexer_.reIndex();
}


void SEGY::DirectDef::setData( const FileDataSet& fds, bool nc )
{
    if ( &fds != myfds_ )
	delete myfds_;
    if ( nc )
	{ myfds_ = 0; fds_ = &fds; }
    else
	fds_ = myfds_ = new FileDataSet( fds );
    keylist_.setFDS( fds_ ); indexer_.reIndex();
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"'"))

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mErrRet(BufferString("Input file '",fnm,"' has wrong file type"))

    int nrfiles = 0;
    IOPar iop; iop.getFrom( astrm );
    iop.get( sKeyNrFiles, nrfiles );
    Seis::GeomType gt;
    if ( !Seis::getFromPar(iop,gt) )
	mErrRet(BufferString("Missing crucial info in '",fnm,"'"))
    
    FileDataSet* fds = new FileDataSet( iop );
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FileData* fd = new FileData(0,gt);
	if ( !fd->getFrom(astrm) )
	{
	    BufferString emsg( "Error reading " );
	    if ( nrfiles > 1 )
		{ emsg += idx+1; emsg += getRankPostFix(idx+1); emsg += " "; }
	    emsg += "file data from '"; emsg += fnm; emsg += "'";
	    delete fds;
	    mErrRet(emsg)
	}
    }

    setData( fds );
    return true;
}


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"' for write"))

    const int nrfiles = fds_ ? fds_->size() : 0;
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( sKeyFileType );
    if ( fds_ )
    {
	IOPar iop( fds_->pars() );
	iop.set( sKeyNrFiles, nrfiles );
	iop.putTo( astrm );
    }

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	const SEGY::FileData& fd = *(*fds_)[ifile];
	if ( !fd.putTo(astrm) )
	{
	    BufferString emsg( "Error writing data for '" );
	    emsg += fd.fname_; emsg += "'"; emsg += "\nto '";
	    emsg += fnm; emsg += "'";
	    mErrRet(emsg)
	}
    }

    return true;
}
