/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "gmtlocations.h"

#include "draw.h"
#include "envvars.h"
#include "filepath.h"
#include "initgmtplugin.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "keystrs.h"
#include "picksetmanager.h"
#include "wellmanager.h"
#include "strmprov.h"
#include "uistrings.h"


mDefineNameSpaceEnumUtils(ODGMT,Shape,"Shapes")
{ "Star", "Circle", "Diamond", "Square", "Triangle", "Cross", "Polygon",
  "Line", 0 };
template<>
void EnumDefImpl<ODGMT::Shape>::init()
{
    uistrings_ += mEnumTr("Star",0);
    uistrings_ += uiStrings::sCircle();
    uistrings_ += mEnumTr("Diamond",0);
    uistrings_ += uiStrings::sSquare();
    uistrings_ += uiStrings::sTriangle();
    uistrings_ += uiStrings::sCross();
    uistrings_ += uiStrings::sPolygon();
    uistrings_ += uiStrings::sLine();
}

mDefineNameSpaceEnumUtils(ODGMT,Alignment,"Alignments")
{ "Above", "Below", "Left", "Right", 0 };
template<>
void EnumDefImpl<ODGMT::Alignment>::init()
{
    uistrings_ += uiStrings::sAbove();
    uistrings_ += uiStrings::sBelow();
    uistrings_ += uiStrings::sLeft();
    uistrings_ += uiStrings::sRight();
}


// Well Symbols

const char* GMTWellSymbol::sKeyIconFileName()	{ return "Icon File Name"; }
const char* GMTWellSymbol::sKeyDefFileName()	{ return "Def File Name"; }

bool GMTWellSymbol::usePar( const IOPar& par )
{
    FixedString namestr = par.find( sKey::Name() );
    if ( !namestr )
	return false;

    setName( namestr );
    if ( !par.get(sKeyIconFileName(),iconfilenm_)
	    || !par.get(sKeyDefFileName(),deffilenm_) )
	return false;

    return true;
}


const GMTWellSymbolRepository& GMTWSR()
{
    mDefineStaticLocalObject( PtrMan<GMTWellSymbolRepository>, inst,
			      = new GMTWellSymbolRepository );
    return *inst;
}


GMTWellSymbolRepository::GMTWellSymbolRepository()
{
    init();
}


GMTWellSymbolRepository::~GMTWellSymbolRepository()
{
    deepErase( symbols_ );
}


int GMTWellSymbolRepository::size() const
{
    return symbols_.size();
}


void GMTWellSymbolRepository::init()
{
    const char* gmt5sharedir = GetOSEnvVar( "GMT5_SHAREDIR" );
    const char* gmtsharedir = gmt5sharedir ? gmt5sharedir
					   : GetOSEnvVar( "GMT_SHAREDIR" );
    if ( !gmtsharedir || !*gmtsharedir )
	return;

    const File::Path fp( gmtsharedir, "custom", "indexfile" );
    IOPar par;
    if ( !par.read(fp.fullPath(),0) || !par.size() )
	return;

    for ( int idx=0; idx<100; idx++ )
    {
	PtrMan<IOPar> subpar = par.subselect( idx );
	if ( !subpar ) return;

	GMTWellSymbol* symbol = new GMTWellSymbol;
	if ( symbol->usePar(*subpar) )
	    symbols_ += symbol;
	else
	    delete symbol;
    }
}


const GMTWellSymbol* GMTWellSymbolRepository::get( int idx ) const
{
    return symbols_.validIdx(idx) ? symbols_[idx] : 0;
}


const GMTWellSymbol* GMTWellSymbolRepository::get( const char* nm ) const
{
    for ( int idx=0; idx<symbols_.size(); idx++ )
    {
	if ( symbols_[idx]->hasName(nm) )
	    return symbols_[idx];
    }

    return 0;
}


// GMTLocations
int GMTLocations::factoryid_ = -1;

void GMTLocations::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Locations", GMTLocations::createInstance );
}

GMTPar* GMTLocations::createInstance( const IOPar& iop )
{
    return new GMTLocations( iop );
}


