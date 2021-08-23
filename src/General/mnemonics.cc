/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Khushnood Qadir
 * DATE     : Aug 2020
-*/



#include "mnemonics.h"

#include "ascstream.h"
#include "ioman.h"
#include "globexpr.h"
#include "safefileio.h"
#include "separstr.h"
#include "unitofmeasure.h"

static const char* filenamebase = "Mnemonics";
const char* Mnemonic::sKeyMnemonic()	{ return "Mnemonic"; }

//------- Mnemonic::DispDefs ----------

Mnemonic::DispDefs::DispDefs()
{
}


Mnemonic::DispDefs::~DispDefs()
{
}


bool Mnemonic::DispDefs::operator ==( const DispDefs& oth ) const
{
    if ( this == &oth )
	return true;

    return color_ == oth.color_ &&
	   scale_ == oth.scale_ &&
	   unitlbl_ == oth.unitlbl_ &&
	   range_ == oth.range_ &&
	   typicalrange_ == oth.typicalrange_;
}


bool Mnemonic::DispDefs::operator !=( const DispDefs& oth ) const
{
    return !(*this == oth );
}


bool Mnemonic::DispDefs::setUnit( const char* newunitlbl )
{
    const UnitOfMeasure* olduom = UoMR().get( unitlbl_ );
    const UnitOfMeasure* newuom = UoMR().get( newunitlbl );
    if ( newuom == olduom )
	return false;

    unitlbl_.set( newunitlbl );
    convValue( range_.start, olduom, newuom );
    convValue( range_.stop, olduom, newuom );
    convValue( typicalrange_.start, olduom, newuom );
    convValue( typicalrange_.stop, olduom, newuom );

    return true;
}


float Mnemonic::DispDefs::commonValue() const
{
    const Interval<float>& range = defRange();
    const bool udf0 = mIsUdf(range.start);
    const bool udf1 = mIsUdf(range.stop);
    if ( udf0 && udf1 )
	return 0.f;

    if ( udf0 || udf1 )
	return udf0 ? range.stop : range.start;

    return range.center();
}


//------- Mnemonic ----------

mDefineEnumUtils(Mnemonic,StdType,"Standard Property")
{
	"Anisotropy",
	"Area",
	"Classification",
	"Compressibility",
	"Density",
	"Distance/Depth",
	"Elastic Ratio",
	"Electrical Potential",
	"Gamma Ray",
	"Impedance",
	"Permeability",
	"Pressure",
	"Pressure Gradient",
	"Pressure-Weight",
	"Resistivity",
	"Sonic travel time",
	"Temperature",
	"Time",
	"Velocity",
	"Volumetrics", // ratios: relative
	"Volume", // absolute
	"Other",
	nullptr
};


Mnemonic::StdType Mnemonic::surveyZType()
{
    return SI().zIsTime() ? Time : Dist;
}


mDefineEnumUtils(Mnemonic,Scale,"Plot Scale")
{
    "Linear",
    "Logarithmic",
    nullptr
};


Mnemonic::Mnemonic( const char* nm, StdType stdtype )
    : NamedObject(nm)
    , stdtype_(stdtype)
{}


Mnemonic::Mnemonic( const Mnemonic& mnc )
    : NamedObject(mnc.name())
{
    *this = mnc;
}


Mnemonic::~Mnemonic()
{
}


Mnemonic& Mnemonic::operator =( const Mnemonic& oth )
{
    if ( this != &oth )
    {
	NamedObject::setName( oth.name() );
	stdtype_ = oth.stdtype_;
	logtypename_ = oth.logtypename_;
	aliases_ = oth.aliases_;
	disp_ = oth.disp_;
    }
    return *this;
}


bool Mnemonic::operator ==( const Mnemonic& oth ) const
{
    if ( this == &oth )
	return true;

    return stdtype_ == oth.stdtype_ &&
	   name() == oth.name() &&
	   logtypename_ == oth.logtypename_ &&
	   aliases_ == oth.aliases_ &&
	   disp_ == oth.disp_;
}


bool Mnemonic::operator !=( const Mnemonic& oth ) const
{
    return !(*this == oth);
}


bool Mnemonic::matches( const char* nm, bool matchaliases ) const
{
    return matchaliases ? isKnownAs( nm ) : name() == nm;
}


bool Mnemonic::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;

    if ( name().matches(nm,CaseInsensitive) )
	return true;

    for ( const auto* aliasnm : aliases_ )
    {
	const GlobExpr ge( *aliasnm, CaseInsensitive );
	if ( ge.matches(nm) )
	    return true;
    }
    return false;
}


