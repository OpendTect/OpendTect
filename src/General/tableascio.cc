/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: tableascio.cc,v 1.10 2007-01-08 17:10:48 cvsbert Exp $";

#include "tableascio.h"
#include "tabledef.h"
#include "tableconvimpl.h"
#include "unitofmeasure.h"
#include "separstr.h"
#include "iopar.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"
#include "filegen.h"
#include <iostream>

namespace Table
{

static const char* sKeyHdr = "Header";
static const char* sKeyBody = "Body";
static const char* sKeyHdrSize = "Header.Size";
static const char* sKeyHdrToken = "Header.Token";

static const char* filenamebase = "FileFormats";
static const char* sKeyGroup = "Group";

FileFormatRepository& FFR()
{
    static FileFormatRepository* ffrepo = 0;
    if ( !ffrepo )
	ffrepo = new FileFormatRepository;
    return *ffrepo;
}


FileFormatRepository::Entry::~Entry()
{
    delete iopar_;
}


FileFormatRepository::FileFormatRepository()
{
    Repos::FileProvider rfp( filenamebase );
    addFromFile( rfp.fileName(), rfp.source() );
    while ( rfp.next() )
	addFromFile( rfp.fileName(), rfp.source() );
}


void FileFormatRepository::addFromFile( const char* fnm,
					Repos::Source src )
{
    if ( !File_exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream stream( *sd.istrm, true );
    while ( stream.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar(stream);
	if ( newpar->name().isEmpty() )
	    { delete newpar; continue; }
	entries_ += new Entry( src, newpar );
    }
}


const char* FileFormatRepository::grpNm( int idx ) const
{
    return (*entries_[idx]->iopar_)[sKeyGroup];
}


int FileFormatRepository::gtIdx( const char* grp, const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->iopar_->name() != nm )
	    continue;

	const BufferString grpnm( grpNm(idx) );
	if ( grpnm == grp )
	    return idx;
    }

    return -1;
}


void FileFormatRepository::getGroups( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	bss.addIfNew( grpNm(idx) );
}


void FileFormatRepository::getFormats( const char* grp,
				       BufferStringSet& bss ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const BufferString grpnm( grpNm(idx) );
	if ( grpnm == grp )
	    bss.addIfNew( entries_[idx]->iopar_->name() );
    }
}


const IOPar* FileFormatRepository::get( const char* grp, const char* nm ) const
{
    const int idx = gtIdx( grp, nm );
    return idx < 0 ? 0 : entries_[idx]->iopar_;
}


void FileFormatRepository::set( const char* grp, const char* nm,
				IOPar* iop, Repos::Source src )
{
    const int idx = gtIdx( grp, nm );
    if ( idx >= 0 )
	{ Entry* entry = entries_[idx]; entries_.remove( idx ); delete entry; }
    if ( !iop ) return;

    if ( iop->find(sKey::Name) )
	iop->removeWithKey(sKey::Name);
    iop->setName( nm );
    iop->set( sKeyGroup, grp );
    entries_ += new Entry( src, iop );
}


bool FileFormatRepository::write( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    const BufferString fnm = rfp.fileName( src );

    bool havesrc = false;
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->src_ == src )
	    { havesrc = true; break; }
    }
    if ( !havesrc )
	return !File_exists(fnm) || File_remove( fnm, NO );

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( fnm );
	return false;
    }

    ascostream strm( *sd.ostrm );
    strm.putHeader( "File Formats" );
    for ( int idx=0; idx<entries_.size(); idx++ )
	entries_[idx]->iopar_->putTo( strm );

    sd.close();
    return true;
}


void TargetInfo::fillPar( IOPar& iopar ) const
{
    const char* nm = name();

    if ( form(selection_.form_).name() != nm )
	iopar.set( IOPar::compKey(nm,"Form"), form(selection_.form_).name() );

    if ( selection_.unit_ )
	iopar.set( IOPar::compKey(nm,"Unit"), selection_.unit_->name() );

    if ( selection_.elems_.size() < 1 || selection_.elems_[0].isEmpty() )
	return;

    FileMultiString fms( selection_.elems_[0].isInFile() ? "R" : "P" );
    for ( int idx=0; idx<selection_.elems_.size(); idx++ )
    {
	const TargetInfo::Selection::Elem& elem = selection_.elems_[idx];
	if ( !elem.isInFile() )
	    fms += elem.val_;
	else
	    { char buf[40]; elem.pos_.fill(buf); fms += buf; }
    }
    iopar.set( IOPar::compKey(nm,"Selection"), fms );
}


void TargetInfo::usePar( const IOPar& iopar )
{
    selection_.elems_.erase();
    const char* nm = name();
    if ( forms_.size() > 1 )
    {
	const char* res = iopar.find( IOPar::compKey(nm,"Form") );
	if ( res )
	    selection_.form_ = formNr( res );
    }
    if ( selection_.form_ < 0 ) selection_.form_ = 0;

    selection_.unit_ = UoMR().get( iopar.find(IOPar::compKey(nm,"Unit")) );
    const char* res = iopar.find( IOPar::compKey(nm,"Selection") );
    if ( !res || !*res )
	return;

    FileMultiString fms( res );
    const bool isinfile = *fms[0] == 'R';
    const int nrelems = fms.size() - 1;

    for ( int idx=0; idx<nrelems; idx++ )
    {
	res = fms[idx+1];
	TargetInfo::Selection::Elem elem;
	if ( !isinfile || !elem.pos_.use(res) )
	    elem.val_ = res;
	selection_.elems_ += elem;
    }
}


