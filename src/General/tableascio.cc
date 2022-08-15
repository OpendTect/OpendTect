/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/


#include "tableascio.h"

#include "ascstream.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "tabledef.h"
#include "tableconvimpl.h"
#include "unitofmeasure.h"

namespace Table
{

static const char* sKeyHdr = "Header";
static const char* sKeyBody = "Body";
static const char* sKeyBodyEndToken = "Body.End.Token";
static const char* sKeyHdrSize = "Header.Size";
static const char* sKeyHdrToken = "Header.Token";

static const char* filenamebase = "FileFormats";
static const char* sKeyGroup = "Group";

const char* TargetInfo::sKeyXY()	{ return "X/Y"; }
const char* TargetInfo::sKeyInlCrl()	{ return "Inl/Crl"; }
const char* TargetInfo::sKeyLatLong()	{ return "Lat/Long"; }

FileFormatRepository& FFR()
{
    mDefineStaticLocalObject( FileFormatRepository, ffrepo, );
    return ffrepo;
}


FileFormatRepository::Entry::~Entry()
{
    delete iopar_;
}


FileFormatRepository::FileFormatRepository()
{
    Repos::FileProvider rfp( filenamebase );
    while ( rfp.next() )
	addFromFile( rfp.fileName(), rfp.source() );
}


void FileFormatRepository::addFromFile( const char* fnm,
					Repos::Source src )
{
    od_istream strm( fnm );
    if ( !strm.isOK() ) return;

    ascistream stream( strm, true );
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
	{ delete entries_.removeSingle( idx );  }
    if ( !iop ) return;

    if ( iop->find(sKey::Name()) )
	iop->removeWithKey(sKey::Name());
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
	return !File::exists(fnm) || File::remove( fnm );

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	strm.addErrMsgTo( msg ); ErrMsg( fnm );
	return false;
    }

    ascostream astrm( strm );
    astrm.putHeader( "File Formats" );
    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( entries_[idx]->src_ == src )
	    entries_[idx]->iopar_->putTo( astrm );

    return true;
}


static const char* sKeyCoordinateSystem = "Coordinate System";

void TargetInfo::fillPar( IOPar& iopar ) const
{
    const char* nm = name();

    if ( form(selection_.form_).name() != nm )
	iopar.set( IOPar::compKey(nm,"Form"), form(selection_.form_).name() );

    if ( selection_.unit_ )
	iopar.set( IOPar::compKey(nm,"Unit"), selection_.unit_->name() );

    if ( selection_.coordsys_ )
    {
	IOPar crspar;
	selection_.coordsys_->fillPar( crspar );
	iopar.mergeComp( crspar, IOPar::compKey(nm,sKeyCoordinateSystem) );
    }

    if ( selection_.elems_.size() < 1 || selection_.elems_[0].isEmpty() )
	return;

    FileMultiString fms;
    for ( int idx=0; idx<selection_.elems_.size(); idx++ )
    {
	const TargetInfo::Selection::Elem& elem = selection_.elems_[idx];
	const int typ = !elem.isInFile() ? 0 : (elem.isKeyworded() ? 1 : 2);
	const char* typstr = typ == 2 ? "P" : (typ == 1 ? "K" : "R");

	if ( idx == 0 )
	    fms = typstr;
	else
	    fms += typstr;

	if ( typ == 0 )
	    fms += elem.val_;
	else if ( typ == 2 )
	    fms += elem.pos_.toString();
	else
	    { fms += elem.keyword_; fms += elem.pos_.col(); }
    }
    iopar.set( IOPar::compKey("Selection",nm), fms );
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
    PtrMan<IOPar> crspar =
		iopar.subselect( IOPar::compKey(nm,sKeyCoordinateSystem) );
    if ( crspar )
	selection_.coordsys_ = Coords::CoordSystem::createSystem( *crspar );

    const char* res = iopar.find( IOPar::compKey("Selection",nm) );
    if ( !res || !*res )
    {
	const int nrspecs = form( selection_.form_ ).specs_.size();
	for ( int idx=0; idx<nrspecs; idx++ )
	{
	    TargetInfo::Selection::Elem elem;
	    selection_.elems_ += elem;
	}

	return;
    }

    FileMultiString fms( res );
    int curfmsidx = 0;
    const int nrelems = fms.size() - 1;

    for ( int idx=0; idx<nrelems; idx++ )
    {
	StringView typestring = fms[curfmsidx];
	const char typc = typestring.isEmpty() ? 0 : *typestring.str();
	const int typ = typc == 'P' ? 2 : (typc == 'K' ? 1 : 0);
	curfmsidx++;
	res = fms[curfmsidx];
	TargetInfo::Selection::Elem elem;
	if ( typ == 0 )
	    elem.val_ = res;
	else if ( typ == 2 )
	    elem.pos_.fromString( res );
	else
	{
	    curfmsidx++;
	    elem.keyword_ = res;
	    elem.pos_.col() = fms.getIValue( curfmsidx );
	}

	selection_.elems_ += elem;
	curfmsidx++;
    }
}