void Mnemonic::usePar( const IOPar& iop )
{
    aliases_.erase();
    FileMultiString fms( iop.find(name()) );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( idx == 0 )
	    logtypename_ = fms[idx];
	else if ( idx == 1 )
	    stdtype_ = StdTypeDef().parse( fms[idx] );
	else if ( idx == 2 )
	    disp_.scale_ = Mnemonic::ScaleDef().parse( fms[2] );
	else if ( idx == 3 )
	    disp_.range_.start = fms.getFValue( 3 );
	else if ( idx == 4 )
	    disp_.range_.stop = fms.getFValue( 4 );
	else if ( idx == 5 )
	    disp_.typicalrange_.start = fms.getFValue( 5 );
	else if ( idx == 6 )
	    disp_.typicalrange_.stop = fms.getFValue( 6 );
	else if ( idx == 7 )
	    disp_.unitlbl_.set( fms[7] ); //Not setUnit!
	else if ( idx >= 8 && idx <= 10 )
	    disp_.color_.set( fms.getFValue(8), fms.getFValue(9),
						fms.getFValue(10) );
	else
	    aliases_.add( fms[idx] );
    }
}


void Mnemonic::fillPar( IOPar& iop ) const
{
    FileMultiString fms( logtypename_ );
    fms += StdTypeDef().toString( stdtype_ );
    fms += Mnemonic::ScaleDef().toString( disp_.scale_ );
    const Interval<float> vrange( disp_.range_ );
    const Interval<float> vtypicalrange( disp_.typicalrange_ );
    fms.add( vrange.start );
    fms.add( vrange.stop );
    fms.add( vtypicalrange.start );
    fms.add( vtypicalrange.stop );
    const char* unitlbl = disp_.getUnitLbl();
    fms += unitlbl && *unitlbl ? unitlbl : "";
    fms += disp_.color_.r();
    fms += disp_.color_.g();
    fms += disp_.color_.b();
    for ( int idx=0; idx<aliases_.size(); idx++ )
	fms += aliases_.get(idx);

    iop.set( name(), fms );
}


const Mnemonic& Mnemonic::undef()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, udf, = nullptr );
    if ( !udf )
    {
	auto* newudf = new Mnemonic( "Other", Other );
	if ( udf.setIfNull(newudf,true) )
	{
	    BufferStringSet& aliases = newudf->aliases();
	    aliases.add( "" ).add( "undef*" ).add( "?undef?" )
		   .add( "?undefined?" )
		   .add( "udf" ).add( "unknown" ).add( "other" );
	    newudf->disp_.color_ = OD::Color::LightGrey();
	}
    }
    return *udf;
}


const Mnemonic& Mnemonic::distance()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, dist, = nullptr );
    if ( !dist )
    {
	auto* ret = new Mnemonic( "DIST", Dist );
	if ( dist.setIfNull(ret,true) )
	{
	    ret->disp_.range_ = Interval<float>( 0.f, mUdf(float) );
	    ret->disp_.typicalrange_ = Interval<float>( 0.f, mUdf(float) );
	    /* NO default unit, as lateral (XY) and vertical(Z) distances
	       may have different units */
	}
    }
    return *dist;
}


const Mnemonic& Mnemonic::volume()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, volume, = nullptr );
    if ( !volume )
    {
	auto* ret = new Mnemonic( "VOL", Volum );
	if ( volume.setIfNull(ret,true) )
	{
	    ret->disp_.range_ = Interval<float>( 0.f, mUdf(float) );
	    ret->disp_.typicalrange_ = Interval<float>( 0.f, mUdf(float) );
	}
    }
    return *volume;
}


const Mnemonic& Mnemonic::defDEN() { return *MNC().getByName("RHOB",false); }
const Mnemonic& Mnemonic::defPVEL() { return *MNC().getByName("PVEL",false); }
const Mnemonic& Mnemonic::defSVEL() { return *MNC().getByName("SVEL",false); }
const Mnemonic& Mnemonic::defDT() { return *MNC().getByName("DT",false); }
const Mnemonic& Mnemonic::defDTS() { return *MNC().getByName("DTS",false); }
const Mnemonic& Mnemonic::defPHI() { return *MNC().getByName("PHI",false); }
const Mnemonic& Mnemonic::defSW() { return *MNC().getByName("SW",false); }
const Mnemonic& Mnemonic::defAI() { return *MNC().getByName("AI",false); }
const Mnemonic& Mnemonic::defSI() { return *MNC().getByName("SI",false); }


//------- MnemonicSetMgr ----------

class MnemonicSetMgr : public CallBacker
{
public:

MnemonicSetMgr()
{
    mAttachCB( IOM().surveyChanged, MnemonicSetMgr::doNull );
}


~MnemonicSetMgr()
{
    detachAllNotifiers();
    delete mns_;
}


void doNull( CallBacker* )
{
    delete mns_;
    mns_ = nullptr;
}


void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    rfp.setSource(Repos::Source::ApplSetup);
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	MnemonicSet* oldmns_ = mns_;
	mns_ = new MnemonicSet;
	mns_->readFrom( astrm );
	if ( mns_->isEmpty() )
	{
	    delete mns_;
	    mns_ = oldmns_;
	}
	else
	{
	    delete oldmns_;
	    sfio.closeSuccess();
	    break;
	}
    }

    if ( !mns_ )
	mns_ = new MnemonicSet;
}

    MnemonicSet* mns_ = nullptr;

};