const char* GMTLocations::userRef() const
{
    BufferString* str = new BufferString( "Locations: " );
    const char* nm = find( sKey::Name() );
    *str += nm;
    return str->buf();
}


bool GMTLocations::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    str = find( ODGMT::sKeyShape() );
    par.set( ODGMT::sKeyShape(), str );
    str = find( sKey::Size() );
    par.set( sKey::Size(), str );
    str = find( sKey::Color() );
    par.set( sKey::Color(), str );
    str = find( ODGMT::sKeyFill() );
    par.set( ODGMT::sKeyFill(), str );
    str = find( ODGMT::sKeyFillColor() );
    if ( !str.isEmpty() )
	par.set( ODGMT::sKeyFillColor(), str );

    return true;
}


bool GMTLocations::execute( od_ostream& strm, const char* fnm )
{
    DBKey id;
    get( sKey::ID(), id );

    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, uirv );
    if ( !ps )
	mErrStrmRet( uirv.getText() )

    strm << "Posting Locations " << ps->name() << " ...  ";

    Color outcol; get( sKey::Color(), outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );
    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );

    float sz;
    get( sKey::Size(), sz );
    const int shape = ODGMT::ShapeDef().parse( find(ODGMT::sKeyShape()) );

    BufferString comm = "@psxy ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -O -K -S";
    comm += ODGMT::sShapeKeys()[shape]; comm += sz;
    comm += " -W1p,"; comm += outcolstr;
    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed to overlay locations")

    Pick::SetIter psiter( *ps );
    while ( psiter.next() )
    {
	const Coord pos = psiter.getPos();
	procstrm << pos.x_ << " " << pos.y_ << "\n";
    }
    psiter.retire();

    strm << "Done" << od_endl;
    return true;
}



int GMTPolyline::factoryid_ = -1;

void GMTPolyline::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Polyline", GMTPolyline::createInstance );
}

GMTPar* GMTPolyline::createInstance( const IOPar& iop )
{
    return new GMTPolyline( iop );
}


const char* GMTPolyline::userRef() const
{
    BufferString* str = new BufferString( "Polyline: " );
    const char* nm = find( sKey::Name() );
    *str += nm;
    return str->buf();
}


bool GMTPolyline::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name() );
    par.set( sKey::Name(), str );
    par.set( ODGMT::sKeyShape(), "Polygon" );
    par.set( sKey::Size(), 1 );
    str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    str = find( ODGMT::sKeyFill() );
    par.set( ODGMT::sKeyFill(), str );
    str = find( ODGMT::sKeyFillColor() );
    if ( !str.isEmpty() )
	par.set( ODGMT::sKeyFillColor(), str );

    return true;
}


bool GMTPolyline::execute( od_ostream& strm, const char* fnm )
{
    DBKey id; get( sKey::ID(), id );
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, uirv );
    if ( !ps )
	mErrStrmRet( uirv.getText() )

    strm << "Posting Polyline " << ps->name() << " ...  ";

    OD::LineStyle ls;
    const char* lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );

    bool drawline = true;
    if ( ls.type_ == OD::LineStyle::None && dofill )
	drawline = false;

    BufferString comm = "@psxy ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -O -K";
    if ( ps->connection() == Pick::Set::Disp::Close )
	comm += " -L";

    if ( drawline )
    {
	BufferString inpstr; mGetLineStyleString( ls, inpstr );
	comm += " -W"; comm += inpstr;
    }

    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay polylines")

    Pick::SetIter psiter( *ps );
    while ( psiter.next() )
    {
	const Coord pos = psiter.getPos();
	procstrm << pos.x_ << " " << pos.y_ << "\n";
    }
    psiter.retire();

    strm << "Done" << od_endl;
    return true;
}


int GMTWells::factoryid_ = -1;

void GMTWells::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Wells", GMTWells::createInstance );
}

GMTPar* GMTWells::createInstance( const IOPar& iop )
{
    return new GMTWells( iop );
}


const char* GMTWells::userRef() const
{
    BufferString* str = new BufferString( "Wells: " );
    BufferStringSet nms;
    get( sKey::Name(), nms );
    if ( nms.size() )
    {
	*str += nms.get( 0 );
	*str += "....";
    }

    return str->buf();
}


