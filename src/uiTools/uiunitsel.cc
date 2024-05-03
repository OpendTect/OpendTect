/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiunitsel.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include "ioman.h"

static const char* sKeyDispNone = "-";
static const uiString sDispNone = toUiString(sKeyDispNone);


uiUnitSel::Setup::Setup( Mnemonic::StdType st, const uiString& labeltxt,
			 const Mnemonic* mn )
    : ptype_(st)
    , mn_(mn)
    , lbltxt_(labeltxt)
    , mode_(Full)
    , selproptype_(false)
    , selmnemtype_(false)
    , variableszpol_(false)
    , allowneg_(false)
    , withnone_(false)
{
}


uiUnitSel::Setup::Setup( const uiString& txtlbl, const SurveyInfo* si )
    : Setup( Mnemonic::surveyZType(si), txtlbl )
{
    allowneg( true );
}


uiUnitSel::Setup::~Setup()
{
}


// uiUnitSel

uiUnitSel::uiUnitSel( uiParent* p, const uiUnitSel::Setup& su )
    : uiGroup(p,"UnitSel")
    , setup_(su)
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::uiUnitSel( uiParent* p, Mnemonic::StdType st )
    : uiGroup(p,"UnitSel")
    , setup_(st)
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::uiUnitSel( uiParent* p, const Mnemonic* mn )
    : uiGroup(p,"UnitSel")
    , setup_(mn ? mn->stdType()
		: Mnemonic::Dist, uiString::empty(), mn )
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::uiUnitSel( uiParent* p, const char* lbltxt )
    : uiGroup(p,"UnitSel")
    , setup_(SI().zIsTime() ? Mnemonic::Time : Mnemonic::Dist,
	     mToUiStringTodo(lbltxt))
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::~uiUnitSel()
{
    detachAllNotifiers();
}


void uiUnitSel::init()
{
    units_.allowNull( true );
    tblkey_ = setup_.lbltxt_.getFullString();
    if ( tblkey_.isEmpty() )
	tblkey_ = Mnemonic::StdTypeNames()[setup_.ptype_];

    if ( setup_.selproptype_ )
    {
	propfld_ = new uiComboBox( this, "Properties" );
	const BufferStringSet typnms( Mnemonic::StdTypeNames() );
	propfld_->addItems( typnms );
	propfld_->setCurrentItem( (int)setup_.ptype_ );
	mAttachCB( propfld_->selectionChanged, uiUnitSel::propSelChg );
    }

    if ( setup_.selmnemtype_ )
    {
	mnfld_ = new uiComboBox( this, "Mnemonic" );
	mAttachCB( mnfld_->selectionChanged, uiUnitSel::mnSelChg );
	const MnemonicSelection mns = setup_.mn_ &&
				setup_.mn_->stdType() != Mnemonic::Other
			      ? MnemonicSelection( setup_.mn_->stdType() )
			      : (setup_.ptype_ == Mnemonic::Other
				? MnemonicSelection( nullptr )
				: MnemonicSelection( setup_.ptype_ ));

	BufferStringSet mnsnames;
	for ( const auto* mn : mns )
	    mnsnames.add( mn->name() );
	mnsnames.sort();
	mnfld_->addItems( mnsnames );
	if ( setup_.mn_ )
	{
	    mnfld_->setCurrentItem( setup_.mn_->name().buf() );
	    if ( propfld_ )
		mnfld_->attach( rightOf, propfld_ );
	}
    }

    inpfld_ = new uiComboBox( this, "Units" );
    uiObject::SzPolicy szpol;
    if ( setup_.mode_ == Setup::SymbolsOnly )
	szpol = setup_.variableszpol_ ? uiObject::SmallVar : uiObject::Small;
    else if ( setup_.mode_ == Setup::NamesOnly )
	szpol = setup_.variableszpol_ ? uiObject::MedVar : uiObject::Medium;
    else
	szpol = setup_.variableszpol_ ? uiObject::MedVar : uiObject::Wide;

    inpfld_->setHSzPol( szpol );

    mAttachCB( inpfld_->selectionChanged, uiUnitSel::selChg );
    if ( mnfld_ )
	inpfld_->attach( rightOf, mnfld_ );
    else if ( propfld_ )
	inpfld_->attach( rightOf, propfld_ );

    uiComboBox* leftcb = propfld_ ? propfld_ : inpfld_;
    if ( setup_.lbltxt_.isEmpty() )
	setHAlignObj( inpfld_ );
    else
    {
	inplbl_ = new uiLabel( this, setup_.lbltxt_ );
	inplbl_->attach( leftOf, leftcb );
	setHAlignObj( leftcb );
    }

    mAttachCB( postFinalize(), uiUnitSel::initGrp );
}


