/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id: unitofmeasure.cc,v 1.8 2005-02-23 16:49:53 cvsbert Exp $";

#include "unitofmeasure.h"
#include "ascstream.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include "filepath.h"
#include "debug.h"
#include "errh.h"

static const char* filenamebase = "UnitsOfMeasure";


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


UnitOfMeasure& UnitOfMeasure::operator =( const UnitOfMeasure& uom )
{
    if ( this != &uom )
    {
	setName( uom.name() );
	symbol_ = uom.symbol_;
	scaler_ = uom.scaler_;
	proptype_ = uom.proptype_;
	source_ = uom.source_;
    }
    return *this;
}


const UnitOfMeasure* UnitOfMeasure::getGuessed( const char* nm )
{
    const UnitOfMeasure* direct = UoMR().get( nm );
    return direct ? direct : UoMR().get( UoMR().guessedStdName(nm) );
}


UnitOfMeasureRepository::UnitOfMeasureRepository()
{
    Repos::FileProvider rfp( filenamebase );
    addUnitsFromFile( rfp.fileName(), rfp.source() );
    while ( rfp.next() )
	addUnitsFromFile( rfp.fileName(), rfp.source() );

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
    mAdd( "Microseconds/foot", "us/ft", 3.28084e-6, Son );
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


void UnitOfMeasureRepository::addUnitsFromFile( const char* fnm,
						Repos::Source src )
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
	un.setSource( src );
	add( un );
    }

    sd.close();
}


bool UnitOfMeasureRepository::write( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    BufferString fnm = rfp.fileName( src );

    bool havesrc = false;
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( entries[idx]->source() == src )
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
    strm.putHeader( "Units of Measure" );
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	const UnitOfMeasure& uom = *entries[idx];
	if ( uom.source() != src ) continue;

	FileMultiString fms( eString(PropertyRef::StdType,uom.propType()) );
	fms += uom.symbol();
	fms += uom.scaler().toString();
	strm.put( uom.name(), fms );
    }

    sd.close();
    return true;
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
	   || caseInsensitiveEqual(nm,"FT/S",0)
	   || caseInsensitiveEqual(nm,"FEET/S",0) )
	return "ft/s";
    else if ( matchStringCI("USEC/F",nm) || matchStringCI("us/f",nm) )
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