void FormatDesc::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyHdrSize, nrhdrlines_ );
    FileMultiString fms( token_ ); fms += tokencol_;
    iopar.set( sKeyHdrToken, fms );
    for ( int idx=0; idx<headerinfos_.size(); idx++ )
    {
	IOPar subpar; headerinfos_[idx]->fillPar( subpar );
	iopar.mergeComp( subpar, sKeyHdr );
    }
    for ( int idx=0; idx<bodyinfos_.size(); idx++ )
    {
	IOPar subpar; bodyinfos_[idx]->fillPar( subpar );
	iopar.mergeComp( subpar, sKeyBody );
    }
}


void FormatDesc::usePar( const IOPar& iopar )
{
    iopar.get( sKeyHdrSize, nrhdrlines_ );
    const char* res = iopar.find( sKeyHdrToken );
    if ( res )
    {
	FileMultiString fms( res );
	token_ = fms[0]; tokencol_ = atoi( fms[1] );
    }

    IOPar* subpar = iopar.subselect( sKeyHdr );
    if ( subpar && subpar->size() )
    {
	for ( int idx=0; idx<headerinfos_.size(); idx++ )
	    headerinfos_[idx]->usePar( *subpar );
    }
    delete subpar; subpar = iopar.subselect( sKeyBody );
    if ( subpar && subpar->size() )
    {
	for ( int idx=0; idx<bodyinfos_.size(); idx++ )
	    headerinfos_[idx]->usePar( *subpar );
    }
    delete subpar;
}


bool FormatDesc::isGood() const
{
    for ( int idx=0; idx<headerinfos_.size(); idx++ )
    {
	const TargetInfo& info = *headerinfos_[idx];
	if ( !info.isOptional() && !info.selection_.isFilled() )
	    return false;
    }
    for ( int idx=0; idx<bodyinfos_.size(); idx++ )
    {
	const TargetInfo& info = *bodyinfos_[idx];
	if ( !info.isOptional() && !info.selection_.isFilled() )
	    return false;
    }
    return true;
}


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

	    const RowCol& rc( tarinf.selection_.elems_[sid.specnr_].pos_ );
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
		aio_.addVal( tarinf.selection_.getVal(sid.specnr_),
		             tarinf.selection_.unit_ );
	    else
	    {
		bool found = false;
		for ( int idx=0; idx<formids_.size(); idx++ )
		{
		    if ( formids_[idx] == sid )
		    {
			aio_.addVal( formvals_.get(idx),
				     tarinf.selection_.unit_ );
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
    aio_.emptyVals();

    for ( int itar=0; itar<aio_.fd_.bodyinfos_.size(); itar++ )
    {
	const Table::TargetInfo& tarinf = *aio_.fd_.bodyinfos_[itar];
	SpecID sid( tarinf.selection_.form_, 0 );
	const Table::TargetInfo::Form& selform = tarinf.form( sid.formnr_ );
	for ( ; sid.specnr_<selform.specs_.size(); sid.specnr_++ )
	{
	    if ( !tarinf.selection_.havePos(sid.specnr_) )
		aio_.addVal( tarinf.selection_.getVal(sid.specnr_),
		             tarinf.selection_.unit_ );
	    else
	    {
		const RowCol& rc( tarinf.selection_.elems_[sid.specnr_].pos_ );
		if ( rc.c() < bss.size() )
		    aio_.addVal( bss.get(rc.c()), tarinf.selection_.unit_ );
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


void Table::AscIO::emptyVals() const
{
    Table::AscIO& aio = *const_cast<AscIO*>(this);
    aio.vals_.erase();
    aio.units_.erase();
}


void Table::AscIO::addVal( const char* s, const UnitOfMeasure* mu ) const
{
    Table::AscIO& aio = *const_cast<AscIO*>(this);
    aio.vals_.add( s );
    aio.units_ += mu;
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
	    for ( int ielem=0; ielem<selform.specs_.size(); ielem++ )
		addVal( tarinf.selection_.getVal(ielem),
		        tarinf.selection_.unit_ );
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


const char* Table::AscIO::text( int ifld ) const
{
    return ifld >= vals_.size() ? "" : ((const char*)vals_.get(ifld));
}


int Table::AscIO::getIntValue( int ifld ) const
{
    if ( ifld >= vals_.size() )
	return mUdf(int);
    const char* val = vals_.get( ifld );
    return *val ? atoi( val ) : mUdf(int);
}


float Table::AscIO::getfValue( int ifld ) const
{
    if ( ifld >= vals_.size() )
	return mUdf(int);
    const char* sval = vals_.get( ifld );
    if ( !*sval ) return mUdf(float);
    float val = atof( sval );
    if ( mIsUdf(val) ) return val;

    const UnitOfMeasure* unit = units_.size() > ifld ? units_[ifld] : 0;
    return unit ? unit->internalValue( val ) : val;
}


double Table::AscIO::getdValue( int ifld ) const
{
    if ( ifld >= vals_.size() )
	return mUdf(int);
    const char* sval = vals_.get( ifld );
    if ( !*sval ) return mUdf(double);
    double val = atof( sval );
    if ( mIsUdf(val) ) return val;

    const UnitOfMeasure* unit = units_.size() > ifld ? units_[ifld] : 0;
    return unit ? unit->internalValue( val ) : val;
}
