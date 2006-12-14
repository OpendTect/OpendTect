/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: tableascio.cc,v 1.6 2006-12-14 18:34:37 cvsbert Exp $";

#include "tableascio.h"
#include "tabledef.h"
#include "tableconvimpl.h"
#include <iostream>

namespace Table
{

struct SpecID
{
    		SpecID( int formnr, int snr )
		    : formnr_(formnr), specnr_(snr)		{}
    int		formnr_;
    int		specnr_;
    bool	operator ==( const SpecID& sid ) const
		{ return formnr_==sid.formnr_ && specnr_==sid.specnr_; }
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

const char* putRow( const BufferStringSet& bss )
{
    return hdr_ ? putHdrRow( bss ) : putBodyRow( bss );
}

const char* putHdrRow( const BufferStringSet& bss )
{
    if ( aio_.fd_.needToken() )
    {
	ready_ = bss.size() >= aio_.fd_.tokencol_
	      && bss.get( aio_.fd_.tokencol_ ) == aio_.fd_.token_;
	if ( ready_ )
	    return finishHdr();
    }

    for ( int itar=0; itar<aio_.fd_.headerinfos_.size(); itar++ )
    {
	const Table::TargetInfo& tarinf = *aio_.fd_.headerinfos_[itar];
	SpecID sid( tarinf.selection_.form_, 0 );
	const Table::TargetInfo::Form& selform = tarinf.form( sid.formnr_ );
	for ( ; sid.specnr_<selform.specs_.size(); sid.specnr_++ )
	{
	    if ( !tarinf.selection_.havePos(sid.specnr_) )
		continue;

	    const RowCol& rc( tarinf.selection_.pos_[sid.specnr_] );
	    if ( rc.r() == rownr_ )
	    {
		if ( rc.c() >= bss.size() )
		    return mkErrMsg( tarinf, sid, rc,
				     "Data not present in header" );
		else
		{
		    formvals_.add( bss.get(rc.c()) );
		    formids_ += sid;
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
    for ( int itar=0; itar<aio_.fd_.headerinfos_.size(); itar++ )
    {
	const Table::TargetInfo& tarinf = *aio_.fd_.headerinfos_[itar];
	SpecID sid( tarinf.selection_.form_, 0 );
	const Table::TargetInfo::Form& selform = tarinf.form( sid.formnr_ );
	for ( ; sid.specnr_<selform.specs_.size(); sid.specnr_++ )
	{
	    if ( !tarinf.selection_.havePos(sid.specnr_) )
		aio_.addVal( tarinf.selection_.getVal(sid.specnr_) );
	    else
	    {
		bool found = false;
		for ( int idx=0; idx<formids_.size(); idx++ )
		{
		    if ( formids_[idx] == sid )
		    {
			aio_.addVal( formvals_.get(idx) );
			found = true; break;
		    }
		}
		if ( !found && !tarinf.isOptional() )
		    return mkErrMsg( tarinf, sid, RowCol(-1,-1),
			    	     "Required field not found" );
	    }
	}
    }
    return 0;
}


const char* mkErrMsg( const TargetInfo& tarinf, SpecID sid,
		      const RowCol& rc, const char* msg )
{
    errmsg_ = msg; errmsg_ += ":\n";
    errmsg_ += tarinf.name(); errmsg_ += " [";
    errmsg_ += tarinf.form(sid.formnr_).name();
    if ( tarinf.nrForms() > 0 )
	{ errmsg_ += " (field "; sid.specnr_; errmsg_ += ")"; }
    errmsg_ += "]";
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


const char* putBodyRow( const BufferStringSet& bss )
{
    aio_.vals_.erase();
    for ( int itar=0; itar<aio_.fd_.bodyinfos_.size(); itar++ )
    {
	const Table::TargetInfo& tarinf = *aio_.fd_.bodyinfos_[itar];
	SpecID sid( tarinf.selection_.form_, 0 );
	const Table::TargetInfo::Form& selform = tarinf.form( sid.formnr_ );
	for ( ; sid.specnr_<selform.specs_.size(); sid.specnr_++ )
	{
	    if ( !tarinf.selection_.havePos(sid.specnr_) )
		aio_.addVal( tarinf.selection_.getVal(sid.specnr_) );
	    else
	    {
		const RowCol& rc( tarinf.selection_.pos_[sid.specnr_] );
		if ( rc.c() < bss.size() )
		    aio_.addVal( bss.get(rc.c()) );
		else
		    return mkErrMsg( tarinf, sid, RowCol(rc.c(),-1),
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
    TypeSet<SpecID>	formids_;
    BufferStringSet	formvals_;

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
	for ( int itar=0; itar<fd_.headerinfos_.size(); itar++ )
	{
	    const Table::TargetInfo& tarinf = *fd_.headerinfos_[itar];
	    const Table::TargetInfo::Form& selform
				= tarinf.form( tarinf.selection_.form_ );
	    for ( int ispec=0; ispec<selform.specs_.size(); ispec++ )
		addVal( tarinf.selection_.getVal(ispec) );
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

    int ret = cnvrtr_->nextStep();
    if ( ret < 0 )
	errmsg_ = cnvrtr_->message();
    return ret;
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
