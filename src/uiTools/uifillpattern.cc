/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          start of 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uifillpattern.h"
#include "uicombobox.h"
#include "uigraphicsview.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"


uiFillPattern::uiFillPattern( uiParent* p )
	: uiGroup(p)
	, patternChanged(this)
{
    const CallBack selchgcb( mCB(this,uiFillPattern,selChg) );
    BufferStringSet nms; FillPattern::getTypeNames( nms );
    typefld_ = new uiComboBox( this, nms, "Type" );
    typefld_->setHSzPol( uiObject::Small );
    typefld_->selectionChanged.notify( selchgcb );

    optfld_ = new uiComboBox( this, "Option" );
    optfld_->attach( rightOf, typefld_ );
    optfld_->selectionChanged.notify( selchgcb );

    uiGraphicsView* gv = new uiGraphicsView( this, "Pattern" );
    gv->attach( rightOf, optfld_ );
    gv->setPrefWidthInChar( 10 );
    gv->setPrefHeightInChar( 1 );
    gv->setStretch( 1, 0 );
    patrect_ = gv->scene().addRect( 0, 0, 200, 100 );

    setHAlignObj( optfld_ );
    reDrawPattern();
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
    patrect_->setFillPattern( get() );
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
