/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2006
________________________________________________________________________

-*/

#include "uiselectvelocityfunction.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "velocityfunction.h"
#include "od_helpids.h"


namespace Vel
{

mImplFactory2Param(uiFunctionSettings, uiParent*,
		   FunctionSource*, uiFunctionSettings::factory );

uiFunctionSel::uiFunctionSel( uiParent* p,
	const ObjectSet<FunctionSource>& srcs,
	const TypeSet<Color>* colors )
    : uiGroup( p, "Velocity Functions" )
    , velsources_( srcs )
    , colorfld_( 0 )
    , listChange( this )
{
    deepRef( velsources_ );
    list_ = new uiListBox( this );
    list_->selectionChanged.notify(
	    mCB(this,uiFunctionSel,selChangedCB) );

    addbutton_ = new uiPushButton( this, uiStrings::sAdd(),
	    mCB( this, uiFunctionSel, addPushedCB ), false );
    addbutton_->attach( rightOf, list_ );

    removebutton_ = new uiPushButton( this, uiStrings::sRemove(),
	    mCB( this, uiFunctionSel, removePushedCB ), true );
    removebutton_->attach( alignedBelow, addbutton_ );

    propbutton_ = new uiPushButton( this, uiStrings::sProperties(),
	    mCB( this, uiFunctionSel, propPushedCB ), false );
    propbutton_->attach( alignedBelow, removebutton_ );


    if ( colors )
    {
	colors_ = *colors;
	colorfld_ = new uiColorInput( this, Color::Black(), "Color" );
	colorfld_->attach( alignedBelow, list_ );
	colorfld_->colorChanged.notify(
		mCB( this, uiFunctionSel, colorChanged ));
    }

    updateList();
    selChangedCB( 0 );
}


uiFunctionSel::~uiFunctionSel()
{
    deepUnRef( velsources_ );
}


ObjectSet<FunctionSource>& uiFunctionSel::getVelSources()
{ return velsources_; }


void uiFunctionSel::updateList()
{
    const int curitem = list_->currentItem();
    const bool wassel = list_->isChosen(curitem);
    for ( int idx=0; idx<velsources_.size(); idx++ )
    {
	if ( idx<list_->size() )
	    list_->setItemText(idx,toUiString(velsources_[idx]->userName()) );
	else
	    list_->addItem( toUiString(velsources_[idx]->userName()) );
    }

    while ( list_->size()>velsources_.size() )
	list_->removeItem( velsources_.size() );

    if ( curitem<list_->size() )
    {
	list_->setCurrentItem( curitem );
	list_->setChosen( curitem, wassel );
    }
}


void uiFunctionSel::selChangedCB(CallBacker*)
{
    const int curitem = list_->currentItem();
    const int sel = curitem>=0 && list_->isChosen( curitem ) ? curitem : -1;

    removebutton_->setSensitive( sel>=0 );
    propbutton_->setSensitive( sel>=0 );
    if ( colorfld_ )
    {
	colorfld_->setSensitive( sel>=0 );
	if ( sel>=0 )
	    colorfld_->setColor( colors_[sel] );
    }
}


void uiFunctionSel::addPushedCB(CallBacker*)
{
    uiAddFunction dlg( this );
    if ( !dlg.go() )
	return;

    FunctionSource* src = dlg.getSource();

    if ( !src ) return;
    src->ref();
    velsources_ += src;
    updateList();

    listChange.trigger();
}


void uiFunctionSel::removePushedCB(CallBacker*)
{
    const int curitem = list_->currentItem();
    const int sel = curitem>=0 && list_->isChosen( curitem ) ? curitem : -1;

    if ( sel<0 ) return;

    velsources_.removeSingle(sel)->unRef();
    if ( colorfld_ ) colors_.removeSingle( sel );
    updateList();
    listChange.trigger();
}


void uiFunctionSel::propPushedCB(CallBacker*)
{
    const int curitem = list_->currentItem();
    const int sel = curitem>=0 && list_->isChosen( curitem ) ? curitem : -1;
    if ( sel<0 ) return;

    uiEditFunction dlg( this, velsources_[sel] );
    if ( !dlg.isOK() || !dlg.go() )
	return;

    updateList();
}


void uiFunctionSel::colorChanged(CallBacker*)
{
    const int curitem = list_->currentItem();
    const int sel = curitem>=0 && list_->isChosen( curitem ) ? curitem : -1;

    if ( sel<0 ) return;

    colors_[sel] = colorfld_->color();
}


uiAddFunction::uiAddFunction( uiParent* p )
    : uiDialog(p, uiDialog::Setup(uiStrings::phrAdd(
				  tr("velocity function source")),
				  mNoDlgTitle,	mODHelpKey(mAddFunctionHelpID)))
    , typesel_( 0 )
{
    const BufferStringSet& sourceclasses =
	uiFunctionSettings::factory().getNames();
    const uiStringSet& sourceusernames =
	uiFunctionSettings::factory().getUserNames();
    typesel_ = new uiGenInput( this, uiStrings::sType(),
                               StringListInpSpec(sourceusernames));
    typesel_->valuechanged.notify(
			    mCB(this,uiAddFunction,typeSelChangeCB));

    for ( int idx=0; idx<sourceclasses.size(); idx++ )
    {
	const char* nm = sourceclasses[idx]->buf();
	uiFunctionSettings* setgrp =
	    uiFunctionSettings::factory().create( nm, this, 0 );

	setgrp->attach( alignedBelow, typesel_ );

	settingldgs_ += setgrp;
    }

    typeSelChangeCB( 0 );
}


FunctionSource* uiAddFunction::getSource()
{
    const int sel = typesel_->getIntValue();
    return settingldgs_[sel]->getSource();
}



void uiAddFunction::typeSelChangeCB(CallBacker*)
{
    const int sel = typesel_->getIntValue();
    for ( int idx=0; idx<settingldgs_.size(); idx++ )
	settingldgs_[idx]->display( sel==idx );
}


bool uiAddFunction::acceptOK(CallBacker*)
{
    const int sel = typesel_->getIntValue();
    return settingldgs_[sel]->acceptOK();
}


uiEditFunction::uiEditFunction( uiParent* p, FunctionSource* vfs )
    : uiDialog( p, uiDialog::Setup(uiStrings::phrEdit(tr("velocity functions")),
			     mNoDlgTitle, mODHelpKey(mEditFunctionHelpID)) )
    , dlggrp_( 0 )
{
    dlggrp_ = uiFunctionSettings::factory().create( 0, this, vfs, false );
}


bool uiEditFunction::acceptOK(CallBacker*)
{
    return dlggrp_ ? dlggrp_->acceptOK() : true;
}


}; //namespace
