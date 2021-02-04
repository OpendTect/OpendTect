/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2004
-*/


#include "unitofmeasure.h"
#include "ascstream.h"
#include "separstr.h"
#include "survinfo.h"
#include "od_iostream.h"
#include "file.h"
#include "debug.h"
#include "dbman.h"
#include "uistrings.h"

static const char* filenamebase = "UnitsOfMeasure";
static const char* sKeyUOM = "UOM";

static const char* sKeyFtPerSecond = "Feet/second";
static const char* sKeyMeterPerSecond = "Meter/second";



class UnitOfMeasureCurDefsMgr : public CallBacker
{
public:

UnitOfMeasureCurDefsMgr() : iop_(crNew())
{
    DBM().surveyChanged.notify( mCB(this,UnitOfMeasureCurDefsMgr,newSrv) );
    const CallBack savedefscb( mCB(this,UnitOfMeasureCurDefsMgr,saveDefs) );
    DBM().surveyToBeChanged.notify( savedefscb );
    DBM().applicationClosing.notify( savedefscb );
}

~UnitOfMeasureCurDefsMgr()
{ delete iop_; }

void saveDefs( CallBacker* )
{ UnitOfMeasure::saveCurrentDefaults(); }

void newSrv( CallBacker* )
{ delete iop_; iop_ = crNew(); }

static IOPar* crNew()
{
    IOPar* ret = SI().getDefaultPars().subselect( sKeyUOM );
    if ( !ret ) ret = new IOPar;
    ret->setName( "Current Units of Measure" );
    return ret;
}

    IOPar*	iop_;

};

IOPar& UnitOfMeasure::currentDefaults()
{
    mDefineStaticLocalObject( PtrMan<UnitOfMeasureCurDefsMgr>, uomdef,
			      = new  UnitOfMeasureCurDefsMgr )
    return *uomdef->iop_;
}


void UnitOfMeasure::saveCurrentDefaults()
{
    IOPar pars( SI().getDefaultPars() );
    pars.mergeComp( currentDefaults(), sKeyUOM );
    SI().setDefaultPars( pars, true );
}


