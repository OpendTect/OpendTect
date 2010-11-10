/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiunitsel.cc,v 1.2 2010-11-10 15:26:43 cvsbert Exp $";

#include "uiunitsel.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "unitofmeasure.h"


uiUnitSel::uiUnitSel( uiParent* p, PropertyRef::StdType typ, const char* txt )
    : uiGroup(p,"UnitSel")
    , proptype_(typ)
{
    inpfld_ = new uiComboBox( this, "Units" );
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
    inpfld_->setCurrentItem( unitnm );
}


const UnitOfMeasure* uiUnitSel::getUnit() const
{
    const int selidx = inpfld_->currentItem();
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
    units_.erase();
    UoMR().getRelevant( proptype_, units_ );
    for ( int idx=0; idx<units_.size(); idx++ )
	inpfld_->addItem( units_[idx]->name() );
}
