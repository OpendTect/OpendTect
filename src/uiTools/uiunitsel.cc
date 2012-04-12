/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiunitsel.cc,v 1.6 2012-04-12 14:46:54 cvsbruno Exp $";

#include "uiunitsel.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "unitofmeasure.h"


uiUnitSel::uiUnitSel( uiParent* p, PropertyRef::StdType typ, const char* txt,
       		      bool symb, bool withempty )
    : uiGroup(p,"UnitSel")
    , proptype_(typ)
    , symbolsdisp_(symb)
    , selChange(this)
    , withempty_(withempty)		     
{
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
}


void uiUnitSel::setUnit( const char* unitnm )
{
    if ( !unitnm || !*unitnm )
    {
	const UnitOfMeasure* un = UoMR().getInternalFor( proptype_ );
	if ( un )
	    setUnit( un->name() );
	return;
    }

    if ( inpfld_->isPresent(unitnm) )
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
}


const UnitOfMeasure* uiUnitSel::getUnit() const
{
    int selidx = inpfld_->currentItem();
    if ( withempty_ ) selidx -= 1;
    return units_.validIdx( selidx ) ? units_[selidx] : 0;
}


void uiUnitSel::setPropType( PropertyRef::StdType typ )
{
    if ( typ == proptype_ )
	return;

    proptype_ = typ;
    update();
}


void uiUnitSel::update()
{
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
}
