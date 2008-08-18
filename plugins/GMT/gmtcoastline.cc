/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmtcoastline.cc,v 1.1 2008-08-18 11:22:43 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtcoastline.h"

#include "draw.h"
#include "filepath.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"


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
    bool drawcontour = false; BufferString str;
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    if ( drawcontour )
    {
	str = find( ODGMT::sKeyLineStyle );
	par.set( ODGMT::sKeyLineStyle, str );
    }

    bool dofill = false;
    getYN( ODGMT::sKeyFill, dofill );
    if ( dofill )
    {
	par.set( ODGMT::sKeyPostColorBar, true );
	str = find( ODGMT::sKeyDataRange );
	par.set( ODGMT::sKeyDataRange, str );
    }

    return true;
}


bool GMTCoastline::execute( std::ostream& strm, const char* fnm )
{
    bool drawcontour, dryfill, wetfill;
    Interval<float> mapdim;
    get( ODGMT::sKeyMapDim, mapdim );
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    getYN( ODGMT::sKeyDryFill, dryfill );
    getYN( ODGMT::sKeyWetFill, wetfill );

    strm << "Drawing coastline ...  ";
    FilePath fp( fnm );
    fp.setExtension( "llr" );
    makeLLRangeFile( fp.fullPath() );

    StreamData sd = StreamProvider( fp.fullPath() ).makeIStream();
    if ( !sd.usable() )
	mErrStrmRet("Cannot read Lat/Long range file")

    char buf[80];
    sd.istrm->getline( buf, 40, ' ' );
    BufferString rangestr = buf;
    *( rangestr.buf() + rangestr.size() - 1 ) = '\0';
    BufferString comm = "pscoast "; comm += rangestr;
    comm += " -JM"; comm += mapdim.start; comm += "c -D";
    const int res = eEnum( ODGMT::Resolution, find(ODGMT::sKeyResolution) );
    comm += sResKeys[res];
    if ( drawcontour )
    {
	LineStyle ls; ls.fromString( find(ODGMT::sKeyLineStyle) );
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
    comm += " -O -K >> "; comm += fnm;
    if ( system(comm) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;
    return true;
}


bool GMTCoastline::makeLLRangeFile( const char* fnm ) const
{
    Interval<float> xrg, yrg;
    int zone;
    get( ODGMT::sKeyXRange, xrg );
    get( ODGMT::sKeyYRange, yrg );
    get( ODGMT::sKeyUTMZone, zone );
    int relzone = zone - 31;
    if ( relzone < 0 )
	relzone += 60;

    const int minlong = relzone * 6;
    BufferString comm = "@mapproject -R";
    comm += minlong; comm += "/";
    comm += minlong + 6; comm += "/0/80 -Ju";
    comm += zone; comm += "/1:1 -I -F -C | minmax -I0.0001/0.0001 > ";
    comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) return false;

    *sd.ostrm << xrg.start << " " << yrg.start << std::endl;
    *sd.ostrm << xrg.stop << " " << yrg.start << std::endl;
    *sd.ostrm << xrg.start << " " << yrg.stop << std::endl;
    *sd.ostrm << xrg.stop << " " << yrg.stop << std::endl;
    sd.close();
    return true;
}

