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


uiUnitSel::uiUnitSel( uiParent* p, PropertyRef::StdType typ, const char* txt,
		      bool symb, bool withempty )
    : uiGroup(p,"UnitSel")
    , proptype_(typ)
    , symbolsdisp_(symb)
    , tblkey_(txt)
    , selChange(this)
    , withempty_(withempty)
{
    if ( tblkey_.isEmpty() )
	tblkey_ = PropertyRef::StdTypeNames()[proptype_];

    inpfld_ = new uiComboBox( this, "Units" );
    if ( symbolsdisp_ )
	inpfld_->setHSzPol( uiObject::Small );
    inpfld_->selectionChanged.notify( mCB(this,uiUnitSel,selChg) );

    if ( txt && *txt )
    {
	uiLabel* lbl = new uiLabel( this, txt );
	inpfld_->attach( rightOf, lbl );
	setHAlignObj( inpfld_ );
    }

    update();
    usePar( lastUsed() );
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
	if ( !withempty_ )
	    un = UoMR().getInternalFor( proptype_ );
	else
	{
	    inpfld_->setCurrentItem( 0 );
	    return;
	}
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
		{ inpfld_->setCurrentItem( withempty_ ? idx+1 : idx ); break; }
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
    if ( withempty_ ) selidx -= 1;
    return units_.validIdx( selidx ) ? units_[selidx] : 0;
}


const char* uiUnitSel::getUnitName() const
{
    fillPar( lastUsed() );

    int selidx = inpfld_->currentItem();
    if ( selidx < 0 || (withempty_ && selidx == 0) )
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
    if ( typ == proptype_ )
	return;

    proptype_ = typ;
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


void uiUnitSel::update()
{
    const BufferString olddef( inpfld_->isEmpty() ? "" : inpfld_->text() );
    inpfld_->setEmpty();

    if ( withempty_ )
	inpfld_->addItem( "-" );
    units_.erase();
    UoMR().getRelevant( proptype_, units_ );
    for ( int idx=0; idx<units_.size(); idx++ )
    {
	const char* disp = symbolsdisp_ ? units_[idx]->symbol()
					: units_[idx]->name().buf();
	inpfld_->addItem( disp );
    }

    if ( !olddef.isEmpty() && inpfld_->isPresent(olddef) )
	inpfld_->setText( olddef );
    else if ( proptype_ == PropertyRef::Dist && SI().depthsInFeet() )
	inpfld_->setText( symbolsdisp_ ? "ft" : "Feet" );
}
