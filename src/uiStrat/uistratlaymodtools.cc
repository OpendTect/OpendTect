/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlaymodtools.cc,v 1.9 2012/09/11 12:50:53 cvsbert Exp $";

#include "uistratlaymodtools.h"
#include "uitoolbutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "stratlevel.h"


uiStratGenDescTools::uiStratGenDescTools( uiParent* p )
    : uiGroup(p,"Gen Desc Tools")
    , openReq(this)
    , saveReq(this)
    , propEdReq(this)
    , genReq(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiToolButton* opentb = new uiToolButton( leftgrp, "open.png",
				"Open stored generation description",
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save.png",
	    			"Save generation description",
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgrp, "defprops.png",
	    			"Select layer properties",
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    nrmodlsfld_ = new uiGenInput( rightgrp, "", IntInpSpec(25) );
    nrmodlsfld_->setElemSzPol( uiObject::Small );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( "Number of models to generate", 0 );
    nrmodlsfld_->updateRequested.notify( gocb );
    uiToolButton* gotb = new uiToolButton( rightgrp, "go.png",
	    			"Generate this amount of models", gocb );
    nrmodlsfld_->attach( leftOf, gotb );
    rightgrp->attach( ensureRightOf, leftgrp );
    rightgrp->setFrame( true );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}


uiStratLayModEditTools::uiStratLayModEditTools( uiParent* p )
    : uiGroup(p,"Lay Mod Edit Tools")
    , selPropChg(this)
    , selLevelChg(this)
    , selContentChg(this)
    , dispEachChg(this)
    , dispZoomedChg(this)
    , dispLithChg(this)
    , flattenChg(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    propfld_ = new uiComboBox( leftgrp, "Display property" );
    propfld_->setToolTip( "Displayed property" );
    propfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selPropCB) );

    uiLabel* eachlbl = new uiLabel( leftgrp, "each" );
    eachlbl->attach( rightOf, propfld_ );
    eachfld_ = new uiSpinBox( leftgrp, 0, "DispEach" );
    eachfld_->setInterval( 1, 1000 );
    eachfld_->attach( rightOf, eachlbl );
    eachfld_->valueChanging.notify(
				mCB(this,uiStratLayModEditTools,dispEachCB) );

    lvlfld_ = new uiComboBox( leftgrp, "Level" );
    lvlfld_->setToolTip( "Selected stratigraphic level" );
    lvlfld_->attach( rightOf, eachfld_ );
    lvlfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selLevelCB) );

    contfld_ = new uiComboBox( leftgrp, "Content" );
    contfld_->setToolTip( "Marked content" );
    contfld_->attach( rightOf, lvlfld_ );
    contfld_->setHSzPol( uiObject::Small );
    contfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selContentCB) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    flattenedtb_ = new uiToolButton( rightgrp, "flattenseis",
			"Show flattened when on",
			mCB(this,uiStratLayModEditTools,showFlatCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );
    lithtb_ = new uiToolButton( rightgrp, "lithologies",
			"Show lithology colors when on",
			mCB(this,uiStratLayModEditTools,dispLithCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, flattenedtb_ );
    zoomtb_ = new uiToolButton( rightgrp, "toggzooming",
			"Do not zoom into models when on",
			mCB(this,uiStratLayModEditTools,dispZoomedCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( true );
    zoomtb_->attach( leftOf, lithtb_ );
    rightgrp->attach( rightTo, leftgrp );
    rightgrp->attach( rightBorder );
}


static void setFldNms( uiComboBox* cb, const BufferStringSet& nms, bool wnone,
		       int def )
{
    const BufferString selnm( cb->text() );
    cb->setEmpty();
    if ( wnone )
	cb->addItem( "---" );
    if ( nms.isEmpty() ) return;

    cb->addItems( nms );
    if ( !selnm.isEmpty() ) 
	def = nms.indexOf( selnm );
    if ( wnone ) def++;
    if ( def > cb->size() ) def = cb->size() - 1;
    cb->setCurrentItem( def );
}


void uiStratLayModEditTools::setProps( const BufferStringSet& nms )
{ setFldNms( propfld_, nms, false, 0 ); }
void uiStratLayModEditTools::setLevelNames( const BufferStringSet& nms )
{ setFldNms( lvlfld_, nms, true, 0 ); }
void uiStratLayModEditTools::setContentNames( const BufferStringSet& nms )
{ setFldNms( contfld_, nms, true, -1 ); }


const char* uiStratLayModEditTools::selProp() const
{
    return propfld_->isEmpty() ? 0 : propfld_->text();
}


int uiStratLayModEditTools::selPropIdx() const
{
    return propfld_->isEmpty() ? 0 : propfld_->getIntValue() + 1;
}


const char* uiStratLayModEditTools::selLevel() const
{
    return lvlfld_->isEmpty() ? 0 : lvlfld_->text();
}


int uiStratLayModEditTools::selLevelIdx() const
{
    return lvlfld_->isEmpty() ? -1 : lvlfld_->currentItem() - 1;
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? 0 : contfld_->text();
}


const Strat::Level* uiStratLayModEditTools::selStratLevel() const
{
    const int lvlidx = selLevelIdx();
    return lvlidx < 0 ? 0 : Strat::LVLS().levels()[lvlidx];
}


Color uiStratLayModEditTools::selLevelColor() const
{
    const Strat::Level* lvl = selStratLevel();
    return lvl ? lvl->color() : Color::NoColor();
}


int uiStratLayModEditTools::dispEach() const
{
    return eachfld_->getValue();
}


bool uiStratLayModEditTools::dispZoomed() const
{
    return !zoomtb_->isOn();
}


bool uiStratLayModEditTools::dispLith() const
{
    return lithtb_->isOn();
}


bool uiStratLayModEditTools::showFlattened() const
{
    return flattenedtb_->isOn();
}


void uiStratLayModEditTools::setSelProp( const char* sel )
{
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelLevel( const char* sel )
{
    lvlfld_->setText( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel )
{
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setDispEach( int nr )
{
    eachfld_->setValue( nr );
}


void uiStratLayModEditTools::setDispZoomed( bool yn )
{
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn )
{
    lithtb_->setOn( yn );
}


void uiStratLayModEditTools::setShowFlattened( bool yn )
{
    flattenedtb_->setOn( yn );
}