Table::TargetInfo*
	TargetInfo::mkHorPosition( bool isreq, bool wic, bool wll, bool wcrs,
				   ConstRefMan<Coords::CoordSystem> crs )
{
    const ReqSpec reqspec( isreq ? Table::Required : Table::Optional );
    TargetInfo* ti = new TargetInfo( "", DoubleInpSpec(), reqspec );
    ti->form(0).setName( sKeyXY() );
    ti->form(0).add( DoubleInpSpec() );
    if ( wcrs )
	ti->selection_.coordsys_ = crs ? crs : SI().getCoordSystem();

    if ( wic )
    {
	auto* form = new TargetInfo::Form( sKeyInlCrl(), IntInpSpec() );
	form->add( IntInpSpec() );
	ti->add( form );
    }

    if ( wll )
    {
	auto* form = new TargetInfo::Form( sKeyLatLong(), StringInpSpec() );
	form->add( StringInpSpec() );
	ti->add( form );
	if ( wcrs && ti->selection_.coordsys_->isProjection() )
	    ti->selection_.llsys_ =
		ti->selection_.coordsys_->getGeodeticSystem();
    }

    if ( ti->nrForms()==1 )
	ti->setName( "Position X/Y" );

    return ti;
}


Table::TargetInfo* TargetInfo::mk2DHorPosition( bool isreq,
					ConstRefMan<Coords::CoordSystem> crs )
{
    const ReqSpec reqspec( isreq ? Required : Optional );
    TargetInfo* ti = new TargetInfo( "", DoubleInpSpec(), reqspec );
    ti->form(0).setName( sKeyXY() );
    ti->form(0).add( DoubleInpSpec() );
    ti->selection_.coordsys_ = crs ? crs : SI().getCoordSystem();

    auto* form1 = new TargetInfo::Form( "Trace Nr", IntInpSpec() );
    ti->add( form1 );

    auto* form2 = new TargetInfo::Form( "SP Nr", FloatInpSpec() );
    ti->add( form2 );

    if ( ti->selection_.coordsys_ && ti->selection_.coordsys_->isProjection() )
    {
	auto* form3 = new TargetInfo::Form( sKeyLatLong(), StringInpSpec() );
	form3->add( StringInpSpec() );
	ti->add( form3 );
	ti->selection_.llsys_ = ti->selection_.coordsys_->getGeodeticSystem();
    }

    return ti;
}


bool TargetInfo::needsConversion() const
{
    ConstRefMan<Coords::CoordSystem> inpcrs = selection_.coordsys_;
    ConstRefMan<Coords::CoordSystem> outcrs = SI().getCoordSystem();
    return inpcrs && outcrs && !(*inpcrs == *outcrs);
}


Coord TargetInfo::convert( const Coord& crd ) const
{
    if ( !needsConversion() )
	return crd;

    ConstRefMan<Coords::CoordSystem> inpcrs = selection_.coordsys_;
    ConstRefMan<Coords::CoordSystem> outcrs = SI().getCoordSystem();

    return outcrs->convertFrom( crd, *inpcrs );
}