bool GMTWells::fillLegendPar( IOPar& par ) const
{
    par.set( sKey::Name(), find(sKey::Name()) );

    FixedString str = find( sKey::Color() );
    par.set( sKey::Color(), str );
    str = find( sKey::Size() );
    par.set( sKey::Size(), str );

    bool usewellsymbols = false;
    getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbols );
    par.setYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbols );
    if ( usewellsymbols )
    {
	str = find( ODGMT::sKeyWellSymbolName() );
	par.set( ODGMT::sKeyWellSymbolName(), str );
    }
    else
    {
	str = find( ODGMT::sKeyShape() );
	par.set( ODGMT::sKeyShape() , str );
	str = find( ODGMT::sKeyFill() );
	par.set( ODGMT::sKeyFill(), str );
	str = find( ODGMT::sKeyFillColor() );
	if ( !str.isEmpty() )
	    par.set( ODGMT::sKeyFillColor(), str );
    }

    return true;
}


bool GMTWells::execute( od_ostream& strm, const char* fnm )
{
    BufferStringSet wellnms;
    strm << "Posting Wells " << " ...  ";
    if ( !get(ODGMT::sKeyWellNames(),wellnms) || !wellnms.size() )
	mErrStrmRet("No wells to post")

    Color outcol; get( sKey::Color(), outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr; comm += " -O -K -S";

    bool usewellsymbols = false;
    getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbols );
    if ( usewellsymbols )
    {
	BufferString wellsymbolnm;
	get( ODGMT::sKeyWellSymbolName(), wellsymbolnm );
	BufferString deffilenm = GMTWSR().get( wellsymbolnm )->deffilenm_;
	comm += "k"; comm += deffilenm;
    }
    else
    {
	const int shape = ODGMT::ShapeDef().parse( find(ODGMT::sKeyShape()) );
	comm += ODGMT::sShapeKeys()[shape];
    }

    float sz;
    get( sKey::Size(), sz );
    comm += " -W"; comm+=sz; comm += "p,"; comm += outcolstr;
    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );
    if ( !usewellsymbols && dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    TypeSet<Coord> surfcoords;
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	DBKey wllky = Well::MGR().getIDByName( wellnms.get(idx) );
	const Coord maploc = Well::MGR().getMapLocation( wllky );
	if ( maploc.isUdf() )
	    mErrStrmRet( BufferString("Cannot get location for ",
				Well::MGR().nameOf(wllky)) )

	surfcoords += maploc;
	procstrm << maploc.x_ << " " << maploc.y_ << " " << sz << "\n";
    }

    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel(), postlabel );
    if ( !postlabel )
    {
	strm << "Done" << od_endl;
	return true;
    }

    ODGMT::Alignment al =
	ODGMT::AlignmentDef().parse( find( ODGMT::sKeyLabelAlignment() ) );

    BufferString alstr;
    float dx = 0, dy = 0;
    switch ( al )
    {
	case ODGMT::Above:	alstr = "BC"; dy = 0.6f * sz; break;
	case ODGMT::Below:	alstr = "TC"; dy = -0.6f * sz; break;
	case ODGMT::Left:	alstr = "RM"; dx = -0.6f * sz; break;
	case ODGMT::Right:	alstr = "LM"; dx = 0.6f * sz; break;
    }

    int fontsz = 10;
    get( ODGMT::sKeyFontSize(), fontsz );
    comm = "@pstext "; comm += rgstr;
    comm += " -D"; comm += dx; comm += "/"; comm += dy;
    if ( GMT::hasModernGMT() )
	comm += " -F+f12p,Sans,"; comm += outcolstr;
    comm += " -O -K 1>> "; comm += fileName( fnm );
    procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed to post labels")

    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	Coord pos = surfcoords[idx];
	procstrm << pos.x_ << " " << pos.y_ << " " << fontsz << " " << 0 << " ";
	procstrm << 4 << " " << alstr << " " << wellnms.get(idx) << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}