const MnemonicSet& MNC()
{
    mDefineStaticLocalObject( MnemonicSetMgr, msm, );
    if ( !msm.mns_)
	msm.createSet();

    return *msm.mns_;
}


//------- MnemonicSet ----------

MnemonicSet::MnemonicSet()
    : ManagedObjectSet<Mnemonic>()
{
}


Mnemonic* MnemonicSet::getByName( const char* nm, bool matchaliases )
{
    const Mnemonic* ret =
	const_cast<const MnemonicSet*>(this)->getByName( nm, matchaliases );
    return const_cast<Mnemonic*>( ret );
}


const Mnemonic* MnemonicSet::getByName( const char* nm,
					bool matchaliases ) const
{
    if ( nm && *nm )
    {
	for ( const auto* mnc : *this )
	    if ( mnc->matches(nm,matchaliases) )
		return mnc;
    }

    return nullptr;
}


const Mnemonic& MnemonicSet::getGuessed( Mnemonic::StdType stdtype,
					 const BufferStringSet* hintnms ) const
{
    const MnemonicSelection mnsel( stdtype );
    if ( !hintnms )
	return mnsel.isEmpty() ? Mnemonic::undef() : *mnsel.first();

    for ( const auto* hintnm : *hintnms )
    {
	const Mnemonic* mn = mnsel.getByName( hintnm->str() );
	if ( mn )
	    return *mn;
    }

    return *mnsel.first();
}


const Mnemonic& MnemonicSet::getGuessed( const UnitOfMeasure* uom ) const
{
    if ( !uom )
	return Mnemonic::undef();

    for ( const auto* mnc : *this )
    {
	if ( mnc->hasType(uom->propType()) )
	   return *mnc;
    }

    return Mnemonic::undef();
}


void MnemonicSet::getNames( BufferStringSet& names ) const
{
    for ( const auto* mnc : *this )
	names.add( mnc->name() );
}


MnemonicSet& MnemonicSet::doAdd( Mnemonic* mn )
{
    if ( !mn || getByName(mn->name(),false) )
    {
	delete mn;
	return *this;
    }

    ObjectSet<Mnemonic>::doAdd( mn );
    return *this;
}


void MnemonicSet::readFrom( ascistream& astrm )
{
    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom( astrm );
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    const BufferString mnemonicnm( iop.getKey(idx) );
	    if ( getByName(mnemonicnm,false) )
		continue;

	    auto* mnc = new Mnemonic( mnemonicnm, Mnemonic::Other );
	    mnc->usePar( iop );
	    add( mnc );
	}
    }
}


//------- MnemonicSelection ----------

MnemonicSelection::MnemonicSelection()
    : ObjectSet<const Mnemonic>()
{
}


MnemonicSelection::MnemonicSelection( const Mnemonic* exclude )
    : ObjectSet<const Mnemonic>()
{
    const MnemonicSet& mns = MNC();
    for ( const auto* mnc : mns )
    {
	if ( mnc != exclude )
	    add( mnc );
    }
}


MnemonicSelection::MnemonicSelection( const Mnemonic::StdType stdtyp )
    : ObjectSet<const Mnemonic>()
{
    const MnemonicSet& mns = MNC();
    for ( const auto* mnc : mns )
    {
	if ( mnc->stdType() == stdtyp )
	    add( mnc );
    }
}


const Mnemonic* MnemonicSelection::getByName( const char* nm,
					      bool matchaliases ) const
{
    if ( nm && *nm )
    {
	for ( const auto* mnc : *this )
	    if ( mnc->matches(nm,matchaliases) )
		return mnc;
    }

    return nullptr;
}


void MnemonicSelection::getAll( const BufferStringSet& mnnms,
				MnemonicSelection& ret )
{
    MnemonicSelection mnrefsel;
    const MnemonicSet& mns = MNC();
    for ( const auto* mnnm : mnnms )
	mnrefsel.add( mns.getByName(mnnm->buf(),false) );

    for ( const auto* mn : mns )
    {
	for ( const auto* mnref : mnrefsel )
	{
	    if ( mn == mnref )
	    {
		ret.add( mn );
		mnrefsel -= mn;
		break;
	    }
	}
    }
}


MnemonicSelection MnemonicSelection::getAllVolumetrics()
{
    MnemonicSelection mnsel;
    const BufferStringSet mnrefnms( "VCL" );
    getAll( mnrefnms, mnsel );

    return mnsel;
}


MnemonicSelection MnemonicSelection::getAllPorosity()
{
    MnemonicSelection mnsel;
    BufferStringSet mnrefnms( "PHI", "PHIT", "PHIE" );
    mnrefnms.add( "PHIS" ).add( "PHID" ).add( "PHIN" );
    getAll( mnrefnms, mnsel );

    return mnsel;
}