Table::TargetInfo* TargetInfo::mkZPos( bool isreq, bool wu, int zopt )
{
    const Table::ReqSpec reqspec( isreq ? Table::Required : Table::Optional );
    Table::TargetInfo* ti = nullptr;
    if ( !wu )
	ti = new TargetInfo( "Z", FloatInpSpec(), reqspec );
    else
    {
	if ( zopt == 0 )
	{
	    ti = new TargetInfo( "Z", FloatInpSpec(), reqspec,
				 Mnemonic::surveyZType() );
	    ti->selection_.unit_ = UnitOfMeasure::surveyDefZUnit();
	}
	else if ( zopt < 0 )
	{
	    ti = new TargetInfo( sKey::Time(), FloatInpSpec(), reqspec,
				 Mnemonic::Time );
	    ti->selection_.unit_ = UoMR().get( "Milliseconds" );
	}
	else
	{
	    ti = new TargetInfo( sKey::Depth(), FloatInpSpec(), reqspec,
				 Mnemonic::Dist );
	    ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
	}
    }

    return ti;
}


void FormatDesc::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyHdrSize, nrhdrlines_ );
    FileMultiString fms; fms += eohtokencol_; fms += eohtoken_;
    iopar.set( sKeyHdrToken, fms );
    iopar.update( sKeyBodyEndToken, eobtoken_ );
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
    if ( res && *res )
    {
	FileMultiString fms( res );
	eohtokencol_ = fms.getIValue( 0 ); eohtoken_ = fms[1];
    }

    res = iopar.find( sKeyBodyEndToken );
    if ( res && *res )
	eobtoken_ = res;

    IOPar* subpar = iopar.subselect( sKeyHdr );
    if ( subpar && subpar->size() )
    {
	for ( int idx=0; idx<headerinfos_.size(); idx++ )
	    headerinfos_[idx]->usePar( *subpar );
    }
    delete subpar;

    subpar = iopar.subselect( sKeyBody );
    if ( subpar && subpar->size() )
    {
	for ( int idx=0; idx<bodyinfos_.size(); idx++ )
	    bodyinfos_[idx]->usePar( *subpar );
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


bool FormatDesc::bodyUsesCol( int icol ) const
{
    for ( int idx=0; idx<bodyinfos_.size(); idx++ )
    {
	const TargetInfo& info = *bodyinfos_[idx];
	const TargetInfo::Selection& sel = info.selection_;
	for ( int ielm=0; ielm<sel.elems_.size(); ielm++ )
	{
	    if ( sel.elems_[ielm].pos_.col() == icol )
		return true;
	}
    }
    return false;
}


class AscIOImp_ExportHandler : public ExportHandler
{

public:

struct BodyInfo
{
		BodyInfo( const TargetInfo& ti, int specnr )
		    : tarinf_(ti)
		    , sel_(ti.selection_)
		    , form_(ti.form(ti.selection_.form_))
		    , col_(-1)
		    , specnr_(specnr)
		{
		    if ( sel_.havePos(specnr_) )
			col_ = sel_.elems_[specnr_].pos_.col();
		}
    virtual	~BodyInfo()			{}
    bool	operator ==( const BodyInfo& bi )
		{ return col_ == bi.col_; }

    int		col_;
    int		specnr_;
    const TargetInfo& tarinf_;
    const TargetInfo::Selection& sel_;	// convenience
    const TargetInfo::Form& form_;	// convenience

};

struct HdrInfo : public BodyInfo
{
		HdrInfo( const TargetInfo& ti, int specnr )
		    : BodyInfo(ti,specnr)
		    , found_(false)
		    , row_(-1)
		{
		    const TargetInfo::Selection::Elem& elem
						= sel_.elems_[specnr_];
		    if ( sel_.havePos(specnr_) )
		    {
			if ( elem.isKeyworded() )
			    keyw_ = elem.keyword_;
			else
			    row_ = elem.pos_.row();
		    }
		    else
		    {
			found_ = true;
			val_ = elem.val_;
		    }
		}
    bool	operator ==( const HdrInfo& hi )
		{ return row_ == hi.row_ && col_ == hi.col_; }

    int		row_;
    BufferString keyw_;
    bool	found_;
    BufferString val_;

};

AscIOImp_ExportHandler( const AscIO& aio, bool hdr )
    : ExportHandler(od_cout())
    , aio_(const_cast<AscIO&>(aio))
    , ishdr_(hdr)
    , hdrready_(false)
    , bodyready_(false)
    , rownr_(0)
{
    if ( ishdr_ )
    {
	for ( int itar=0; itar<aio_.fd_.headerinfos_.size(); itar++ )
	{
	    const TargetInfo& tarinf = *aio_.fd_.headerinfos_[itar];
	    TargetInfo::Selection& sel = tarinf.selection_;
	    const int nrspecs = tarinf.form( sel.form_ ).specs_.size();
	    for ( int ispec=0; ispec<nrspecs; ispec++ )
		hdrinfos_ += new HdrInfo( tarinf, ispec );
	}
    }
    else
    {
	for ( int itar=0; itar<aio_.fd_.bodyinfos_.size(); itar++ )
	{
	    const TargetInfo& tarinf = *aio_.fd_.bodyinfos_[itar];
	    TargetInfo::Selection& sel = tarinf.selection_;
	    const int nrspecs = tarinf.form( sel.form_ ).specs_.size();
	    for ( int ispec=0; ispec<nrspecs; ispec++ )
		bodyinfos_ += new BodyInfo( tarinf, ispec );
	}
    }
}

bool putRow( const BufferStringSet& bss, uiString& msg ) override
{
    return ishdr_ ? putHdrRow( bss, msg ) : putBodyRow( bss, msg );
}

bool putHdrRow( const BufferStringSet& bss, uiString& msg )
{
    if ( aio_.fd_.needEOHToken() )
    {
	if ( aio_.fd_.eohtokencol_ >= 0 )
	    hdrready_ = bss.size() >= aio_.fd_.eohtokencol_
		  && bss.get(aio_.fd_.eohtokencol_) == aio_.fd_.eohtoken_;
	else
	{
	    for ( int idx=0; idx<bss.size(); idx++ )
	    {
		if ( bss.get(idx) == aio_.fd_.eohtoken_ )
		    { hdrready_ = true; break; }
	    }
	}
	if ( hdrready_ )
	    return finishHdr( msg );
    }

    if ( bss.size() > 0 )
    {
	for ( int ihdr=0; ihdr<hdrinfos_.size(); ihdr++ )
	{
	    HdrInfo& hdrinf = *hdrinfos_[ihdr];
	    if ( hdrinf.found_ )
		continue;

	    const bool hasrow = hdrinf.row_ >= 0;
	    if ( hasrow && hdrinf.row_ != rownr_ )
		continue;

	    if ( hasrow || hdrinf.keyw_ == bss.get(0) )
	    {
		if ( hdrinf.col_ >= bss.size() )
		{
		    msg = mkErrMsg( hdrinf, "Data not present in header" );
		    return false;
		}

		hdrinf.found_ = true;
		hdrinf.val_ = bss.get( hdrinf.col_ );
	    }
	}
    }

    rownr_++;
    hdrready_ = hdrready_ || rownr_ >= aio_.fd_.nrHdrLines();
    return hdrready_ ? finishHdr(msg) : true;
}


bool finishHdr( uiString& msg )
{
    for ( int ihdr=0; ihdr<hdrinfos_.size(); ihdr++ )
    {
	const HdrInfo& hdrinf = *hdrinfos_[ihdr];
	if ( !hdrinf.found_ && !hdrinf.tarinf_.isOptional() )
	{
	    msg = mkErrMsg( hdrinf, "Required field not found" );
	    return false;
	}
	else
	    aio_.addVal( hdrinf.val_, hdrinf.sel_.unit_ );
    }

    return true;
}


uiString mkErrMsg( const HdrInfo& hdrinf, const char* msg ) const
{
    BufferString errmsg = msg; errmsg += ":\n";
    errmsg += hdrinf.tarinf_.name();

    const bool diffnms = hdrinf.tarinf_.name() != hdrinf.form_.name();
    if ( diffnms )
    {
	errmsg += " [";
	errmsg += hdrinf.form_.name();
    }
    if ( hdrinf.form_.specs_.size() > 1 )
	{ errmsg += " (field "; errmsg += hdrinf.specnr_; errmsg += ")"; }
    if ( diffnms )
	errmsg += "]";

    if ( hdrinf.col_ >= 0 )
    {
	errmsg += "\n\nIt was specified at ";
	if ( hdrinf.row_ >= 0 )
	    { errmsg += "row/col "; errmsg += hdrinf.row_+1; errmsg += "/"; }
	errmsg += "column ";
	errmsg += hdrinf.col_+1;
	if ( !hdrinf.keyw_.isEmpty() )
	{
	    errmsg += "; keyword '";
	    errmsg += hdrinf.keyw_; errmsg += "'";
	}
    }

    return toUiString(errmsg.str());
}


bool putBodyRow( const BufferStringSet& bss, uiString& msg )
{
    aio_.emptyVals();
    if ( bodyready_ ) return true;

    if ( aio_.fd_.haveEOBToken() && !bss.isEmpty() )
    {
	if ( bss.get(0) == aio_.fd_.eobtoken_
	  || bss.get(bss.size()-1) == aio_.fd_.eobtoken_ )
	{
	    bodyready_ = true;
	    return true;
	}
    }

    for ( int iinf=0; iinf<bodyinfos_.size(); iinf++ )
    {
	const BodyInfo& bodyinf = *bodyinfos_[iinf];
	if ( bss.validIdx(bodyinf.col_) )
	    aio_.addVal( bss.get(bodyinf.col_), bodyinf.sel_.unit_ );
	else
	    aio_.addVal( "", nullptr );
    }

    if ( aio_.needfullline_ )
	aio_.fullline_ = bss;
    rownr_++;
    return true;
}

    const bool		ishdr_;
    AscIO&		aio_;
    BufferString	errmsg_;
    bool		hdrready_;
    bool		bodyready_;
    int			rownr_;
    ObjectSet<HdrInfo>	hdrinfos_;
    ObjectSet<BodyInfo>	bodyinfos_;

};

} // namespace Table

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

