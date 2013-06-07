/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtlocations.cc,v 1.17 2011/12/14 13:16:41 cvsbert Exp $";

#include "gmtlocations.h"

#include "draw.h"
#include "envvars.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "strmdata.h"
#include "strmprov.h"
#include "welldata.h"
#include "wellreader.h"


DefineNameSpaceEnumNames(ODGMT,Shape,3,"Shapes")
{ "Star", "Circle", "Diamond", "Square", "Triangle", "Cross", "Polygon",
  "Line", 0 };

DefineNameSpaceEnumNames(ODGMT,Alignment,1,"Alignments")
{ "Above", "Below", "Left", "Right", 0 };


// Well Symbols

const char* GMTWellSymbol::sKeyIconFileName()	{ return "Icon File Name"; }
const char* GMTWellSymbol::sKeyDefFileName()	{ return "Def File Name"; }

bool GMTWellSymbol::usePar( const IOPar& par )
{
    FixedString namestr = par.find( sKey::Name );
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
    static GMTWellSymbolRepository* inst = 0;
    if ( !inst )
	inst = new GMTWellSymbolRepository;

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
    const char* gmtsharedir = GetOSEnvVar( "GMT_SHAREDIR" );
    if ( !gmtsharedir || !*gmtsharedir )
	return;

    const FilePath fp( gmtsharedir, "custom", "indexfile" );
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
	if ( symbols_[idx]->name() == nm )
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
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTLocations::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name );
    par.set( sKey::Name, str );
    str = find( ODGMT::sKeyShape );
    par.set( ODGMT::sKeyShape, str );
    str = find( sKey::Size );
    par.set( sKey::Size, str );
    str = find( sKey::Color );
    par.set( sKey::Color, str );
    str = find( ODGMT::sKeyFill );
    par.set( ODGMT::sKeyFill, str );
    str = find( ODGMT::sKeyFillColor );
    if ( !str.isEmpty() )
	par.set( ODGMT::sKeyFillColor, str );

    return true;
}


bool GMTLocations::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    strm << "Posting Locations " << setobj->name() << " ...  ";
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    Color outcol; get( sKey::Color, outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );
    bool dofill;
    getYN( ODGMT::sKeyFill, dofill );

    float sz;
    get( sKey::Size, sz );
    const int shape = ODGMT::parseEnumShape( find(ODGMT::sKeyShape) );

    BufferString comm = "@psxy ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -O -K -S";
    comm += ODGMT::sShapeKeys[shape]; comm += sz;
    comm += " -W1p,"; comm += outcolstr;
    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor, fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay locations")

    for ( int idx=0; idx<ps.size(); idx++ )
	*sd.ostrm << ps[idx].pos.x << " " << ps[idx].pos.y << std::endl;

    sd.close();
    strm << "Done" << std::endl;
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
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTPolyline::fillLegendPar( IOPar& par ) const
{
    FixedString str = find( sKey::Name );
    par.set( sKey::Name, str );
    par.set( ODGMT::sKeyShape, "Polygon" );
    par.set( sKey::Size, 1 );
    str = find( ODGMT::sKeyLineStyle );
    par.set( ODGMT::sKeyLineStyle, str );
    str = find( ODGMT::sKeyFill );
    par.set( ODGMT::sKeyFill, str );
    str = find( ODGMT::sKeyFillColor );
    if ( !str.isEmpty() )
	par.set( ODGMT::sKeyFillColor, str );

    return true;
}


bool GMTPolyline::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    strm << "Posting Polyline " << setobj->name() << " ...  ";
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    LineStyle ls;
    const char* lsstr = find( ODGMT::sKeyLineStyle );
    ls.fromString( lsstr );
    bool dofill;
    getYN( ODGMT::sKeyFill, dofill );

    bool drawline = true;
    if ( ls.type_ == LineStyle::None && dofill )
	drawline = false;

    BufferString comm = "@psxy ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -O -K";
    if ( ps.disp_.connect_ == Pick::Set::Disp::Close )
	comm += " -L";

    if ( drawline )
    {
	BufferString inpstr; mGetLineStyleString( ls, inpstr );
	comm += " -W"; comm += inpstr;
    }

    if ( dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor, fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay polylines")

    for ( int idx=0; idx<ps.size(); idx++ )
	*sd.ostrm << ps[idx].pos.x << " " << ps[idx].pos.y << std::endl;

    sd.close();

    strm << "Done" << std::endl;
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
    get( sKey::Name, nms );
    if ( nms.size() )
    {
	*str += nms.get( 0 );
	*str += "....";
    }

    return str->buf();
}


