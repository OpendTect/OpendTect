/*+
________________________________________________________________________

 (C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtcoastline.cc,v 1.9 2010/12/07 22:59:52 cvskris Exp $";

#include "gmtcoastline.h"

#include "draw.h"
#include "filepath.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include <iostream>


DefineNameSpaceEnumNames(ODGMT,Resolution,2,"Resolutions")
{ "Full", "High", "Intermediate", "Low", "Crude", 0 };


static const char* sResKeys[] = { "f", "h", "i", "l", "c", 0 };

int GMTCoastline::factoryid_ = -1;

void GMTCoastline::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Coastline", GMTCoastline::createInstance );
}

GMTPar* GMTCoastline::createInstance( const IOPar& iop )
{
    return new GMTCoastline( iop );
}


const char* GMTCoastline::userRef() const
{
    BufferString* str = new BufferString( "Coastline: " );
    const char* res = find( ODGMT::sKeyResolution );
    *str += res; *str += " resolution";
    return str->buf();
}


bool GMTCoastline::fillLegendPar( IOPar& par ) const
{
    par.set( sKey::Name, "Coastline" );
    const char* str = find( ODGMT::sKeyLineStyle );
    par.set( ODGMT::sKeyLineStyle, str );
    par.set( ODGMT::sKeyShape, "Line" );
    return true;
}


bool GMTCoastline::execute( std::ostream& strm, const char* fnm )
{
    bool drawcontour, dryfill, wetfill;
    Interval<float> mapdim;
    get( ODGMT::sKeyMapDim, mapdim );
    getYN( ODGMT::sKeyDryFill, dryfill );
    getYN( ODGMT::sKeyWetFill, wetfill );

    strm << "Drawing coastline ...  ";
    FilePath fp( fnm );
    fp.setExtension( "llr" );
    if ( !makeLLRangeFile(fp.fullPath(),strm) )
	mErrStrmRet("Cannot create Lat/Long range file")

    StreamProvider sp( fp.fullPath() );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	mErrStrmRet("Cannot read Lat/Long range file")

    char buf[80];
    sd.istrm->getline( buf, 40, ' ' );
    BufferString rangestr = buf;
    sd.close(); sp.remove();
    *( rangestr.buf() + rangestr.size() - 1 ) = '\0';
    BufferString comm = "pscoast "; comm += rangestr;
    comm += " -JM"; comm += mapdim.start; comm += "c -D";
    const int res = ODGMT::parseEnumResolution( find(ODGMT::sKeyResolution) );
    comm += sResKeys[res];
    LineStyle ls; ls.fromString( find(ODGMT::sKeyLineStyle) );
    drawcontour = ls.type_ != LineStyle::None;
    if ( drawcontour )
    {
	BufferString lsstr; mGetLineStyleString( ls, lsstr );
	comm += " -W"; comm += lsstr;
    }
    if ( wetfill )
    {
	Color wetcol;
	get( ODGMT::sKeyWetFillColor, wetcol );
	BufferString wetcolstr;
	mGetColorString( wetcol, wetcolstr );
	comm += " -S"; comm += wetcolstr;
    }
    if ( dryfill )
    {	
	Color drycol;
	get( ODGMT::sKeyDryFillColor, drycol );
	BufferString drycolstr;
	mGetColorString( drycol, drycolstr );
	comm += " -G"; comm += drycolstr;
    }
    comm += " -O -K 1>> "; comm += fileName( fnm );
    if ( !execCmd(comm,strm) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;
    return true;
}


bool GMTCoastline::makeLLRangeFile( const char* fnm, std::ostream& strm )
{
    Interval<float> xrg, yrg;
    int zone;
    get( ODGMT::sKeyXRange, xrg );
    get( ODGMT::sKeyYRange, yrg );
    get( ODGMT::sKeyUTMZone, zone );
    int relzone = zone - 30;
    if ( relzone < 1 )
	relzone += 60;

    const int minlong = 6 * ( relzone - 1 );
    BufferString comm = "@mapproject -R";
    comm += minlong; comm += "/";
    comm += minlong + 6; comm += "/0/80 -Ju";
    comm += zone; comm += "/1:1 -I -F -C 1> \"";
    BufferString tmpfilenm = FilePath::getTempName("dat");
    comm += tmpfilenm; comm += "\"";
    
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) return false;

    *sd.ostrm << xrg.start << " " << yrg.start << std::endl;
    *sd.ostrm << xrg.stop << " " << yrg.start << std::endl;
    *sd.ostrm << xrg.start << " " << yrg.stop << std::endl;
    *sd.ostrm << xrg.stop << " " << yrg.stop << std::endl;
    sd.close();

    comm = "minmax \""; comm += tmpfilenm; comm += "\" -I0.0001/0.0001 1> ";
    comm += fileName( fnm );
    if ( !execCmd(comm,strm) )
	return false;

    StreamProvider(tmpfilenm).remove();
    return true;
}