bool Table::AscIO::getHdrVals( od_istream& strm ) const
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
		mErrRet( hdrcnvrtr.uiMessage() )
	    else if ( res == 0 || hdrexphndlr.hdrready_ )
		break;
	}
	if ( !hdrexphndlr.hdrready_ || !strm.isOK() )
	    mErrRet( tr("File header does not comply with format description" ))
    }

    if ( !strm.isOK() )
	mErrRet( tr("End of file reached before end of header" ))

    hdrread_ = true;
    return true;
}


int Table::AscIO::getNextBodyVals( od_istream& strm ) const
{
    if ( !hdrread_ )
    {
	if ( !getHdrVals(strm) )
	{
	    uiString prevmsg = errmsg_;
	    errmsg_ = uiStrings::phrCannotRead(tr("file header" ));
	    if ( !prevmsg.isEmpty() )
		errmsg_.append( prevmsg, true );

	    return -1;
	}
    }

    if ( !cnvrtr_ )
    {
	AscIO& self = *const_cast<AscIO*>(this);
	self.imphndlr_ = new Table::WSImportHandler( strm );
	self.exphndlr_ = new Table::AscIOImp_ExportHandler( *this, false );
	self.cnvrtr_ = new Table::Converter( *imphndlr_, *exphndlr_ );
    }

    int ret = cnvrtr_->nextStep();
    if ( ret < 0 )
	errmsg_ = cnvrtr_->uiMessage();
    return ret;
}


