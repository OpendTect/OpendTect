/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: tableascio.cc,v 1.3 2006-11-07 17:51:31 cvsbert Exp $";

#include "tableascio.h"
#include "tabledef.h"
#include "tableconvimpl.h"
#include <iostream>

namespace Table
{

struct SubID
{
    		SubID( int finr, int snr )
		    : finr_(finr), subnr_(snr)		{}
    int		finr_;
    int		subnr_;
    bool	operator ==( const SubID& sid ) const
		{ return finr_==sid.finr_ && subnr_==sid.subnr_; }
};


class AscIOImp_ExportHandler : public ExportHandler
{

public:

AscIOImp_ExportHandler( const AscIO& aio, bool hdr )
    : ExportHandler(std::cerr)
    , aio_(const_cast<AscIO&>(aio))
    , hdr_(hdr)
    , ready_(false)
    , rownr_(0)
{
}

const char* putRow( const BufferStringSet& bs )
{
    return hdr_ ? putHdrRow( bs ) : putBodyRow( bs );
}

const char* putHdrRow( const BufferStringSet& bs )
{
    if ( aio_.fd_.needToken() )
    {
	ready_ = bs.size() >= aio_.fd_.tokencol_
	      && bs.get( aio_.fd_.tokencol_ ) == aio_.fd_.token_;
	if ( ready_ )
	    return finishHdr();
    }

    for ( SubID sid(0,0); sid.finr_<aio_.fd_.headerinfos_.size(); sid.finr_++ )
    {
	const Table::FormatInfo& fi = *aio_.fd_.headerinfos_[sid.finr_];
	const int selelem = fi.selection_.elem_;
	for ( sid.subnr_=0; sid.subnr_<fi.nrSubElements(selelem); sid.subnr_++ )
	{
	    if ( !fi.selection_.havePos(sid.subnr_) )
		continue;

	    const RowCol& rc( fi.selection_.pos_[sid.subnr_] );
	    if ( rc.r() == rownr_ )
	    {
		if ( rc.c() >= bs.size() )
		    return mkErrMsg( fi, sid, selelem, rc,
			    	     "Data not present in header" );
		else
		{
		    elemvals_.add( bs.get(rc.c()) );
		    elemids_ += sid;
		}
	    }
	}
    }

    rownr_++;
    ready_ = ready_ || rownr_ >= aio_.fd_.nrHdrLines();
    return ready_ ? finishHdr() : 0;
}


const char* finishHdr()
{
    for ( SubID sid(0,0); sid.finr_<aio_.fd_.headerinfos_.size(); sid.finr_++ )
    {
	const Table::FormatInfo& fi = *aio_.fd_.headerinfos_[sid.finr_];
	const int selelem = fi.selection_.elem_;
	for ( sid.subnr_=0; sid.subnr_<fi.nrSubElements(selelem); sid.subnr_++ )
	{
	    if ( !fi.selection_.havePos(sid.subnr_) )
		aio_.addVal( fi.selection_.vals_.get(sid.subnr_) );
	    else
	    {
		bool found = false;
		for ( int idx=0; idx<elemids_.size(); idx++ )
		{
		    if ( elemids_[idx] == sid )
		    {
			aio_.addVal(elemvals_.get(idx));
			found = true; break;
		    }
		}
		if ( !found && !fi.isOptional() )
		    return mkErrMsg( fi, sid, selelem, RowCol(-1,-1),
			    	     "Required field not found" );
	    }
	}
    }
    return 0;
}


const char* mkErrMsg( const FormatInfo& fi, SubID sid, int selelem,
		      const RowCol& rc, const char* msg )
{
    errmsg_ = msg; errmsg_ += ":\n";
    errmsg_ += fi.elementName( selelem );
    errmsg_ += " (";
    errmsg_ += fi.subElementName( selelem, sid.subnr_ );
    errmsg_ += ")";
    if ( rc.c() >= 0 )
    {
	errmsg_ += "\nwas specified at ";
	if ( rc.r() < 0 )
	    errmsg_ += "column ";
	else
	    { errmsg_ += "row/col "; errmsg_ += rc.r()+1; errmsg_ += "/"; }
	errmsg_ += rc.c()+1;
    }
    return errmsg_;
}


const char* putBodyRow( const BufferStringSet& bs )
{
    for ( SubID sid(0,0); sid.finr_<aio_.fd_.bodyinfos_.size(); sid.finr_++ )
    {
	const Table::FormatInfo& fi = *aio_.fd_.bodyinfos_[sid.finr_];
	const int selelem = fi.selection_.elem_;
	for ( sid.subnr_=0; sid.subnr_<fi.nrSubElements(selelem); sid.subnr_++ )
	{
	    if ( !fi.selection_.havePos(sid.subnr_) )
		aio_.addVal( fi.selection_.vals_.get(sid.subnr_) );
	    else
	    {
		const RowCol& rc( fi.selection_.pos_[sid.subnr_] );
		if ( rc.c() < bs.size() )
		    aio_.addVal( bs.get(rc.c()) );
		else
		    return mkErrMsg( fi, sid, selelem, RowCol(rc.c(),-1),
			    	     "Column missing in file" );
	    }
	}
    }
    return 0;
}

