/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uiarrowdlg.h"

#include "uicombobox.h"
#include "uisellinest.h"
#include "uislider.h"
#include "draw.h"


static const char* arrowtypes[] = { "Top", "Bottom", "Both", 0 };


uiArrowDialog::uiArrowDialog( uiParent* p )
    : uiDialog(p,Setup("Arrow properties",mNoDlgTitle,"50.0.14"))
    , propertyChange(this)
{
    setCancelText(0);
    typefld_ = new uiLabeledComboBox( this, "Type" );
    typefld_->box()->addItems( arrowtypes );
    typefld_->box()->selectionChanged.notify(
				mCB(this,uiArrowDialog,changeCB) );

    LineStyle ls; uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    linestylefld_ = new uiSelLineStyle( this, ls, lssu );
    linestylefld_->changed.notify( mCB(this,uiArrowDialog,changeCB) );
    linestylefld_->attach( alignedBelow, typefld_ );

    scalefld_ = new uiSliderExtra( this,
	    		uiSliderExtra::Setup("Scale").nrdec(1).logscale(true),
	   		"Size" );
    scalefld_->sldr()->setMinValue( 0.1 );
    scalefld_->sldr()->setMaxValue( 10 );
    scalefld_->sldr()->setValue( 1 );
    scalefld_->sldr()->valueChanged.notify( mCB(this,uiArrowDialog,changeCB) );
    scalefld_->attach( alignedBelow, linestylefld_ );
}


void uiArrowDialog::setColor( const Color& col )
{
    linestylefld_->setColor( col );
}


const Color& uiArrowDialog::getColor() const
{
    return linestylefld_->getColor();
}


void uiArrowDialog::setLineWidth( int ni )
{
    linestylefld_->setWidth( ni );
}


int uiArrowDialog::getLineWidth() const
{
    return linestylefld_->getWidth();
}


void uiArrowDialog::setArrowType( int type )
{
    typefld_->box()->setValue( type );
}


int uiArrowDialog::getArrowType() const
{
    return typefld_->box()->getIntValue();
}


void uiArrowDialog::setScale( float scale )
{
}


float uiArrowDialog::getScale() const
{
    return scalefld_->sldr()->getValue();
}


void uiArrowDialog::changeCB( CallBacker* )
{
    propertyChange.trigger();
}
