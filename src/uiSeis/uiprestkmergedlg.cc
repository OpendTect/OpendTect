/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          October 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprestkmergedlg.cc,v 1.18 2009-08-21 10:11:46 cvsbert Exp $";

#include "uiprestkmergedlg.h"

#include "uipossubsel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "cubesampling.h"
#include "pixmap.h"
#include "posinfo.h"
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
#include "seisselectionimpl.h"
#include "seissingtrcproc.h"
#include "transl.h"
#include "uiioobjsel.h"


uiPreStackMergeDlg::uiPreStackMergeDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Merge Pre-Stack Data",
				 "Select volumes to merge into one",
				 "109.0.0"))
    , inctio_(*mMkCtxtIOObj(SeisPS3D))
    , outctio_(*mMkCtxtIOObj(SeisPS3D))
    , volsbox_(0), selvolsbox_(0)
{
    uiGroup* topgrp = new uiGroup( this, "selection group" );
    uiGroup* selbuttons = new uiGroup( topgrp, "select buttons" );
    uiGroup* movebuttons = new uiGroup( topgrp, "move buttons" );
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );
    createFields( topgrp );
    attachFields( selbuttons, topgrp, movebuttons );

    fillListBox();
}


uiPreStackMergeDlg::~uiPreStackMergeDlg()
{
    deepErase( selobjs_ );
    delete inctio_.ioobj; delete outctio_.ioobj;
    delete &inctio_; delete &outctio_;
}


void uiPreStackMergeDlg::createFields( uiGroup* topgrp )
{
    volsbox_ = new uiListBox( topgrp, "Available Volumes", true );
    selvolsbox_ = new uiListBox( topgrp, "Selected Volumes", true );
    outctio_.ctxt.forread = false;
    outpfld_ = new uiIOObjSel( this, outctio_, "Output Volume" );
    uiPosSubSel::Setup psssu( false, false );
    psssu.choicetype( uiPosSubSel::Setup::OnlySeisTypes )
	 .withstep( false );
    subselfld_ = new uiPosSubSel( this, psssu );
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
    deepErase( selobjs_ );
    const int nrobjs = selvolsbox_->size();
    if ( nrobjs < 2 )
    {
	uiMSG().error( "Select at least two volumes to merge" );
	return false;
    }

    BufferString storage = "";
    static const char* storagekey = "Data storage";
    bool altstormsgdone = false;
    for ( int idx=0; idx<nrobjs; idx++ )
    {
	const char* txt = selvolsbox_->textOfItem(idx);
	int volidx = allvolsnames_.indexOf( txt );
	if ( volidx < 0 ) continue;

	const MultiID id = allvolsids_[volidx];
	IOObj* ioobj = IOM().get( id );
	if ( !ioobj ) continue;

	if ( selobjs_.isEmpty() )
	    storage = ioobj->pars().find( storagekey );
	else if ( !altstormsgdone && storage != ioobj->pars().find(storagekey) )
	{
	    altstormsgdone = true;
	    if ( !uiMSG().askContinue( "Not all stores have the same storage type."
				   "\nContinue?" ) )
		return false;
	}

	selobjs_ += ioobj;
    }

    if ( !outpfld_->commitInput() )
    {
	if ( outpfld_->isEmpty() )
	    uiMSG().error( "Please enter an output data set name" );
	return false;
    }

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

    PtrMan<Seis::SelData> sd = 0;
    if ( !subselfld_->isAll() )
    {
	IOPar iop; subselfld_->fillPar( iop );
	sd = Seis::SelData::get( iop );
    }
    PtrMan<SeisPSMerger> Exec = new SeisPSMerger( selobjs_, *outctio_.ioobj,
	    					  sd );
    uiTaskRunner dlg( this );
    return dlg.execute( *Exec );
}


uiPreStackCopyDlg::uiPreStackCopyDlg( uiParent* p, const MultiID& key )
    : uiDialog(p,uiDialog::Setup("Copy Pre-Stack Data",
				 "",
				 "109.0.0"))
    , inctio_(*mMkCtxtIOObj(SeisPS3D))
    , outctio_(*mMkCtxtIOObj(SeisPS3D))
{
    inctio_.setObj( key );
    inpfld_ = new uiIOObjSel( this, inctio_, "Input Volume" );
    inpfld_->selectiondone.notify( mCB(this,uiPreStackCopyDlg,objSel) );

    uiPosSubSel::Setup psssu( false, false );
    psssu.choicetype( uiPosSubSel::Setup::OnlySeisTypes )
	 .withstep( true );
    subselfld_ = new uiPosSubSel( this, psssu );
    subselfld_->attach( alignedBelow, inpfld_ );

    outctio_.ctxt.forread = false;
    outpfld_ = new uiIOObjSel( this, outctio_, "Output Volume" );
    outpfld_->attach( alignedBelow, subselfld_ );
    finaliseDone.notify( mCB(this,uiPreStackCopyDlg,objSel) );
}


uiPreStackCopyDlg::~uiPreStackCopyDlg()
{
    delete &inctio_; delete &outctio_;
}


void uiPreStackCopyDlg::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() || !inctio_.ioobj )
	return;

    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *inctio_.ioobj );
    if ( !rdr ) return;

    HorSampling hs;
    StepInterval<int> inlrg, crlrg;
    rdr->posData().getInlRange( inlrg );
    rdr->posData().getCrlRange( crlrg );
    hs.set( inlrg, crlrg );
    IOPar iop;
    Seis::RangeSelData(hs).fillPar( iop );
    subselfld_->usePar( iop );
}


bool uiPreStackCopyDlg::acceptOK( CallBacker* cb )
{
    if ( !inpfld_->commitInput() )
    {
	uiMSG().error( "Please select the input data set" );
	return false;
    }

    if ( !outpfld_->commitInput() )
    {
	if ( outpfld_->isEmpty() )
	    uiMSG().error( "Please enter an output data set name" );
	return false;
    }

    PtrMan<Seis::SelData> sd = 0;
    if ( !subselfld_->isAll() )
    {
	IOPar iop; subselfld_->fillPar( iop );
	sd = Seis::SelData::get( iop );
    }

    ObjectSet<IOObj> selobjs;
    selobjs += inctio_.ioobj;
    PtrMan<SeisPSMerger> Exec = new SeisPSMerger( selobjs, *outctio_.ioobj,
	    					  sd );
    uiTaskRunner dlg( this );
    return dlg.execute( *Exec );
}
