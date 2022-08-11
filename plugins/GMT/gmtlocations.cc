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
#include "gmt2dlines.h"
#include "initgmtplugin.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "welldata.h"
#include "wellreader.h"


mDefineNameSpaceEnumUtils(ODGMT,Shape,"Shapes")
{ "Star", "Circle", "Diamond", "Square", "Triangle", "Cross", "Polygon",
  "Line", 0 };

mDefineNameSpaceEnumUtils(ODGMT,Alignment,"Alignments")
{ "Above", "Below", "Left", "Right", 0 };


// Well Symbols

const char* GMTWellSymbol::sKeyIconFileName()	{ return "Icon File Name"; }
const char* GMTWellSymbol::sKeyDefFileName()	{ return "Def File Name"; }

bool GMTWellSymbol::usePar( const IOPar& par )
{
    StringView namestr = par.find( sKey::Name() );
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

GMTPar* GMTLocations::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTLocations( iop, workdir );
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
    StringView str = find( sKey::Name() );
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


bool GMTLocations::doExecute( od_ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID(), id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    strm << "Posting Locations " << setobj->name() << " ...  ";
    BufferString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    OD::Color outcol; get( sKey::Color(), outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );
    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );

    float sz;
    get( sKey::Size(), sz );
    const int shape = ODGMT::ShapeDef().parse( find(ODGMT::sKeyShape()) );
    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    BufferString sstr( ODGMT::sShapeKeys()[shape] );
    sstr.add( sz ).insertAt(0,"-S");
    BufferString w1str( outcolstr ); w1str.insertAt(0,"-W1p,");

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( "-K" )
	.addArg( sstr ).addArg( w1str );

    if ( dofill )
    {
	OD::Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	xymc.addArg( fillcolstr.insertAt(0,"-G") );
    }

    od_ostream procstrm = makeOStream( xymc, strm, fnm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed to overlay locations")

    for ( int idx=0; idx<ps->size(); idx++ )
    {
	const Coord3& pos = ps->getPos( idx );
	procstrm << pos.x << " " << pos.y << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}



int GMTPolyline::factoryid_ = -1;

void GMTPolyline::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Polyline", GMTPolyline::createInstance );
}

GMTPar* GMTPolyline::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTPolyline( iop, workdir );
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
    StringView str = find( sKey::Name() );
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


bool GMTPolyline::doExecute( od_ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID(), id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find pickset")

    strm << "Posting Polyline " << setobj->name() << " ...  ";
    BufferString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    OD::LineStyle ls;
    const char* lsstr = find( ODGMT::sKeyLineStyle() );
    ls.fromString( lsstr );
    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );

    bool drawline = true;
    if ( ls.type_ == OD::LineStyle::None && dofill )
	drawline = false;

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( "-K" );
    if ( ps->disp_.connect_ == Pick::Set::Disp::Close )
	xymc.addArg( "-L" );

    if ( drawline )
    {
	BufferString inpstr; mGetLineStyleString( ls, inpstr );
	xymc.addArg( inpstr.insertAt(0,"-W") );
    }

    if ( dofill )
    {
	OD::Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	xymc.addArg( fillcolstr.insertAt(0,"-G") );
    }

    od_ostream procstrm = makeOStream( xymc, strm, fnm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay polylines")

    for ( int idx=0; idx<ps->size(); idx++ )
    {
	const Coord3& pos = ps->getPos( idx );
	procstrm << pos.x << " " << pos.y << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}


int GMTWells::factoryid_ = -1;

void GMTWells::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Wells", GMTWells::createInstance );
}

GMTPar* GMTWells::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTWells( iop, workdir );
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

    StringView str = find( sKey::Color() );
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


bool GMTWells::doExecute( od_ostream& strm, const char* fnm )
{
    BufferStringSet wellnms;
    strm << "Posting Wells " << " ...  ";
    if ( !get(ODGMT::sKeyWellNames(),wellnms) || !wellnms.size() )
	mErrStrmRet("No wells to post")

    OD::Color outcol; get( sKey::Color(), outcol );
    BufferString outcolstr;
    mGetColorString( outcol, outcolstr );

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")

    OS::MachineCommand xymc( "psxy" );
    xymc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( "-K" );

    BufferString sstr( "-S" );
    bool usewellsymbols = false;
    getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbols );
    if ( usewellsymbols )
    {
	BufferString wellsymbolnm;
	get( ODGMT::sKeyWellSymbolName(), wellsymbolnm );
	BufferString deffilenm = GMTWSR().get( wellsymbolnm )->deffilenm_;
	sstr.add( "k" ).add( deffilenm );
    }
    else
    {
	const int shape = ODGMT::ShapeDef().parse( find(ODGMT::sKeyShape()) );
	sstr.add( ODGMT::sShapeKeys()[shape] );
    }

    xymc.addArg( sstr );

    float sz;
    get( sKey::Size(), sz );
    BufferString wstr( "-W" );
    wstr.add( sz ).add( "p," ).add( outcolstr );
    xymc.addArg( wstr );

    bool dofill;
    getYN( ODGMT::sKeyFill(), dofill );
    if ( !usewellsymbols && dofill )
    {
	OD::Color fillcol;
	get( ODGMT::sKeyFillColor(), fillcol );
	BufferString fillcolstr;
	mGetColorString( fillcol, fillcolstr );
	xymc.addArg( fillcolstr.insertAt(0,"-G") );
    }

    od_ostream procstrm = makeOStream( xymc, strm, fnm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    TypeSet<Coord> surfcoords;
    IOM().to( IOObjContext::WllInf );
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	const IOObj* ioobj = IOM().getLocal( wellnms.get(idx), "Well" );
	RefMan<Well::Data> data = new Well::Data;
	Coord maploc;
	Well::Reader rdr( *ioobj, *data );
	if ( !rdr.getMapLocation(maploc) )
	    mErrStrmRet(BufferString("Cannot get location for ",ioobj->name()))

	surfcoords += maploc;
	procstrm << maploc.x << " " << maploc.y << " " << sz << "\n";
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
    BufferString darg( "-D" );
    darg.add( dx ).add( "/" ).add( dy );
    const bool modern = GMT::hasModernGMT();

    OS::MachineCommand textmc( "pstext" );
    textmc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( "-K" )
	  .addArg( darg );
    if ( modern )
    {
	BufferString farg( "-F" );
	farg.add( "+f" ).add( fontsz ).add( "p,Helvetica," ).add( outcolstr );
	farg.add( "+a+j" );
	textmc.addArg( farg );
    }

    procstrm = makeOStream( textmc, strm, fnm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed to post labels")

    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	GMT2DLines::postText( surfcoords[idx], fontsz, 0.f, alstr,
			      wellnms.get(idx), modern, procstrm );
    }

    strm << "Done" << od_endl;
    return true;
}
