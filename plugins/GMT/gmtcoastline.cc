/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtcoastline.h"

#include "draw.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "strmprov.h"
#include "od_iostream.h"


mDefineNameSpaceEnumUtils(ODGMT,Resolution,"Resolutions")
{ "Full", "High", "Intermediate", "Low", "Crude", 0 };


static const char* sResKeys[] = { "f", "h", "i", "l", "c", 0 };

int GMTCoastline::factoryid_ = -1;

void GMTCoastline::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Coastline", GMTCoastline::createInstance );
}

GMTPar* GMTCoastline::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTCoastline( iop, workdir );
}


const char* GMTCoastline::userRef() const
{
    BufferString* str = new BufferString( "Coastline: " );
    const BufferString res = find( ODGMT::sKeyResolution() );
    *str += res; *str += " resolution";
    return str->buf();
}


bool GMTCoastline::fillLegendPar( IOPar& par ) const
{
    par.set( sKey::Name(), "Coastline" );
    const BufferString str = find( ODGMT::sKeyLineStyle() );
    par.set( ODGMT::sKeyLineStyle(), str );
    par.set( ODGMT::sKeyShape(), "Line" );
    return true;
}


bool GMTCoastline::doExecute( od_ostream& strm, const char* fnm )
{
    bool drawcontour, dryfill, wetfill;
    Interval<float> mapdim;
    get( ODGMT::sKeyMapDim(), mapdim );
    getYN( ODGMT::sKeyDryFill(), dryfill );
    getYN( ODGMT::sKeyWetFill(), wetfill );
    const int res =
	ODGMT::ResolutionDef().parse( find(ODGMT::sKeyResolution()) );
    OD::LineStyle ls; ls.fromString( find(ODGMT::sKeyLineStyle()) );
    drawcontour = ls.type_ != OD::LineStyle::None;

    strm << "Drawing coastline ...  ";
    FilePath fp( fnm );
    fp.setExtension( "llr" );
    if ( !makeLLRangeFile(fp.fullPath(),strm) )
	mErrStrmRet("Cannot create Lat/Long range file")

    od_istream sd( fp.fullPath() );
    if ( !sd.isOK() )
	mErrStrmRet("Cannot read Lat/Long range file")

    BufferString rangestr( 80, true );
    sd.getLine( rangestr );
    sd.close(); File::remove( fp.fullPath() );
    BufferString jmarg( "-JM" ); jmarg.add( mapdim.start ).add( "c" );
    BufferString darg( "-D" ); darg.add( sResKeys[res] );

    OS::MachineCommand coastmc( "pscoast" );
    coastmc.addArg( jmarg ).addArg( rangestr ).addArg( "-O" ).addArg( "-K" )
	   .addArg( darg );
    if ( drawcontour )
    {
	BufferString lsstr; mGetLineStyleString( ls, lsstr );
	coastmc.addArg( lsstr.insertAt(0,"-W") );
    }
    if ( wetfill )
    {
	OD::Color wetcol;
	get( ODGMT::sKeyWetFillColor(), wetcol );
	BufferString wetcolstr;
	mGetColorString( wetcol, wetcolstr );
	coastmc.addArg( wetcolstr.insertAt(0,"-S") );
    }
    if ( dryfill )
    {
	OD::Color drycol;
	get( ODGMT::sKeyDryFillColor(), drycol );
	BufferString drycolstr;
	mGetColorString( drycol, drycolstr );
	coastmc.addArg( drycolstr.insertAt(0,"-G") );
    }

    if ( !execCmd(coastmc,strm,fnm) )
	mErrStrmRet("Failed")

    strm << "Done" << od_endl;
    return true;
}


bool GMTCoastline::makeLLRangeFile( const char* fnm, od_ostream& strm )
{
    Interval<float> xrg, yrg;
    int zone;
    get( ODGMT::sKeyXRange(), xrg );
    get( ODGMT::sKeyYRange(), yrg );
    get( ODGMT::sKeyUTMZone(), zone );
    int relzone = zone - 30;
    if ( relzone < 1 )
	relzone += 60;

    const int minlong = 6 * ( relzone - 1 );
    BufferString rangestr( "-R" );
    rangestr.add( minlong ).add( "/" ).add( minlong + 6 ).add( "/0/80" );
    const BufferString jarg( "-Ju", zone, "/1:1" );
    const BufferString tmpfilenm( FilePath::getTempFullPath("gmtcoast","dat"));

    OS::MachineCommand mapmc( "mapproject" );
    mapmc.addArg( jarg ).addArg( rangestr )
	 .addArg( "-I" ).addArg( "-F" ).addArg( "-C" );

    od_ostream procstrm = makeOStream( mapmc, strm, tmpfilenm, false );
    if ( !procstrm.isOK() ) return false;

    procstrm << xrg.start << " " << yrg.start << "\n";
    procstrm << xrg.stop  << " " << yrg.start << "\n";
    procstrm << xrg.start << " " << yrg.stop  << "\n";
    procstrm << xrg.stop  << " " << yrg.stop  << "\n";

    procstrm.close();

    OS::MachineCommand minmaxmc( "gmtinfo" );
    minmaxmc.addArg( tmpfilenm ).addArg( "-I0.0001/0.0001" );
    if ( !execCmd(minmaxmc,strm,fnm,false) )
	return false;

    File::remove( tmpfilenm );
    return true;
}
