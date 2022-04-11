/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
________________________________________________________________________

-*/


#include "uiunitsel.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include "ioman.h"

static const char* sDispNone = "-";

uiUnitSel::uiUnitSel( uiParent* p, const uiUnitSel::Setup& su )
    : uiGroup(p,"UnitSel")
    , setup_(su)
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::uiUnitSel( uiParent* p, PropertyRef::StdType st )
    : uiGroup(p,"UnitSel")
    , setup_(st)
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::uiUnitSel( uiParent* p, Mnemonic* mn )
    : uiGroup(p,"UnitSel")
    , setup_(mn ? mn->stdType() : PropertyRef::Dist, toUiString(""), mn )
    , selChange(this)
    , propSelChange(this)
{
    init();
}



uiUnitSel::uiUnitSel( uiParent* p, const char* lbltxt )
    : uiGroup(p,"UnitSel")
    , setup_(SI().zIsTime() ? PropertyRef::Time : PropertyRef::Dist,
	     mToUiStringTodo(lbltxt))
    , selChange(this)
    , propSelChange(this)
{
    init();
}


uiUnitSel::~uiUnitSel()
{}


void uiUnitSel::init()
{
    units_.allowNull( true );
    tblkey_ = setup_.lbltxt_.getFullString();
    if ( tblkey_.isEmpty() )
	tblkey_ = PropertyRef::StdTypeNames()[setup_.ptype_];

    propfld_ = 0;
    if ( setup_.selproptype_ )
    {
	propfld_ = new uiComboBox( this, "Properties" );
	const BufferStringSet typnms( PropertyRef::StdTypeNames() );
	propfld_->addItems( typnms );
	propfld_->setCurrentItem( (int)setup_.ptype_ );
	propfld_->selectionChanged.notify( mCB(this,uiUnitSel,propSelChg) );
    }

    mnfld_ = nullptr;
    if ( setup_.selmnemtype_ )
    {
	mnfld_ = new uiComboBox( this, "Mnemonic" );
	mAttachCB( mnfld_->selectionChanged, uiUnitSel::mnSelChg );
	const MnemonicSet& mns = MNC();
	BufferStringSet mnsnames;
	uiStringSet tooltips;
	for ( int idx=0; idx<mns.size(); idx++ )
	{
	    const Mnemonic& mn = *mns[idx];
	    mnsnames.add( mn.name() );
	    tooltips.add( toUiString(mn.logTypeName()) );
	}

	ArrPtrMan<int> sortindexes = mnsnames.getSortIndexes();
	mnsnames.useIndexes( sortindexes );
	tooltips.useIndexes( sortindexes );
	mnfld_->addItems( mnsnames );
	mnfld_->setToolTips( tooltips );
	if ( setup_.mn_ )
	{
	    mnfld_->setCurrentItem( setup_.mn_->name() );
	    if ( propfld_ )
		mnfld_->attach( rightOf, propfld_ );
	}
    }

    inpfld_ = new uiComboBox( this, "Units" );
    if ( setup_.mode_ == Setup::SymbolsOnly )
	inpfld_->setHSzPol( uiObject::Small );

    inpfld_->selectionChanged.notify( mCB(this,uiUnitSel,selChg) );
    if ( mnfld_ )
	inpfld_->attach( rightOf, mnfld_ );
    else if ( propfld_ )
	inpfld_->attach( rightOf, propfld_ );

    uiComboBox* leftcb = propfld_ ? propfld_ : inpfld_;
    if ( setup_.lbltxt_.isEmpty() )
	setHAlignObj( inpfld_ );
    else
    {
	uiLabel* lbl = new uiLabel( this, setup_.lbltxt_ );
	lbl->attach( leftOf, leftcb );
	setHAlignObj( leftcb );
    }

    update();
    usePar( lastUsed() );
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


void uiUnitSel::setPropFld( PropertyRef::StdType typ )
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


void uiUnitSel::setMnemFld( Mnemonic* mn )
{
    if ( mnfld_ )
    {
	NotifyStopper nst( mnfld_->selectionChanged );
	mnfld_->setCurrentItem( mn->name() );
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


void uiUnitSel::setUnit( const char* unitnm )
{
    const FixedString unnm( unitnm );
    const UnitOfMeasure* un = 0;

    if ( !unnm.isEmpty() && unnm != "-" )
	un = UoMR().get( unitnm );
    setUnit( un );
}


const UnitOfMeasure* uiUnitSel::getUnit() const
{
    fillPar( lastUsed() );
    return gtUnit();
}


const UnitOfMeasure* uiUnitSel::gtUnit() const
{
    int selidx = inpfld_->currentItem();
    return units_.validIdx( selidx ) ? units_[selidx] : 0;
}


const char* uiUnitSel::getUnitName() const
{
    fillPar( lastUsed() );

    int selidx = inpfld_->currentItem();
    if ( selidx < 0 || (setup_.withnone_ && selidx == 0) )
	return 0;

    return inpfld_->text();
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


void uiUnitSel::setPropType( PropertyRef::StdType typ )
{
    setPropFld( typ );
    usePar( lastUsed() );
}


Mnemonic* uiUnitSel::mnemonic() const
{
    return setup_.mn_;
}


void uiUnitSel::setMnemonic( Mnemonic& mn )
{
    setMnemFld( &mn );
    const char* res = mn.disp_.unit_;
    const UnitOfMeasure* un = UoMR().get( res );
    setUnFld( un );
}


const char* uiUnitSel::tblKey() const
{
    if ( setup_.ptype_ != PropertyRef::Other )
	return PropertyRef::StdTypeNames()[setup_.ptype_];
    return tblkey_;
}


void uiUnitSel::fillPar( IOPar& iop, const char* altkey ) const
{
    const UnitOfMeasure* un = gtUnit();
    iop.update( altkey ? altkey : tblKey(), un ? un->name().buf() : 0 );
}


bool uiUnitSel::usePar( const IOPar& iop, const char* altkey )
{
    const char* res = iop.find( altkey ? altkey : tblKey() );
    if ( res && *res )
    {
	const UnitOfMeasure* un = UoMR().get( res );
	if ( setup_.ptype_ == PropertyRef::Other
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
	return mToUiStringTodo(sDispNone);
    else if ( setup_.mode_ == Setup::SymbolsOnly )
	return mToUiStringTodo(un->symbol());
    else if ( setup_.mode_ == Setup::NamesOnly )
	return mToUiStringTodo(un->name().buf());

    mDeclStaticString( ret );
    ret.set( un->symbol() ).add( " (" ).add( un->name() ).add( ")" );
    return mToUiStringTodo(ret);
}


void uiUnitSel::update()
{
    const BufferString olddef( !inpfld_->isEmpty() ? inpfld_->text()
				    : (setup_.withnone_ ? sDispNone : "") );
    inpfld_->setEmpty();
    if ( propfld_ )
	setup_.ptype_ = (PropertyRef::StdType)propfld_->currentItem();

    if ( mnfld_ )
	setup_.mn_ = eMNC().find( mnfld_->text() );

    units_.erase();
    if ( setup_.mn_ )
	UoMR().getRelevant( setup_.mn_->stdType(), units_ );
    else
	UoMR().getRelevant( setup_.ptype_, units_ );

    if ( setup_.withnone_ )
	units_.insertAt( 0, 0 );

    for ( int idx=0; idx<units_.size(); idx++ )
	inpfld_->addItem( getSelTxt(units_[idx]) );

    if ( !olddef.isEmpty() && inpfld_->isPresent(olddef) )
	inpfld_->setText( olddef );
    else if ( setup_.ptype_ == PropertyRef::Dist )
	inpfld_->setText( mFromUiStringTodo(getSelTxt(
					UnitOfMeasure::surveyDefDepthUnit())) );
}