void uiUnitSel::initGrp( CallBacker* )
{
    update();
    usePar( lastUsed() );
    displayGroup( !inpfld_->isEmpty() );
}


void uiUnitSel::update()
{
    const BufferString olddef( !inpfld_->isEmpty() ? inpfld_->text()
				: (setup_.withnone_ ? sKeyDispNone : "") );
    inpfld_->setEmpty();
    if ( propfld_ )
	setup_.ptype_ = (Mnemonic::StdType)propfld_->currentItem();

    if ( mnfld_ )
    {
	setup_.mn_ = MNC().getByName( mnfld_->text() );
	setup_.ptype_ = setup_.mn_ ? setup_.mn_->stdType() : Mnemonic::Other;
    }

    units_.erase();
    if ( setup_.mn_ )
	UoMR().getRelevant( setup_.mn_->stdType(), units_ );
    else
    {
	if ( setup_.ptype_ == Mnemonic::Other )
	    units_.append( UoMR().all() );
	else
	    UoMR().getRelevant( setup_.ptype_, units_ );
    }

    if ( !setup_.allowneg_ )
    {
	for ( int idx=units_.size()-1; idx>=0; idx-- )
	{
	    if ( units_.get(idx)->scaler().factor < 0. )
		units_.removeSingle( idx );
	}
    }

    if ( setup_.withnone_ )
	units_.insertAt( nullptr, 0 );

    for ( int idx=0; idx<units_.size(); idx++ )
	inpfld_->addItem( getSelTxt(units_[idx]) );

    if ( !olddef.isEmpty() && inpfld_->isPresent(olddef) )
	inpfld_->setText( olddef );
    else if ( setup_.ptype_ == Mnemonic::Dist )
	inpfld_->setText(
		getSelTxt(UnitOfMeasure::surveyDefDepthUnit()).getString() );
    prevuom_ = getUnit();
    displayGroup( !inpfld_->isEmpty() );
}


void uiUnitSel::selChg( CallBacker* )
{
    const UnitOfMeasure* prevuom = prevuom_;
    prevuom_ = gtUnit();
    selChange.trigger( prevuom );
}


void uiUnitSel::propSelChg( CallBacker* )
{
    update();
    propSelChange.trigger();
}


void uiUnitSel::mnSelChg( CallBacker* )
{
    update();
}


void uiUnitSel::setFallbackKey( const char* newky )
{
    if ( tblkey_ == newky )
	return;

    tblkey_ = newky;
    usePar( lastUsed() );
}


IOPar& uiUnitSel::lastUsed()
{
    return UnitOfMeasure::currentDefaults();
}


void uiUnitSel::setPropFld( Mnemonic::StdType typ )
{
    setup_.ptype_ = typ;
    if ( propfld_ )
    {
	NotifyStopper nst( propfld_->selectionChanged );
	propfld_->setCurrentItem( (int)typ );
	inpfld_->setCurrentItem( 0 );
    }
    update();
}


void uiUnitSel::setMnemFld( const Mnemonic* mn )
{
    if ( mn )
	setup_.ptype_ = mn->stdType();

    if ( mnfld_ )
    {
	NotifyStopper nst( mnfld_->selectionChanged );
	mnfld_->setCurrentItem( mn->name().buf() );
	setup_.mn_ = mn;
    }

    update();
}


void uiUnitSel::setUnFld( const UnitOfMeasure* un )
{
    if ( !un && !setup_.withnone_ )
	un = UoMR().getInternalFor( setup_.ptype_ );

    int selidx = -1;
    for ( int idx=0; idx<units_.size(); idx++ )
    {
	if ( units_[idx] == un )
	    { selidx = idx; break; }
    }
    if ( selidx < 0 && setup_.withnone_ )
	selidx = 0;

    if ( selidx >= 0 )
    {
	inpfld_->setCurrentItem( selidx );
	if ( selidx > 0 || !setup_.withnone_ )
	    fillPar( lastUsed() );
    }
}


