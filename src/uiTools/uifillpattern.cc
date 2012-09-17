/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          start of 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uifillpattern.cc,v 1.1 2012-09-17 14:40:56 cvsbert Exp $";

#include "uifillpattern.h"
#include "uicombobox.h"
#include "uigraphicsview.h"


uiFillPattern::uiFillPattern( uiParent* p )
	: uiGroup(p)
	, patternChanged(this)
{
    BufferStringSet nms; FillPattern::getTypeNames( nms );
    typefld_ = new uiComboBox( this, nms, "Type" );
    typefld_->setHSzPol( uiObject::Small );
    typefld_->selectionChanged.notify( mCB(this,uiFillPattern,selChg) );

    optfld_ = new uiComboBox( this, "Option" );
    optfld_->attach( rightOf, typefld_ );
    setHAlignObj( optfld_ );

    gv_ = new uiGraphicsView( this, "Pattern" );
    gv_->attach( rightOf, optfld_ );
    gv_->setPrefWidthInChar( 10 );
    gv_->setPrefHeightInChar( 1 );
}


void uiFillPattern::setOptNms()
{
    BufferStringSet nms;
    FillPattern::getOptNames( typefld_->currentItem(), nms );
    optfld_->setEmpty(); optfld_->addItems( nms );
}


void uiFillPattern::selChg( CallBacker* cb )
{
    if ( cb == typefld_ )
	setOptNms();

    reDrawPattern();
    patternChanged.trigger();
}


void uiFillPattern::reDrawPattern()
{
    //TODO
}


FillPattern uiFillPattern::get() const
{
    int typ = typefld_->currentItem(); if ( typ < 0 ) typ = 0;
    int opt = optfld_->currentItem(); if ( opt < 0 || typ == 0 ) opt = 0;
    return FillPattern( typ, opt );
}


void uiFillPattern::set( const FillPattern& fp )
{
    NotifyStopper ns( patternChanged );
    typefld_->setCurrentItem( fp.type_ );
    setOptNms();
    optfld_->setCurrentItem( fp.opt_ );
    reDrawPattern();
    patternChanged.trigger();
}
