/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/


#include "unitofmeasure.h"

#include "ascstream.h"
#include "debug.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "od_iostream.h"
#include "separstr.h"
#include "survinfo.h"
#include "uistrings.h"

static const char* filenamebase = "UnitsOfMeasure";
static const char* sKeyUOM = "UOM";

//Must match UoM data file
static const char* secondsKey = "Seconds";
static const char* millisecondsKey = "Milliseconds";
static const char* feetKey = "Feet";
static const char* meterKey = "Meter";
static const char* ftpersecondKey = "Feet/second";
static const char* mpersecondKey = "Meter/second";



class UnitOfMeasureCurDefsMgr : public CallBacker
{
public:

UnitOfMeasureCurDefsMgr() : iop_(crNew())
{
    IOM().surveyChanged.notify( mCB(this,UnitOfMeasureCurDefsMgr,newSrv) );
    const CallBack savedefscb( mCB(this,UnitOfMeasureCurDefsMgr,saveDefs) );
    IOM().surveyToBeChanged.notify( savedefscb );
    IOM().applicationClosing.notify( savedefscb );
}

~UnitOfMeasureCurDefsMgr()
{ delete iop_; }

void saveDefs( CallBacker* )
{ UnitOfMeasure::saveCurrentDefaults(); }

void newSrv( CallBacker* )
{ delete iop_; iop_ = crNew(); }

static IOPar* crNew()
{
    IOPar* ret = SI().pars().subselect( sKeyUOM );
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
    SI().getPars().mergeComp( currentDefaults(), sKeyUOM );
    SI().savePars();
}


UnitOfMeasureRepository& UoMR()
{
     mDefineStaticLocalObject(PtrMan<UnitOfMeasureRepository>, umrepo,
			      = new UnitOfMeasureRepository);
     return *umrepo;
}



UnitOfMeasure::UnitOfMeasure( const char* nm, const char* symb,
			      double shft, double fact,
			      const TypeSet<PropType>& stdtypes,
			      Repos::Source src )
    : NamedObject(nm)
    , symbol_(symb)
    , scaler_(shft,fact)
    , proptypes_(stdtypes)
    , source_(src)
{
}


UnitOfMeasure::UnitOfMeasure( const UnitOfMeasure& uom )
    : NamedObject(uom.name())
{
    *this = uom;
}


UnitOfMeasure::~UnitOfMeasure()
{
}


UnitOfMeasure& UnitOfMeasure::operator =( const UnitOfMeasure& uom )
{
    if ( this != &uom )
    {
	setName( uom.name() );
	symbol_ = uom.symbol_;
	scaler_ = uom.scaler_;
	proptypes_ = uom.proptypes_;
	source_ = uom.source_;
    }
    return *this;
}


bool UnitOfMeasure::isCompatibleWith( const UnitOfMeasure& oth ) const
{
    for ( const auto& othprtyp : oth.proptypes_ )
	for ( const auto& prtyp : proptypes_ )
	    if ( othprtyp == prtyp )
		return true;

    return false;
}


UnitOfMeasure::PropType UnitOfMeasure::propType( int idx ) const
{
    return proptypes_.validIdx(idx) ? proptypes_[idx] : Mnemonic::Other;
}


const UnitOfMeasure* UnitOfMeasure::getGuessed( const char* nm )
{
    const UnitOfMeasure* direct = UoMR().get( nm );
    return direct ? direct : UoMR().get( UoMR().guessedStdName(nm) );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefZUnit()
{
    return SI().zIsTime() ? surveyDefTimeUnit() : surveyDefDepthUnit();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefZStorageUnit()
{
    return SI().zIsTime() ? UoMR().get( secondsKey )
			  : surveyDefDepthStorageUnit();
}


const UnitOfMeasure* UnitOfMeasure::surveyDefTimeUnit()
{
    return UoMR().get( millisecondsKey );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefTimeStorageUnit()
{
    return UoMR().get( secondsKey );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefDepthUnit()
{
    return UoMR().get( SI().depthsInFeet() ? feetKey : meterKey );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefDepthStorageUnit()
{
    return UoMR().get( SI().zInFeet() ? feetKey : meterKey );
}


const UnitOfMeasure* UnitOfMeasure::surveyDefVelUnit()
{
    return UoMR().get( SI().depthsInFeet() ? ftpersecondKey : mpersecondKey );
}


uiString UnitOfMeasure::surveyDefZUnitAnnot( bool symb, bool withparens )
{
    return zUnitAnnot( SI().zIsTime(), symb, withparens );
}


uiString UnitOfMeasure::surveyDefTimeUnitAnnot( bool symb, bool withparens )
{
    return zUnitAnnot( true, symb, withparens );
}


uiString UnitOfMeasure::surveyDefDepthUnitAnnot( bool symb, bool withparens )
{
    return zUnitAnnot( false, symb, withparens );
}

#define mCreateLbl() \
{ \
    if ( withparens ) lbl.append( "(" ); \
    lbl.append( tr(symb ? uom->symbol() : uom->name()) ); \
    if ( withparens ) lbl.append( ")" ); \
}

uiString UnitOfMeasure::zUnitAnnot( bool time, bool symb, bool withparens )
{
    const UnitOfMeasure* uom = time  ? surveyDefTimeUnit()
				     : surveyDefDepthUnit();
    if ( !uom )
	return uiString::emptyString();

    uiString lbl;
    mCreateLbl()

    return lbl;
}


uiString UnitOfMeasure::surveyDefVelUnitAnnot( bool symb, bool withparens )
{
    const UnitOfMeasure* uom = surveyDefVelUnit();
    if ( !uom )
	return uiString::emptyString();

    uiString lbl;
    mCreateLbl()

    return lbl;
}


const char* UnitOfMeasure::getLabel() const
{
    return symbol_.isEmpty() ? name().str() : symbol();
}


BufferString UnitOfMeasure::getUnitLbl( const UnitOfMeasure* uom,
					const char* deflbl )
{
    BufferString ret;
    if ( uom )
	ret.set( uom->getLabel() );
    if ( ret.isEmpty() && deflbl )
	ret.set( deflbl );

    return ret;
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
    while ( !atEndOfSection(stream.next()) )
    {
	const FileMultiString fms( stream.value() );
	const int sz = fms.size();
	if ( sz < 3 )
	    continue;

	const SeparString types( fms[0], '|' );
	const BufferString symb = fms[1];
	const double fac = fms.getDValue( 2 );
	const double shft = sz > 3 ? fms.getDValue( 3 ) : 0.;

	const int nrtypes = types.size();
	TypeSet<UnitOfMeasure::PropType> stdtypes;
	for ( int ityp=0; ityp<nrtypes; ityp++ )
	{
	    PropType stdtype;
	    Mnemonic::parseEnumStdType( types[ityp], stdtype );
	    stdtypes += stdtype;
	}

	const UnitOfMeasure un( stream.keyWord(), symb, shft, fac,
				stdtypes, src );

	add( un );
    }
}


bool UnitOfMeasureRepository::write( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    const BufferString fnm = rfp.fileName( src );

    bool havesrc = false;
    for ( const auto* uom : entries_ )
    {
	if ( uom->source() == src )
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
    for ( const auto* uom : entries_ )
    {
	if ( uom->source() != src )
	    continue;

	FileMultiString fms( Mnemonic::getStdTypeString(uom->propType()) );
	fms += uom->symbol();
	fms += uom->scaler().toString();
	astrm.put( uom->name(), fms );
    }

    return true;
}


const char* UnitOfMeasureRepository::guessedStdName( const char* nm )
{
    const StringView fsnm( nm );
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

    return nullptr;
}


const UnitOfMeasure* UnitOfMeasureRepository::get( const char* nm ) const
{
    return findBest( entries_, nm );
}


const UnitOfMeasure* UnitOfMeasureRepository::get( PropType typ,
						   const char* nm ) const
{
    ObjectSet<const UnitOfMeasure> uns; getRelevant( typ, uns );
    return findBest( uns, nm );
}


const UnitOfMeasure* UnitOfMeasureRepository::findBest(
	const ObjectSet<const UnitOfMeasure>& uns, const char* nm ) const
{
    const StringView fsnm( nm );
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
	if ( StringView(uns[idx]->symbol()).isEqual(nm,CaseInsensitive) )
	    return uns[idx];
    }

    return nullptr;
}


bool UnitOfMeasureRepository::add( const UnitOfMeasure& uom )
{
    if ( get(uom.name()) || get(uom.symbol()) )
	return false;

    entries_.add( new UnitOfMeasure(uom) );
    return true;
}


void UnitOfMeasureRepository::getRelevant( PropType typ,
			    ObjectSet<const UnitOfMeasure>& ret ) const
{
    for ( const auto* uom : entries_ )
    {
	for ( const auto& stdtype : uom->proptypes_ )
	{
	    if ( stdtype == typ )
	    {
		ret += uom;
		break;
	    }
	}
    }
}


const UnitOfMeasure* UnitOfMeasureRepository::getInternalFor(
							PropType st ) const
{
    ObjectSet<const UnitOfMeasure> candidates;
    getRelevant( st, candidates );
    for ( const auto* uom : candidates )
	if ( uom->scaler().isEmpty() )
	    return uom;

    return nullptr;
}


const UnitOfMeasure* UnitOfMeasureRepository::getCurDefaultFor(
		const char* ky ) const
{
    const StringView res = UnitOfMeasure::currentDefaults().find( ky );
    return res.isEmpty() ? nullptr : get( res );
}


const UnitOfMeasure* UnitOfMeasureRepository::getDefault( const char* ky,
							  PropType st ) const
{
    const UnitOfMeasure* ret = getCurDefaultFor( ky );
    if ( !ret )
	ret = getCurDefaultFor( Mnemonic::toString(st) );
    return ret ? ret : getInternalFor( st );
}
