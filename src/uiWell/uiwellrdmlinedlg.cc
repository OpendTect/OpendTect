/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          October 2005
 RCS:           $Id: uiwellrdmlinedlg.cc,v 1.3 2005-11-15 16:16:56 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiwellrdmlinedlg.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uimsg.h"
#include "pixmap.h"
#include "oddirs.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "welldata.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltransl.h"
#include "transl.h"
#include "uiioobjsel.h"
#include "uiwellpartserv.h"



uiWell2RandomLineDlg::uiWell2RandomLineDlg( uiParent* p, uiWellPartServer* ws )
    : uiDialog(p,uiDialog::Setup("Random line - Wells relations","",
				 "109.0.0").modal(false))
    , wellsbox_(0), selectedwellsbox_(0), wellserv_(ws)
{
    BufferString title( "Select wells to set up the random line path" );
    setTitleText( title );

    uiGroup* topgrp = new uiGroup( this, "selection group" );
    uiGroup* selbuttons = new uiGroup( topgrp, "select buttons" );
    uiGroup* movebuttons = new uiGroup( topgrp, "move buttons" );
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );
    createFields( topgrp );
    attachFields( selbuttons, topgrp, movebuttons );

    fillListBox();
}


void uiWell2RandomLineDlg::createFields( uiGroup* topgrp )
{
    wellsbox_ = new uiListBox( topgrp, "Available Wells", true );
    selectedwellsbox_ = new uiListBox( topgrp, "Selected Wells", true );
    
    onlytopfld_ = new uiGenInput( this, "use only wells' top position", 
				  BoolInpSpec( "Yes","No" ) );

    CallBack cb = mCB(this,uiWell2RandomLineDlg,previewPush);
    previewbutton_ = new uiPushButton( this, "Preview", cb );
}


void uiWell2RandomLineDlg::createSelectButtons( uiGroup* selbuttons )
{
    const ioPixmap pm0( GetIconFileName("rightarrow.png") );
    const ioPixmap pm1( GetIconFileName("leftarrow.png") );

    uiLabel* sellbl = new uiLabel( selbuttons, "Select" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,selButPush);
    toselect_ = new uiToolButton( selbuttons, "", pm0, cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::undef );
    fromselect_ = new uiToolButton( selbuttons, "", pm1, cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiWell2RandomLineDlg::createMoveButtons( uiGroup* movebuttons )
{
    const ioPixmap pm0( GetIconFileName("upwardarrow.png") );
    const ioPixmap pm1( GetIconFileName("downwardarrow.png") );

    uiLabel* movelbl = new uiLabel( movebuttons, "Change \n order" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, "", pm0, cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::undef );
    movedownward_ = new uiToolButton( movebuttons, "", pm1, cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::undef );
    movebuttons->setHAlignObj( moveupward_ );
}


void uiWell2RandomLineDlg::attachFields( uiGroup* selbuttons, uiGroup* topgrp,
					 uiGroup* movebuttons )
{
    selbuttons->attach( centeredLeftOf, selectedwellsbox_ );
    selbuttons->attach( ensureRightOf, wellsbox_ );
    selectedwellsbox_->attach( rightTo, wellsbox_ );
    movebuttons->attach( centeredRightOf, selectedwellsbox_ );
    onlytopfld_->attach( centeredBelow, topgrp );
    previewbutton_->attach( centeredBelow, onlytopfld_ );
}


void uiWell2RandomLineDlg::selButPush( CallBacker* cb )
{
    uiListBox* frombox;
    uiListBox* tobox;
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	frombox = wellsbox_;
	tobox = selectedwellsbox_;
    }
    else if ( but == fromselect_ )
    {
	frombox = selectedwellsbox_;
	tobox = wellsbox_;
    }

    int lastusedidx = 0;
    for ( int idx=0; idx<frombox->size(); idx++ )
    {
	if ( !frombox->isSelected(idx) ) continue;
	tobox->addItem( frombox->textOfItem(idx) );
	frombox->removeItem(idx);
	lastusedidx = idx;
	if ( tobox == wellsbox_ )
	    tobox->sort();
	idx--;
    }
    
    frombox->setSelected( lastusedidx<frombox->size() ? 
	    		  lastusedidx : frombox->size()-1 );
}


void uiWell2RandomLineDlg::fillListBox()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    IOM().to( MultiID(IOObjContext::getStdDirData(ctio->ctxt.stdseltype)->id) );
    IODirEntryList entrylist( IOM().dirPtr(), ctio->ctxt );

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	entrylist.setCurrent( idx );
	allwellsids_ += entrylist.selected()->key();
	allwellsnames_.add( entrylist[idx]->name() );
    }

    wellsbox_->addItems( allwellsnames_ );
}


void uiWell2RandomLineDlg::setSelectedWells()
{
    selwellsids_.erase();

    for ( int idx=0; idx<selectedwellsbox_->size(); idx++ )
    {
	const char* txt = selectedwellsbox_->textOfItem( idx );
	int wellidx = allwellsnames_.indexOf( txt );
	if ( wellidx<0 ) continue;
	selwellsids_.addIfNew( allwellsids_[wellidx] );
    }
}


void uiWell2RandomLineDlg::getCoordinates( TypeSet<Coord>& coords )
{
    for ( int idx=0; idx<selwellsids_.size(); idx++ )
    {
	Well::Data* wd;
	wd = Well::MGR().get( selwellsids_[idx] );
	if ( onlytopfld_->getBoolValue() )
	{
	    Coord3 coord3 = wd->track().pos(0);
	    coords += Coord( coord3.x, coord3.y );
	}
	else
	{
	    for ( int idx=0; idx<wd->track().size(); idx++ )
	    {
		Coord3 coord3 = wd->track().pos(idx);
		coords += Coord( coord3.x, coord3.y );
	    }
	}
    }
}


void uiWell2RandomLineDlg::previewPush( CallBacker* cb )
{
    setSelectedWells();
    wellserv_->sendPreviewEvent();
}


void uiWell2RandomLineDlg::moveButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( !selectedwellsbox_->isSelected(selectedwellsbox_->currentItem() ) ) 
	return;
    
    int index = selectedwellsbox_->currentItem();
    
    if ( but == moveupward_ && index>0 )
    {
	selectedwellsbox_->insertItem( selectedwellsbox_->getText(), index-1 );
	selectedwellsbox_->removeItem( index+1 );
	selectedwellsbox_->setSelected( index-1 );
	selectedwellsbox_->setCurrentItem( index-1 );
    }
    else if ( but == movedownward_ && index<selectedwellsbox_->size() )
    {
	selectedwellsbox_->insertItem( selectedwellsbox_->getText(), index+2 );
	selectedwellsbox_->removeItem( index );
	selectedwellsbox_->setSelected( index+1 );
	selectedwellsbox_->setCurrentItem( index+1 );
    }
}