bool Table::AscIO::putHdrVals( od_ostream& strm ) const
{
    errmsg_ = toUiString("TODO: Table::AscIO::putHdrVals not implemented");
    return false;
}


bool Table::AscIO::putNextBodyVals( od_ostream& strm ) const
{
    errmsg_ = toUiString("TODO: Table::AscIO::putNextBodyVals not implemented");
    return false;
}


BufferString Table::AscIO::getText( int ifld ) const
{
    BufferString txt = "";

    if ( vals_.validIdx(ifld) )
    {
	txt = vals_.get(ifld);
	BufferString firstchar;
	firstchar.add( txt[0] );
	const BufferString chartoavoid = "!";
	if ( firstchar == chartoavoid )
	    txt.replace( "!", "_" );
    }

    return txt;
}


static const char* trimmedNumbStr( const char* sval, bool isint )
{
    if ( !*sval ) return 0;
    const int flg = isint ? true : false;
    if ( isNumberString(sval,flg) )
	return sval;

    while ( *sval && !iswdigit(*sval) && *sval != '.' && *sval != '-' )
	sval++;
    if ( !*sval ) return 0;
    if ( isNumberString(sval,flg) )
	return sval;

    mDeclStaticString( bufstr );
    bufstr = sval;
    sval = bufstr.buf();
    char* ptr = bufstr.getCStr() + bufstr.size() - 1;
    while ( ptr>sval && !iswdigit(*ptr) && *ptr != '.' )
	ptr--;
    *(ptr+1) = '\0';
    return isNumberString(sval,flg) ? sval : 0;
}


