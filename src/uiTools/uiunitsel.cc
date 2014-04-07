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
    usePar( lastUsed() );
}


void uiUnitSel::propSelChg( CallBacker* )
{
    update();
    propSelChange.trigger();
}


void uiUnitSel::setKey( const char* newky )
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


void uiUnitSel::setUnit( const UnitOfMeasure* un )
{
    if ( !un )
    {
	if ( !setup_.withnone_ )
	    un = UoMR().getInternalFor( setup_.ptype_ );
	else
	    { inpfld_->setCurrentItem( 0 ); return; }
    }

    if ( un )
	setUnit( un->name() );
}


void uiUnitSel::setUnit( const char* unitnm )
{
    if ( !unitnm || !*unitnm )
	{ const UnitOfMeasure* un = 0; setUnit( un ); }
    else if ( inpfld_->isPresent(unitnm) )
	inpfld_->setCurrentItem( unitnm );
    else
    {
	for ( int idx=0; idx<units_.size(); idx++ )
	{
	    const UnitOfMeasure& un = *units_[idx];
	    const BufferString unnm( un.name() );
	    const BufferString unsymb( un.symbol() );
	    if ( unnm == unitnm || unsymb == unitnm )
		{
		    inpfld_->setCurrentItem( setup_.withnone_ ? idx+1 : idx );
		    break;
		}
	}
    }
    fillPar( lastUsed() );
}


const UnitOfMeasure* uiUnitSel::getUnit() const
{
    fillPar( lastUsed() );
    return gtUnit();
}


const UnitOfMeasure* uiUnitSel::gtUnit() const
{
    int selidx = inpfld_->currentItem();
    if ( setup_.withnone_ )
	selidx -= 1;
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
    if ( typ == setup_.ptype_ )
	return;

    setup_.ptype_ = typ;
    if ( propfld_ )
    {
	NotifyStopper nst( propfld_->selectionChanged );
	propfld_->setCurrentItem( (int)typ );
    }
    update();
    usePar( lastUsed() );
}


void uiUnitSel::fillPar( IOPar& iop, const char* altkey ) const
{
    const UnitOfMeasure* un = gtUnit();
    iop.update( altkey ? altkey : tblkey_.buf(), un ? un->name().buf() : 0 );
}


bool uiUnitSel::usePar( const IOPar& iop, const char* altkey )
{
    const char* res = iop.find( altkey ? altkey : tblkey_.buf() );
    if ( res && *res )
    {
	setUnit( res );
	return true;
    }
    return false;
}


const char* uiUnitSel::getSelTxt( const UnitOfMeasure* un ) const
{
    if ( !un )
	return "";
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
    const BufferString olddef( inpfld_->isEmpty() ? "" : inpfld_->text() );
    inpfld_->setEmpty();
    if ( propfld_ )
	setup_.ptype_ = (PropertyRef::StdType)propfld_->currentItem();

    if ( setup_.withnone_ )
	inpfld_->addItem( "-" );
    units_.erase();
    UoMR().getRelevant( setup_.ptype_, units_ );
    for ( int idx=0; idx<units_.size(); idx++ )
	inpfld_->addItem( getSelTxt(units_[idx]) );

    if ( !olddef.isEmpty() && inpfld_->isPresent(olddef) )
	inpfld_->setText( olddef );
    else if ( setup_.ptype_ == PropertyRef::Dist )
	inpfld_->setText( getSelTxt(UnitOfMeasure::surveyDefDepthUnit()) );
}
