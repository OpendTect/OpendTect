/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          October 2007
 RCS:           $Id: uiprestkmergedlg.cc,v 1.5 2008-01-22 15:04:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiprestkmergedlg.h"

#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "cubesampling.h"
#include "pixmap.h"
#include "ptrman.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "multiid.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"
#include "seispsmerge.h"
#include "seissingtrcproc.h"
#include "transl.h"
#include "uiioobjsel.h"


uiPreStackMergeDlg::uiPreStackMergeDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Merge Pre-Stack Data","",
				 "109.0.0"))
    , inctio_(*mMkCtxtIOObj(SeisPS3D))
    , outctio_(*mMkCtxtIOObj(SeisPS3D))
    , volsbox_(0), selvolsbox_(0)
{
    BufferString title( "Select volumes to merge into one" );
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


void uiPreStackMergeDlg::createFields( uiGroup* topgrp )
{
    volsbox_ = new uiListBox( topgrp, "Available Volumes", true );
    selvolsbox_ = new uiListBox( topgrp, "Selected Volumes", true );
    outctio_.ctxt.forread = false;
    outpfld_ = new uiIOObjSel( this, outctio_, "Output Volume" );
    subselfld_ = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withz(false)
	    			    .withstep(true) );
}


void uiPreStackMergeDlg::createSelectButtons( uiGroup* selbuttons )
{
    const ioPixmap pm0( "rightarrow.png" );
    const ioPixmap pm1( "leftarrow.png" );

    uiLabel* sellbl = new uiLabel( selbuttons, "Select" );
    CallBack cb = mCB(this,uiPreStackMergeDlg,selButPush);
    toselect_ = new uiToolButton( selbuttons, "", pm0, cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( selbuttons, "", pm1, cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiPreStackMergeDlg::createMoveButtons( uiGroup* movebuttons )
{
    const ioPixmap pm0( "uparrow.png" );
    const ioPixmap pm1( "downarrow.png" );

    uiLabel* movelbl = new uiLabel( movebuttons, "Change \n Priority" );
    CallBack cb = mCB(this,uiPreStackMergeDlg,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, "", pm0, cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons, "", pm1, cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::Undef );
    movebuttons->setHAlignObj( moveupward_ );
}


void uiPreStackMergeDlg::attachFields( uiGroup* selbuttons, uiGroup* topgrp,
					 uiGroup* movebuttons )
{
    selbuttons->attach( centeredLeftOf, selvolsbox_ );
    selbuttons->attach( ensureRightOf, volsbox_ );
    selvolsbox_->attach( rightTo, volsbox_ );
    movebuttons->attach( centeredRightOf, selvolsbox_ );
    subselfld_->attach( rightAlignedBelow, topgrp );
    outpfld_->attach( rightAlignedBelow, subselfld_ );
}


void uiPreStackMergeDlg::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<volsbox_->size(); idx++ )
	{
	    if ( !volsbox_->isSelected(idx) ) continue;
	   
	    selvolsbox_->addItem( volsbox_->textOfItem(idx));
	    volsbox_->removeItem(idx);
	    lastusedidx = idx;
	    volsbox_->sort();
	    idx--;
	}
	volsbox_->setSelected( lastusedidx<volsbox_->size() ?
			        lastusedidx : volsbox_->size()-1 );
    }
    else if ( but == fromselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<selvolsbox_->size(); idx++ )
	{
	    if ( !selvolsbox_->isSelected(idx) ) continue;
	   
	    volsbox_->addItem( selvolsbox_->textOfItem(idx));
	    selvolsbox_->removeItem(idx);
	    lastusedidx = idx;
	    idx--;
	}
	selvolsbox_->setSelected( lastusedidx<selvolsbox_->size() ?
				  lastusedidx : selvolsbox_->size()-1 );
    }
}


void uiPreStackMergeDlg::fillListBox()
{
    IOM().to( inctio_.ctxt.getSelKey() );
    IODirEntryList entrylist( IOM().dirPtr(), inctio_.ctxt );

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	entrylist.setCurrent( idx );
	allvolsids_ += entrylist.selected()->key();
	IOObj* ioobj = IOM().get(entrylist.selected()->key());
	allvolsnames_.add( entrylist[idx]->name() );
	delete ioobj;
    }

    volsbox_->addItems( allvolsnames_ );
}


bool uiPreStackMergeDlg::setSelectedVols()
{
    selobjs_.erase();
    const int nrobjs = selvolsbox_->size();
    if ( nrobjs < 2 )
    {
	uiMSG().error( "Select at least two volumes to merge" );
	return false;
    }

    BufferString storage = "";
    static const char* storagekey = "Data storage";
    for ( int idx=0; idx<nrobjs; idx++ )
    {
	const char* txt = selvolsbox_->textOfItem(idx);
	int volidx = allvolsnames_.indexOf( txt );
	if ( volidx < 0 ) continue;

	const MultiID id = allvolsids_[volidx];
	IOObj* ioobj = IOM().get( id );
	if ( !ioobj ) continue;

	if ( !selobjs_.size() )
	    storage = ioobj->pars().find( storagekey );
	else if ( storage != ioobj->pars().find(storagekey) )
	{
	    uiMSG().error( "Files with different storage types selected" );
	    return false;
	}

	selobjs_ += ioobj;
    }

    if ( !outpfld_->commitInput(true) )
    {
	uiMSG().error( "Please enter an output data set name" );
	return false;
    }
    else if ( outctio_.ioobj->implExists(false)
	      && !uiMSG().askGoOn("Output data set exists. Overwrite?") )
	return false;

    outctio_.ioobj->pars().set( storagekey, storage );
    IOM().commitChanges( *outctio_.ioobj );
    return true;
}


void uiPreStackMergeDlg::moveButPush( CallBacker* cb )
{
    if ( selvolsbox_->nrSelected() > 1 ) return;

    const int idx = selvolsbox_->currentItem();
    const char* item = selvolsbox_->textOfItem(idx);
    mDynamicCastGet(uiToolButton*,but,cb);
    if ( but == moveupward_ )
    {
	if ( idx < 1 ) return;
	selvolsbox_->removeItem(idx);
	selvolsbox_->insertItem( item, idx-1 );
	selvolsbox_->setCurrentItem( idx - 1 );
    }
    else if ( but == movedownward_ )
    {
	const int totalnr = selvolsbox_->size();
	if ( idx > totalnr-2 ) return;
	selvolsbox_->removeItem(idx);
	if ( idx == totalnr-2 ) selvolsbox_->addItem( item );
	else selvolsbox_->insertItem( item, idx+1 );
	selvolsbox_->setCurrentItem( idx + 1 );
    }
}


bool uiPreStackMergeDlg::acceptOK( CallBacker* cb )
{
    if ( !setSelectedVols() ) return false;

    HorSampling hs = subselfld_->data().cs_.hrg;
    PtrMan<SeisPSMerger> Exec =
	   new SeisPSMerger( selobjs_, outctio_.ioobj, hs );
    uiExecutor dlg( this, *Exec );
    return dlg.go();
}
