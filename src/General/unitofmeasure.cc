/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id: unitofmeasure.cc,v 1.4 2004-02-27 08:21:31 nanne Exp $";

#include "unitofmeasure.h"
#include "ascstream.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include "debug.h"

UnitOfMeasureRepository& UoMR()
{
    static UnitOfMeasureRepository* umrepo = 0;
    if ( !umrepo )
    {
	if ( DBG::isOn() ) DBG::message( "Creating UnitOfMeasureRepository" );
	umrepo = new UnitOfMeasureRepository;
	if ( DBG::isOn() )
	{
	    BufferString msg( "Total units of measure: " );
	    msg += umrepo->all().size();
	    DBG::message( msg );
	}
    }
    return *umrepo;
}


const UnitOfMeasure* UnitOfMeasure::getGuessed( const char* nm )
{
    const UnitOfMeasure* direct = UoMR().get( nm );
    return direct ? direct : UoMR().get( UoMR().guessedStdName(nm) );
}


UnitOfMeasureRepository::UnitOfMeasureRepository()
{
    BufferString fnm = GetSettingsDir();
    fnm = File_getFullPath( fnm, "unitsofmeasure" );
    addUnitsFromFile(fnm);
    fnm = GetDataFileName( "UnitsOfMeasure" );
    addUnitsFromFile(fnm);

#define mAdd(nm,symb,fac,ptyp) \
    add( UnitOfMeasure( nm, symb, fac, PropertyRef::ptyp ) )

    mAdd( "Seconds", "s", 1, Time );
    mAdd( "Milliseconds", "msec", 0.001, Time );
    mAdd( "Microseconds", "usec", 1e-6, Time );
    mAdd( "Meter", "m", 1, Dist );
    mAdd( "Feet", "ft", 0.3048, Dist );
    mAdd( "Inches", "in", 0.0254, Dist );
    mAdd( "Kg/m3", "kg/m3", 1, Den );
    mAdd( "Gram/cm3", "g/cc", 1000, Den );
    mAdd( "Meter/second", "m/s", 1, Vel );
    mAdd( "Feet/second ", "ft/s", 0.3048, Vel );
    mAdd( "Km/second ", "km/s", 1000, Vel );
    mAdd( "Seconds/meter", "s/m", 1, Son );
    mAdd( "Microseconds/feet", "us/ft", 3.28084e-6, Son );
    mAdd( "Microseconds/meter", "us/m", 1e-6, Son );
    mAdd( "Kg/m2s", "kg/m2s", 1, AI );
    mAdd( "Kg/m2us", "kg/m2us", 1000000, AI );
    mAdd( "G/ft2s", "g/ft2s", 0.0107639, AI );
    mAdd( "Fraction", "", 1, Por );
    mAdd( "Percent", "%", 0.01, Por );
    mAdd( "Darcy", "k", 1, Perm );
    mAdd( "Fraction", "", 1, Sat );
    mAdd( "Percent", "%", 0.01, Sat );
    mAdd( "API", "API", 1, GR );
    mAdd( "Volt", "V", 1, ElPot );
    mAdd( "Ohm", "ohm", 1, Res );
    mAdd( "Ratio", "", 1, PR );
    mAdd( "1/Pascal", "1/Pa", 1, Comp );
    mAdd( "Kelvin", "K", 1, Temp );

    UnitOfMeasure degc( "Degrees Celcius", "deg.C", 1, PropertyRef::Temp );
    degc.setScaler( LinScaler(273.15,1) );
    add( degc );
    UnitOfMeasure degf( "Degrees Fahrenheit", "deg.F", 1, PropertyRef::Temp );
    degf.setScaler( LinScaler(273.15-160./9.,5./9.) );
    add( degf );

    mAdd( "Pascal", "Pa", 1, Pres );
    mAdd( "Bar", "bar", 1e5, Pres );
}


void UnitOfMeasureRepository::addUnitsFromFile( const char* fnm )
{
    if ( !File_exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream stream( *sd.istrm, true );
    while ( !atEndOfSection( stream.next() ) )
    {
	FileMultiString fms( stream.value() );
	const int sz = fms.size();
	if ( sz < 3 ) continue;
	BufferString ptypestr = fms[0];
	BufferString symb = fms[1];
	double fac = atof( fms[2] );
	UnitOfMeasure un( stream.keyWord(), symb, fac,
			  eEnum(PropertyRef::StdType,ptypestr) );
	if ( sz > 3 )
	{
	    double shft = atof( fms[3] );
	    un.setScaler( LinScaler(shft,fac) );
	}
	add( un );
    }

    sd.close();
}


const char* UnitOfMeasureRepository::guessedStdName( const char* nm )
{
    if ( !nm || !*nm ) return 0;

    if ( caseInsensitiveEqual(nm,"PU",0) )
	return "%";
    else if ( caseInsensitiveEqual(nm,"F",0)
	   || caseInsensitiveEqual(nm,"FEET",0) )
	return "ft";
    else if ( caseInsensitiveEqual(nm,"F/S",0)
	   || caseInsensitiveEqual(nm,"FEET/S",0) )
	return "ft/s";
    else if ( matchStringCI("USEC/F",nm) )
	return "us/ft";
    else if ( matchStringCI("USEC/M",nm) )
	return "us/m";
    else if ( matchStringCI("G/C",nm) )
	return "g/cc";

    return 0;
}


const UnitOfMeasure* UnitOfMeasureRepository::get( const char* nm ) const
{
    if ( !nm || !*nm ) return 0;

    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( caseInsensitiveEqual(entries[idx]->name().buf(),nm,0) )
	    return entries[idx];
    }
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( caseInsensitiveEqual(entries[idx]->symbol(),nm,0) )
	    return entries[idx];
    }
    return 0;
}


bool UnitOfMeasureRepository::add( const UnitOfMeasure& uom )
{
    if ( get(uom.name()) || get(uom.symbol()) )
	return false;

    entries += new UnitOfMeasure( uom );
    return true;
}


void UnitOfMeasureRepository::getRelevant(
		PropertyRef::StdType typ,
		ObjectSet<const UnitOfMeasure>& ret ) const
{
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( entries[idx]->propType() == typ )
	    ret += entries[idx];
    }
}