int Table::AscIO::getIntValue( int ifld, int udf ) const
{
    if ( !vals_.validIdx(ifld) )
	return mUdf(int);
    const char* sval = trimmedNumbStr( vals_.get(ifld), true );
    if ( !sval || !*sval )
	return mUdf(int);
    int val = toInt( sval );
    return val == udf ? mUdf(int) : val;
}


float Table::AscIO::getFValue( int ifld, float udf ) const
{
    if ( !vals_.validIdx(ifld) )
	return mUdf(float);

    const char* sval = trimmedNumbStr( vals_.get(ifld), false );
    if ( !sval ) return mUdf(float);
    const double val = toDouble( sval );
    if ( mIsEqual(mCast(float,val),udf,mDefEps) )
	return mUdf(float);

    const UnitOfMeasure* unit = units_.size() > ifld ? units_[ifld] : 0;
    return mCast(float, unit ? unit->internalValue( val ) : val );
}


double Table::AscIO::getDValue( int ifld, double udf ) const
{
    if ( !vals_.validIdx(ifld) )
	return mUdf(double);

    const char* sval = trimmedNumbStr( vals_.get(ifld), false );
    if ( !sval ) return mUdf(double);
    double val = toDouble( sval );
    if ( mIsEqual(val,udf,mDefEps) )
	return mUdf(double);

    const UnitOfMeasure* unit = units_.size() > ifld ? units_[ifld] : 0;
    return unit ? unit->internalValue( val ) : val;
}


Coord Table::AscIO::getPos( int xfld, int yfld, double udf, bool isll,
			    ConstRefMan<Coords::CoordSystem> outcrs ) const
{
    Coord curpos;

    curpos.x = getDValue(xfld);
    curpos.y = getDValue(yfld);

    if ( !curpos.isUdf() )
    {
	for ( int idx=0; idx<fd_.bodyinfos_.size(); idx++ )
	{
	    ConstRefMan<Coords::CoordSystem> inpcrs = isll ?
				fd_.bodyinfos_[idx]->selection_.llsys_ :
				fd_.bodyinfos_[idx]->selection_.coordsys_;
	    if ( !inpcrs ) continue;

	    if ( inpcrs && outcrs && !(*inpcrs == *outcrs) )
		curpos.setFrom( outcrs->convertFrom(curpos,*inpcrs) );

	    break;
	}
    }

    return curpos;
}


Coord3 Table::AscIO::getPos3D( int xfld, int yfld, int zfld, double udf,
		bool isll, ConstRefMan<Coords::CoordSystem> outcrs ) const
{
    return Coord3( getPos(xfld,yfld,udf,isll,outcrs), getDValue(zfld) );
}


BinID Table::AscIO::getBinID( int inlfld, int crlfld, double udf ) const
{
    BinID bid;
    bid.inl() = getIntValue(inlfld);
    bid.crl() = getIntValue(crlfld);
    return bid;
}


int Table::AscIO::formOf( bool hdr, int iinf ) const
{
    const ObjectSet<TargetInfo>& tis = hdr ? fd_.headerinfos_ : fd_.bodyinfos_;
    if ( tis.size() <= iinf ) return 0;

    return tis[iinf]->selection_.form_;
}


int Table::AscIO::columnOf( bool hdr, int iinf,int ielem ) const
{
    const ObjectSet<TargetInfo>& tis = hdr ? fd_.headerinfos_ : fd_.bodyinfos_;
    if ( tis.size() <= iinf ) return 0;

    return tis[iinf]->selection_.elems_[ielem].pos_.col();
}