void uiUnitSel::setUnit( const UnitOfMeasure* un )
{
    if ( un )
    {
	setPropFld( un->propType() );
	if ( setup_.mn_ )
	    setMnemFld( setup_.mn_ );
    }

    setUnFld( un );
}


void uiUnitSel::setUnit( const char* nm )
{
    setUnit( UoMR().get(nm) );
}


const UnitOfMeasure* uiUnitSel::getUnit() const
{
    fillPar( lastUsed() );
    return gtUnit();
}


const UnitOfMeasure* uiUnitSel::gtUnit() const
{
    const int selidx = inpfld_->currentItem();
    return units_.validIdx( selidx ) ? units_[selidx] : nullptr;
}


const char* uiUnitSel::getUnitName() const
{
    const UnitOfMeasure* uom = getUnit();
    const int selidx = inpfld_->currentItem();
    return !uom || (setup_.withnone_ && selidx == 0) ? nullptr
						     : uom->getLabel();
}


void uiUnitSel::displayGroup( bool yn )
{
    if ( inplbl_ )
	inplbl_->display( yn );
    inpfld_->display( yn );
    if ( propfld_ )
	propfld_->display( yn );
    if ( mnfld_ )
	mnfld_->display( yn );
}


void uiUnitSel::setPropType( Mnemonic::StdType typ )
{
    setPropFld( typ );
    usePar( lastUsed() );
}


const Mnemonic* uiUnitSel::mnemonic() const
{
    return setup_.mn_;
}


void uiUnitSel::setMnemonic( const Mnemonic& mn )
{
    setMnemFld( &mn );
    setUnFld( mn.unit() );
}


const char* uiUnitSel::tblKey() const
{
    if ( setup_.ptype_ != Mnemonic::Other )
	return Mnemonic::StdTypeNames()[setup_.ptype_];
    return tblkey_;
}


void uiUnitSel::fillPar( IOPar& iop, const char* altkey ) const
{
    prevuom_ = gtUnit();
    iop.update( altkey ? altkey : tblKey(), prevuom_ ? prevuom_->getLabel()
						     : nullptr );
}


bool uiUnitSel::usePar( const IOPar& iop, const char* altkey )
{
    const BufferString res = iop.find( altkey ? altkey : tblKey() );
    if ( !res.isEmpty() )
    {
	const UnitOfMeasure* un = UoMR().get( res );
	if ( setup_.ptype_ == Mnemonic::Other
		|| (un && un->propType() == setup_.ptype_) )
	{
	    setUnFld( un );
	    return true;
	}
    }
    return false;
}


uiString uiUnitSel::getSelTxt( const UnitOfMeasure* un ) const
{
    if ( !un )
	return sDispNone;
    else if ( setup_.mode_ == Setup::SymbolsOnly )
    {
	const StringView symb( un->symbol() );
	return symb.isEmpty() ? sDispNone : toUiString(symb.str());
    }
    else if ( setup_.mode_ == Setup::NamesOnly )
	return toUiString(un->name());

    mDeclStaticString( ret );
    ret.set( un->symbol() ).add( " (" ).add( un->name() ).add( ")" );
    return toUiString(ret);
}


template <class T> static T gtUserValue( const UnitOfMeasure* uom, T val )
{
    if ( mIsUdf(val) )
	return val;
    return uom ? uom->getUserValueFromSI( val ) : val;
}


template <class T> static T gtIntnValue( const UnitOfMeasure* uom, T val )
{
    if ( mIsUdf(val) )
	return val;
    return uom ? uom->getSIValue( val ) : val;
}


float uiUnitSel::getUserValue( float val ) const
{ return gtUserValue( getUnit(), val ); }
double uiUnitSel::getUserValue( double val ) const
{ return gtUserValue( getUnit(), val ); }
float uiUnitSel::getInternalValue( float val ) const
{ return gtIntnValue( getUnit(), val ); }
double uiUnitSel::getInternalValue( double val ) const
{ return gtIntnValue( getUnit(), val ); }