    AscIO&		aio_;
    BufferString	errmsg_;
    const bool		hdr_;
    bool		ready_;
    int			rownr_;
    TypeSet<SubID>	elemids_;
    BufferStringSet	elemvals_;

};

}; // namespace Table

Table::AscIO::~AscIO()
{
    delete imphndlr_;
    delete exphndlr_;
    delete cnvrtr_;
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool Table::AscIO::getHdrVals( std::istream& strm ) const
{
    const int nrhdrlines = fd_.nrHdrLines();
    if ( nrhdrlines < 1 )
    {
	for ( SubID sid(0,0); sid.finr_<fd_.headerinfos_.size(); sid.finr_++ )
	{
	    const Table::FormatInfo& fi = *fd_.headerinfos_[sid.finr_];
	    const int selelem = fi.selection_.elem_;
	    for ( sid.subnr_=0; sid.subnr_<fi.nrSubElements(selelem);
		  sid.subnr_++ )
		addVal( fi.selection_.vals_.get(sid.subnr_) );
	}
    }
    else
    {
	Table::WSImportHandler hdrimphndlr( strm );
	Table::AscIOImp_ExportHandler hdrexphndlr( *this, true );
	Table::Converter hdrcnvrtr( hdrimphndlr, hdrexphndlr );
	for ( int idx=0; idx<nrhdrlines; idx++ )
	{
	    int res = hdrcnvrtr.nextStep();
	    if ( res < 0 )
		mErrRet( hdrcnvrtr.message() )
	    else if ( res == 0 || hdrexphndlr.ready_ )
		break;
	}
	if ( !hdrexphndlr.ready_ || !strm.good() )
	    mErrRet( "File header does not comply with format description" )
    }

    if ( !strm.good() || strm.eof() )
	mErrRet( "End of file reached before end of header" )

    return true;
}


int Table::AscIO::getNextBodyVals( std::istream& strm ) const
{
    if ( !cnvrtr_ )
    {
	AscIO& self = *const_cast<AscIO*>(this);
	self.imphndlr_ = new Table::WSImportHandler( strm );
	self.exphndlr_ = new Table::AscIOImp_ExportHandler( *this, false );
	self.cnvrtr_ = new Table::Converter( *imphndlr_, *exphndlr_ );
    }

    return cnvrtr_->nextStep();
}


bool Table::AscIO::putHdrVals( std::ostream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::putHdrVals not implemented";
    return false;
}


bool Table::AscIO::putNextBodyVals( std::ostream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::putNextBodyVals not implemented";
    return false;
}
