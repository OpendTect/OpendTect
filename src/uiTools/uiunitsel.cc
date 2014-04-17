/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

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


uiUnitSel::uiUnitSel( uiParent* p, const char* lbltxt )
    : uiGroup(p,"UnitSel")
    , setup_(SI().zIsTime() ? PropertyRef::Time : PropertyRef::Dist,lbltxt)
    , selChange(this)
    , propSelChange(this)
{
    init();
}


void uiUnitSel::init()
{
    units_.allowNull( true );
    tblkey_ = setup_.lbltxt_;
    if ( tblkey_.isEmpty() )
	tblkey_ = PropertyRef::StdTypeNames()[setup_.ptype_];

    propfld_ = 0;
    if ( setup_.selproptype_ )
    {
	propfld_ = new uiComboBox( this, "Properties" );
	propfld_->addItems( PropertyRef::StdTypeNames() );
	propfld_->setCurrentItem( (int)setup_.ptype_ );
	propfld_->selectionChanged.notify( mCB(this,uiUnitSel,propSelChg) );
    }

    inpfld_ = new uiComboBox( this, "Units" );
    if ( setup_.mode_ == Setup::SymbolsOnly )
	inpfld_->setHSzPol( uiObject::Small );
    inpfld_->selectionChanged.notify( mCB(this,uiUnitSel,selChg) );
    if ( propfld_ )
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
}


void uiUnitSel::propSelChg( CallBacker* )
{
    update();
    propSelChange.trigger();
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
	setPropFld( un->propType() );
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


const char* uiUnitSel::getSelTxt( const UnitOfMeasure* un ) const
{
    if ( !un )
	return sDispNone;
    else if ( setup_.mode_ == Setup::SymbolsOnly )
	return un->symbol();
    else if ( setup_.mode_ == Setup::NamesOnly )
	return un->name();

    mDeclStaticString( ret );
    ret.set( un->symbol() ).add( " (" ).add( un->name() ).add( ")" );
    return ret;
}


void uiUnitSel::update()
{
    const BufferString olddef( !inpfld_->isEmpty() ? inpfld_->text()
				    : (setup_.withnone_ ? sDispNone : "") );
    inpfld_->setEmpty();
    if ( propfld_ )
	setup_.ptype_ = (PropertyRef::StdType)propfld_->currentItem();

    units_.erase();
    UoMR().getRelevant( setup_.ptype_, units_ );
    if ( setup_.withnone_ )
	units_.insertAt( 0, 0 );

    for ( int idx=0; idx<units_.size(); idx++ )
	inpfld_->addItem( getSelTxt(units_[idx]) );

    if ( !olddef.isEmpty() && inpfld_->isPresent(olddef) )
	inpfld_->setText( olddef );
    else if ( setup_.ptype_ == PropertyRef::Dist )
	inpfld_->setText( getSelTxt(UnitOfMeasure::surveyDefDepthUnit()) );
}
