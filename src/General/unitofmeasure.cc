/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id: unitofmeasure.cc,v 1.1 2004-02-19 14:02:53 bert Exp $";

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

    mAdd( "Feet", "ft", 0.3048, Dist );
    mAdd( "Inches", "in", 0.0254, Dist );
    mAdd( "Gram/cm3", "g/cc", 1000, Den );
    mAdd( "Feet/second ", "ft/s", 0.3048, Vel );
    mAdd( "Km/second ", "km/s", 1000, Vel );
    mAdd( "Microseconds/feet", "us/ft", 3.28084e-6, Son );
    mAdd( "Microseconds/meter", "us/m", 1e-6, Son );
    mAdd( "Kg/m2us", "kg/m2us", 1000000, AI );
    mAdd( "G/ft2s", "g/ft2s", 0.0107639, AI );
    mAdd( "Percent", "%", 0.01, Por );
    mAdd( "Percent", "%", 0.01, Sat );

    UnitOfMeasure degf( "Degrees Fahrenheit", "deg.F", 1, PropertyRef::Temp );
    degf.setScaler( LinScaler(-160./9.,5./9.) );
    add( degf );
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