bool GMTWells::fillLegendPar( IOPar& par ) const
{
    par.set( sKey::Name, find(sKey::Name) );

    FixedString str = find( sKey::Color );
    par.set( sKey::Color, str );
    str = find( sKey::Size );
    par.set( sKey::Size, str );

    bool usewellsymbols = false;
    getYN( ODGMT::sKeyUseWellSymbolsYN, usewellsymbols );
    par.setYN( ODGMT::sKeyUseWellSymbolsYN, usewellsymbols );
    if ( usewellsymbols )
    {
	str = find( ODGMT::sKeyWellSymbolName );
	par.set( ODGMT::sKeyWellSymbolName, str );
    }
    else
    {
	str = find( ODGMT::sKeyShape );
	par.set( ODGMT::sKeyShape , str );
	str = find( ODGMT::sKeyFill );
	par.set( ODGMT::sKeyFill, str );
	str = find( ODGMT::sKeyFillColor );
	if ( !str.isEmpty() )
	    par.set( ODGMT::sKeyFillColor, str );
    }

    return true;
}


bool GMTWells::execute( std::ostream& strm, const char* fnm )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::WllInf)->id) );
    BufferStringSet wellnms;
    strm << "Posting Wells " << " ...  ";
    if ( !get(ODGMT::sKeyWellNames,wellnms) || !wellnms.size() )
	mErrStrmRet("No wells to post")

    Color outcol; get( sKey::Color, outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );

    BufferString comm = "@psxy ";
    BufferString rgstr; mGetRangeProjString( rgstr, "X" );
    comm += rgstr; comm += " -O -K -S";

    bool usewellsymbols = false;
    getYN( ODGMT::sKeyUseWellSymbolsYN, usewellsymbols );
    if ( usewellsymbols )
    {
	BufferString wellsymbolnm;
	get( ODGMT::sKeyWellSymbolName, wellsymbolnm );
	BufferString deffilenm = GMTWSR().get( wellsymbolnm )->deffilenm_;
	comm += "k"; comm += deffilenm;
    }
    else
    {
	const int shape = ODGMT::parseEnumShape( find(ODGMT::sKeyShape) );
	comm += ODGMT::sShapeKeys[shape];
    }

    float sz;
    get( sKey::Size, sz );
    comm += " -W"; comm+=sz; comm += "p,"; comm += outcolstr;
    bool dofill;
    getYN( ODGMT::sKeyFill, dofill );
    if ( !usewellsymbols && dofill )
    {
	Color fillcol;
	get( ODGMT::sKeyFillColor, fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	comm += " -G";
	comm += fillcolstr;
    }

    comm += " 1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed")

    TypeSet<Coord> surfcoords;
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	const IOObj* ioobj = IOM().getLocal( wellnms.get(idx) );
	Well::Data data;
	Well::Reader rdr( ioobj->fullUserExpr(true), data );
	if ( !rdr.getInfo() )
	    mErrStrmRet("Cannot read well info")

	Coord surfcoord = data.info().surfacecoord;
	surfcoords += surfcoord;
	*sd.ostrm << surfcoord.x << " " << surfcoord.y << " " << sz << std::endl;
    }

    sd.close();
    bool postlabel = false;
    getYN( ODGMT::sKeyPostLabel, postlabel );
    if ( !postlabel )
    {
	strm << "Done" << std::endl;
	return true;
    }

    ODGMT::Alignment al =
	ODGMT::parseEnumAlignment( find( ODGMT::sKeyLabelAlignment ) );

    BufferString alstr;
    float dx = 0, dy = 0;
    switch ( al )
    {
	case ODGMT::Above:	alstr = "BC"; dy = 0.6 * sz; break;
	case ODGMT::Below:	alstr = "TC"; dy = -0.6 * sz; break;
	case ODGMT::Left:	alstr = "RM"; dx = -0.6 * sz; break;
	case ODGMT::Right:	alstr = "LM"; dx = 0.6 * sz; break;
    }

    int fontsz = 10;
    get( ODGMT::sKeyFontSize, fontsz );
    comm = "@pstext "; comm += rgstr;
    comm += " -D"; comm += dx; comm += "/"; comm += dy;
    comm += " -G"; comm += outcolstr;
    comm += " -O -K 1>> "; comm += fileName( fnm );
    sd = makeOStream( comm, strm );
    if ( !sd.usable() )
	mErrStrmRet("Failed to post labels")

    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	Coord pos = surfcoords[idx];
	*sd.ostrm << pos.x << " " << pos.y << " " << fontsz << " " << 0 << " ";
	*sd.ostrm << 4 << " " << alstr << " " << wellnms.get(idx) << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}
