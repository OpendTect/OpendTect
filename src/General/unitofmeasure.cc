/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id$";

#include "unitofmeasure.h"
#include "ascstream.h"
#include "separstr.h"
#include "survinfo.h"
#include "strmprov.h"
#include "file.h"
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


const UnitOfMeasure* UnitOfMeasure::surveyDefZUnit()
{
    if ( SI().zIsTime() )
	return UoMR().get( "Milliseconds" );
    else
	return surveyDefDepthUnit();
}


const char* UnitOfMeasure::surveyDefZUnitAnnot( bool symb, bool withparens )
{
    return zUnitAnnot( SI().zIsTime(), symb, withparens );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefDepthUnit()
{
    return UoMR().get( SI().depthsInFeetByDefault() ? "Feet" : "Meter" );
}


const char* UnitOfMeasure::surveyDefDepthUnitAnnot( bool symb, bool withparens )
{
    return zUnitAnnot( false, symb, withparens );
}


const char* UnitOfMeasure::zUnitAnnot( bool time, bool symbol,
				       bool withparens )
{
    if ( time )
    {
	if ( !symbol )
	    return "Milliseconds";
	else
	    return withparens ? "(ms)" : "ms";
    }

    if ( SI().depthsInFeetByDefault() )
    {
	if ( !symbol )
	    return "Feet";
	return withparens ? "(ft)" : "ft";
    }

    if ( !symbol )
	return "Meter";

    return withparens ? "(m)" : "m";
}


bool UnitOfMeasure::isImperial() const
{
    const char* unitnm = name();
    const char* unitsymb = symbol_.buf();
    BufferStringSet needle;
    TypeSet<int> usename;

    needle.add( getDistUnitString( true, false ) ); usename += 0;
    needle.add( "Fahrenheit" ); usename += 1;
    needle.add( "Inch" ); usename += 1;
    needle.add( "psi" ); usename += 0;
    needle.add( "pounds" ); usename += 1;

    for ( int idx=0; idx<needle.size(); idx++ )
    {
	const char* haystack = (bool)usename[idx] ? unitnm : unitsymb;
	if ( strstr(haystack,needle[idx]->buf()) )
	    return true;
    }

    return false;
}




UnitOfMeasureRepository::UnitOfMeasureRepository()
{
    Repos::FileProvider rfp( filenamebase );
    while ( rfp.next() )
	addUnitsFromFile( rfp.fileName(), rfp.source() );
}


void UnitOfMeasureRepository::addUnitsFromFile( const char* fnm,
						Repos::Source src )
{
    if ( !File::exists(fnm) ) return;
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
	double fac = toDouble( fms[2] );
	PropertyRef::StdType stdtype;
	PropertyRef::parseEnumStdType( ptypestr, stdtype );
	UnitOfMeasure un( stream.keyWord(), symb, fac, stdtype );
	if ( sz > 3 )
	{
	    double shft = toDouble( fms[3] );
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
    const BufferString fnm = rfp.fileName( src );

    bool havesrc = false;
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( entries[idx]->source() == src )
	    { havesrc = true; break; }
    }
    if ( !havesrc )
	return !File::exists(fnm) || File::remove( fnm );

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

	FileMultiString fms( PropertyRef::getStdTypeString(uom.propType()) );
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

    switch ( *nm )
    {
    case 'F': case 'f':
	if ( caseInsensitiveEqual(nm,"F",0)
	  || caseInsensitiveEqual(nm,"FT",0)
	  || caseInsensitiveEqual(nm,"FEET",0) )
	    return "ft";
	else if ( caseInsensitiveEqual(nm,"F/S",0)
	       || caseInsensitiveEqual(nm,"F/SEC",0) )
	    return "ft/s";
	else if ( (stringEndsWithCI("/S",nm) || stringEndsWithCI("/SEC",nm))
		    && (matchStringCI("FT",nm) || matchStringCI("FEET",nm)) )
	    return "ft/s";
    break;
    case 'G' : case 'g':
    	if ( matchStringCI("G/cm2s",nm) )
	    return "m/s x g/cc";
	if ( matchStringCI("G/C",nm) || matchStringCI("GM/C",nm)
	  || matchStringCI("GR/C",nm) )
	    return "g/cc";
    break;
    case 'K' : case 'k':
	if ( matchStringCI("kg/m2s",nm) )
	    return "m/s x kg/m3";
	if ( matchStringCI("kg/m2us",nm) )
	    return "kg/m3 / us/m";
    break;
    case 'P' : case 'p':
	if ( caseInsensitiveEqual(nm,"PU",0) )
	    return "%";
    break;
    case 'U' : case 'u':
	if ( matchStringCI("USEC/F",nm) || matchStringCI("US/F",nm) )
	    return "us/ft";
	else if ( matchStringCI("USEC/M",nm) )
	    return "us/m";
    break;
    }

    return 0;
}


const UnitOfMeasure* UnitOfMeasureRepository::get( const char* nm ) const
{
    return findBest( entries, nm );
}


const UnitOfMeasure* UnitOfMeasureRepository::get( PropertyRef::StdType typ,
						   const char* nm ) const
{
    ObjectSet<const UnitOfMeasure> uns; getRelevant( typ, uns );
    return findBest( uns, nm );
}


const UnitOfMeasure* UnitOfMeasureRepository::findBest(
	const ObjectSet<const UnitOfMeasure>& uns, const char* nm ) const
{
    if ( !nm || !*nm ) return 0;

    if ( matchStringCI( "FRAC", nm ) || matchStringCI( "DEC", nm ) ||
	 matchStringCI( "UNITLESS", nm ) )
	nm = "Fraction";

    if ( matchStringCI( "RAT", nm ) )
	nm = "Ratio";

    for ( int idx=0; idx<uns.size(); idx++ )
    {
	if ( caseInsensitiveEqual(uns[idx]->name().buf(),nm,0) )
	    return uns[idx];
    }
    for ( int idx=0; idx<uns.size(); idx++ )
    {
	if ( caseInsensitiveEqual(uns[idx]->symbol(),nm,0) )
	    return uns[idx];
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


const UnitOfMeasure* UnitOfMeasureRepository::getInternalFor(
	                PropertyRef::StdType st ) const
{
    ObjectSet<const UnitOfMeasure> candidates;
    getRelevant( st, candidates );
    for ( int idx=0; idx<candidates.size(); idx++ )
    {
	if ( candidates[idx]->scaler().isEmpty() )
	    return candidates[idx];
    }
    return 0;
}