UnitOfMeasureRepository& UoMR()
{
     mDefineStaticLocalObject(PtrMan<UnitOfMeasureRepository>, umrepo,
			      = new UnitOfMeasureRepository);
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


const UnitOfMeasure* UnitOfMeasure::feetUnit()
{
    return UoMR().get( sKeyFeet() );
}


const UnitOfMeasure* UnitOfMeasure::meterUnit()
{
    return UoMR().get( sKeyMeter() );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefZUnit()
{
    return SI().zIsTime() ? surveyDefTimeUnit() : surveyDefDepthUnit();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefZStorageUnit()
{
    return SI().zIsTime()
	? UoMR().get( sKeySeconds() )
	: surveyDefDepthStorageUnit();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefTimeUnit()
{
    return UoMR().get( sKeyMilliSeconds() );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefDepthUnit()
{
    return SI().depthsInFeet() ? feetUnit() : meterUnit();
}


const UnitOfMeasure* UnitOfMeasure::xyUnitInObjects()
{
    return SI().xyInFeet() ? feetUnit() : meterUnit();
}


const UnitOfMeasure* UnitOfMeasure::zUnitInObjects()
{
    return SI().zInFeet() ? feetUnit() : meterUnit();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefDepthStorageUnit()
{
    return zUnitInObjects();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefVelUnit()
{
    return UoMR().get( SI().depthsInFeet() ? sKeyFtPerSecond
					   : sKeyMeterPerSecond );
}


uiString UnitOfMeasure::surveyDefZUnitAnnot( bool symb )
{
    return zUnitAnnot( SI().zIsTime(), symb );
}


uiString UnitOfMeasure::surveyDefTimeUnitAnnot( bool symb )
{
    return zUnitAnnot( true, symb );
}


uiString UnitOfMeasure::surveyDefDepthUnitAnnot( bool symb )
{
    return zUnitAnnot( false, symb );
}


#define mRetLbl() \
    return !uom ? uiString::empty() \
		: toUiString(symb ? uom->symbol() : uom->name())

uiString UnitOfMeasure::zUnitAnnot( bool time, bool symb )
{
    const UnitOfMeasure* uom = time  ? surveyDefTimeUnit()
				     : surveyDefDepthUnit();
    mRetLbl();
}


uiString UnitOfMeasure::surveyDefVelUnitAnnot( bool symb )
{
    const UnitOfMeasure* uom = surveyDefVelUnit();
    mRetLbl();
}


bool UnitOfMeasure::isImperial() const
{
    const char* unitnm = name();
    const char* unitsymb = symbol_.str();
    BufferStringSet needles;
    BoolTypeSet usename;

    needles.add( getDistUnitString( true, false ) ); usename += false;
    needles.add( "mile" ); usename += true;
    needles.add( "Acre" ); usename += true;
    needles.add( "Fahrenheit" ); usename += true;
    needles.add( "nch" ); usename += true;
    needles.add( "psi" ); usename += false;
    needles.add( "pounds" ); usename += true;
    needles.add( "Gallon" ); usename += true;
    needles.add( "Barrel" ); usename += true;
    needles.add( "bcf" ); usename += false;

    for ( int idx=0; idx<needles.size(); idx++ )
    {
	const char* haystack = usename[idx] ? unitnm : unitsymb;
	if ( firstOcc(haystack,needles[idx]->str()) )
	    return true;
    }

    return false;
}




UnitOfMeasureRepository::UnitOfMeasureRepository()
{
    DBG::message( DBG_IO, "Creating UnitOfMeasureRepository" );
    Repos::FileProvider rfp( filenamebase );
    while ( rfp.next() )
	addUnitsFromFile( rfp.fileName(), rfp.source() );

    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Total units of measure: " );
	msg += all().size();
	DBG::message( DBG_IO, msg );
    }
}


void UnitOfMeasureRepository::addUnitsFromFile( const char* fnm,
						Repos::Source src )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    ascistream stream( strm, true );
    while ( !atEndOfSection( stream.next() ) )
    {
	FileMultiString fms( stream.value() );
	const int sz = fms.size();
	if ( sz < 3 ) continue;
	SeparString types( fms[0], '|' );
	const BufferString symb = fms[1];
	double fac = fms.getDValue( 2 );
	double shft = sz > 3 ? fms.getDValue( 3 ) : 0.;

	const int nrtypes = types.size();
	for ( int ityp=0; ityp<nrtypes; ityp++ )
	{
	    PropertyRef::StdType stdtype =
                PropertyRef::StdTypeDef().parse( types[ityp] );
	    UnitOfMeasure un( stream.keyWord(), symb, fac, stdtype );
	    un.setScaler( LinScaler(shft,fac) );
	    un.setSource( src );
	    add( un );
	}
    }
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

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	strm.addErrMsgTo( msg ); ErrMsg( fnm );
	return false;
    }

    ascostream astrm( strm );
    astrm.putHeader( "Units of Measure" );
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	const UnitOfMeasure& uom = *entries[idx];
	if ( uom.source() != src ) continue;

	FileMultiString fms( PropertyRef::toString(uom.propType()) );
	fms += uom.symbol();
	fms += uom.scaler().toString();
	astrm.put( uom.name(), fms );
    }

    return true;
}


const char* UnitOfMeasureRepository::guessedStdName( const char* nm )
{
    const FixedString fsnm( nm );
    if ( fsnm.isEmpty() )
	return 0;

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
	else if ( (fsnm.endsWith("/S",CaseInsensitive)
		    || fsnm.endsWith("/SEC",CaseInsensitive))
		&& (fsnm.startsWith("FT",CaseInsensitive)
		    || fsnm.startsWith("FEET",CaseInsensitive)) )
	    return "ft/s";
    break;
    case 'K' : case 'k':
	if ( fsnm.startsWith("kg/m2s",CaseInsensitive) )
	    return "m/s x kg/m3";
	if ( fsnm.startsWith("kg/m2us",CaseInsensitive) )
	    return "kg/m3 / us/m";
    break;
    case 'G' : case 'g':
	if ( caseInsensitiveEqual(nm,"gAPI",0) )
	    return "API";
	if ( fsnm.startsWith("G/cm2s",CaseInsensitive) )
	    return "m/s x g/cc";
	if ( fsnm.startsWith("G/C",CaseInsensitive)
	  || fsnm.startsWith("GM/C",CaseInsensitive)
	  || fsnm.startsWith("GR/C",CaseInsensitive) )
	    return "g/cc";
    break;
    case 'P' : case 'p':
	if ( caseInsensitiveEqual(nm,"PU",0) )
	    return "%";
    break;
    case 'U' : case 'u':
	if ( fsnm.startsWith("USEC/F",CaseInsensitive)
	  || fsnm.startsWith("US/F",CaseInsensitive) )
	    return "us/ft";
	else if ( fsnm.startsWith("USEC/M",CaseInsensitive) )
	    return "us/m";
    break;
    case 'm':
	if ( caseInsensitiveEqual(nm,"m3/m3",0) ) // Petrel special
	    return "";
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
    const FixedString fsnm( nm );
    if ( fsnm.isEmpty() )
	return 0;

    if ( fsnm.startsWith( "FRAC", CaseInsensitive )
      || fsnm.startsWith( "DEC", CaseInsensitive )
      || fsnm.startsWith( "UNITLESS", CaseInsensitive ) )
	nm = "Fraction";

    if ( fsnm.startsWith( "RAT", CaseInsensitive ) )
	nm = "Ratio";

    for ( int idx=0; idx<uns.size(); idx++ )
    {
	if ( uns[idx]->name().isEqual(nm,CaseInsensitive) )
	    return uns[idx];
    }
    for ( int idx=0; idx<uns.size(); idx++ )
    {
	if ( FixedString(uns[idx]->symbol()).isEqual(nm,CaseInsensitive) )
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
	if ( typ == PropertyRef::Other || entries[idx]->propType() == typ )
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


const UnitOfMeasure* UnitOfMeasureRepository::getCurDefaultFor(
		const char* ky ) const
{
    const char* res = UnitOfMeasure::currentDefaults().find( ky );
    return res && *res ? get( res ) : 0;
}


const UnitOfMeasure* UnitOfMeasureRepository::getDefault( const char* ky,
					PropertyRef::StdType st ) const
{
    const UnitOfMeasure* ret = getCurDefaultFor( ky );
    if ( !ret )
	ret = getCurDefaultFor( PropertyRef::toString(st) );
    return ret ? ret : getInternalFor( st );
}
