/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestkcopy.h"
#include "uiprestkmergedlg.h"

#include "uipossubsel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uistrings.h"
#include "trckeyzsampling.h"
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
#include "survinfo.h"
#include "transl.h"
#include "uiioobjsel.h"
#include "od_helpids.h"


uiPreStackMergeDlg::uiPreStackMergeDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Merge Prestack Data"),
				 tr("Select data stores to merge into one"),
				 mODHelpKey(mPreStackMergeDlgHelpID) ))
    , inctio_(*mMkCtxtIOObj(SeisPS3D))
    , outctio_(*mMkCtxtIOObj(SeisPS3D))
    , volsbox_(0), selvolsbox_(0)
{
    uiGroup* topgrp = new uiGroup( this, "selection group" );
    uiGroup* selbuttons = new uiGroup( topgrp, "select buttons" );
    movebuttons_ = new uiGroup( topgrp, "move buttons" );
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons_ );
    createFields( topgrp );
    attachFields( selbuttons, topgrp, movebuttons_ );

    fillListBox();
    setToolButtonProperty();
}


uiPreStackMergeDlg::~uiPreStackMergeDlg()
{
    deepErase( selobjs_ );
    delete inctio_.ioobj_; delete outctio_.ioobj_;
    delete &inctio_; delete &outctio_;
}


void uiPreStackMergeDlg::createFields( uiGroup* topgrp )
{
    volsbox_ = new uiListBox( topgrp, "Available Stores",
				OD::ChooseAtLeastOne );
    selvolsbox_ = new uiListBox( topgrp, "Selected Stores" );
    outctio_.ctxt_.forread_ = false;
    outctio_.ctxt_.deftransl_ = CBVSSeisPS3DTranslator::translKey();
    stackfld_ = new uiGenInput( this, tr("Duplicate traces"),
				BoolInpSpec(true,tr("Stack"),tr("Use first")) );
    stackfld_->valuechanged.notify( mCB(this,uiPreStackMergeDlg,stackSel) );
    outpfld_ = new uiIOObjSel( this, outctio_, uiStrings::sOutpDataStore() );
    uiPosSubSel::Setup psssu( false, false );
    psssu.choicetype( uiPosSubSel::Setup::OnlySeisTypes )
	 .withstep( false );
    subselfld_ = new uiPosSubSel( this, psssu );
}


void uiPreStackMergeDlg::createSelectButtons( uiGroup* selbuttons )
{
    uiLabel* sellbl = new uiLabel( selbuttons, uiStrings::sSelect() );
    CallBack cb = mCB(this,uiPreStackMergeDlg,selButPush);
    toselect_ = new uiToolButton( selbuttons, uiToolButton::RightArrow,
				  uiString::emptyString(), cb);
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( selbuttons, uiToolButton::LeftArrow,
				    uiString::emptyString(),cb);
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiPreStackMergeDlg::createMoveButtons( uiGroup* movebuttons )
{
    uiLabel* movelbl = new uiLabel( movebuttons, tr("Change \n Priority") );
    CallBack cb = mCB(this,uiPreStackMergeDlg,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, uiToolButton::UpArrow,
				    uiString::emptyString(),cb);
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons, uiToolButton::DownArrow,
				      uiString::emptyString(), cb );
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
    stackfld_->attach( centeredBelow, topgrp );
    subselfld_->attach( alignedBelow, stackfld_ );
    outpfld_->attach( alignedBelow, subselfld_ );
}


void uiPreStackMergeDlg::stackSel( CallBacker* )
{
    movebuttons_->setSensitive( !stackfld_->getBoolValue() );
}


void uiPreStackMergeDlg::setToolButtonProperty()
{
    toselect_->setSensitive( !volsbox_->isEmpty() );
    fromselect_->setSensitive( !selvolsbox_->isEmpty() );
}


void uiPreStackMergeDlg::setInputIds( const BufferStringSet& selnms )
{
    volsbox_->setChosen( selnms );
}


void uiPreStackMergeDlg::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    BufferStringSet selnms;
    if ( but == toselect_ )
    {
	volsbox_->getChosen( selnms );
	for ( int idx=0; idx<selnms.size(); idx++ )
	{
	    selvolsbox_->addItem( toUiString(selnms.get( idx )) );
	    volsbox_->removeItem( selnms.get( idx ) );
	}

    }
    else if ( but == fromselect_ )
    {
	selvolsbox_->getChosen( selnms );
	for ( int idx=0; idx<selnms.size(); idx++ )
	{
	    volsbox_->addItem( toUiString(selnms.get( idx )) );
	    selvolsbox_->removeItem( selnms.get( idx ) );
	}
    }

    volsbox_->sortItems();
    setToolButtonProperty();
}


void uiPreStackMergeDlg::fillListBox()
{
    const IODir iodir( inctio_.ctxt_.getSelKey() );
    IODirEntryList entrylist( iodir, inctio_.ctxt_ );

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	entrylist.setCurrent( idx );
	allvolsids_ += entrylist.selected()->key();
	IOObj* ioobj = IOM().get(entrylist.selected()->key());
	allvolsnames_.add( entrylist[idx]->name() );
	delete ioobj;
    }

    volsbox_->addItems( allvolsnames_ );
    stackSel(0);
}


bool uiPreStackMergeDlg::setSelectedVols()
{
    deepErase( selobjs_ );
    const int nrobjs = selvolsbox_->size();
    if ( nrobjs < 2 )
    {
	uiMSG().error( tr("Select at least two volumes to merge") );
	return false;
    }

    BufferString storage = "";
    const char* storagekey = sKey::DataStorage();
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
	    if ( !uiMSG().askContinue(
		   tr("Not all stores have the same storage type.\nContinue?")))
		return false;
	}

	selobjs_ += ioobj;
    }

    if ( !outpfld_->commitInput() )
    {
	if ( outpfld_->isEmpty() )
	    uiMSG().error( tr("Please enter an output data set name") );
	return false;
    }

    outctio_.ioobj_->pars().set( storagekey, storage );
    IOM().commitChanges( *outctio_.ioobj_ );
    return true;
}


void uiPreStackMergeDlg::moveButPush( CallBacker* cb )
{
    if ( selvolsbox_->nrChosen() != 1 ) return;

    const int idx = selvolsbox_->currentItem();
    const char* item = selvolsbox_->textOfItem(idx);
    mDynamicCastGet(uiToolButton*,but,cb);
    if ( but == moveupward_ )
    {
	if ( idx < 1 ) return;
	selvolsbox_->removeItem(idx);
	selvolsbox_->insertItem( toUiString(item), idx-1 );
	selvolsbox_->setCurrentItem( idx - 1 );
    }
    else if ( but == movedownward_ )
    {
	const int totalnr = selvolsbox_->size();
	if ( idx > totalnr-2 ) return;
	selvolsbox_->removeItem(idx);
	if ( idx == totalnr-2 ) selvolsbox_->addItem( toUiString(item) );
	else selvolsbox_->insertItem( toUiString(item), idx+1 );
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

    const bool dostack = stackfld_->getBoolValue();
    ObjectSet<IOObj>& selobjs = selobjs_;
    ObjectSet<const IOObj>& selobjsconst
		= reinterpret_cast<ObjectSet<const IOObj>&>(selobjs);
    PtrMan<SeisPSMerger> exec = new SeisPSMerger( selobjsconst,*outctio_.ioobj_,
						  dostack, sd );
    exec->setName( "Merge Prestack Data Stores" );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, *exec );
}


uiPreStackOutputGroup::uiPreStackOutputGroup( uiParent* p )
    : uiGroup(p,"Prestack Data Output Group")
    , inpioobj_(0)
{

    uiPosSubSel::Setup psssu( false, true );
    psssu.choicetype( uiPosSubSel::Setup::OnlySeisTypes )
	 .withstep( true );
    subselfld_ = new uiPosSubSel( this, psssu );

    uiString offsetrangestr (tr("Offset range %1").arg(SI().getXYUnitString()));
    offsrgfld_ = new uiGenInput( this, offsetrangestr,
				 FloatInpSpec(0), FloatInpSpec() );
    offsrgfld_->attach( alignedBelow, subselfld_ );

    IOObjContext ctxt( mIOObjContext(SeisPS3D) );
    ctxt.forread_ = false;
    ctxt.deftransl_ = CBVSSeisPS3DTranslator::translKey();
    outpfld_ = new uiIOObjSel( this, ctxt, uiStrings::sOutpDataStore() );
    outpfld_->attach( alignedBelow, offsrgfld_ );

    setHAlignObj( subselfld_ );
}


uiPreStackOutputGroup::~uiPreStackOutputGroup()
{
    delete inpioobj_;
}


void uiPreStackOutputGroup::setInput( const IOObj& ioobj )
{
    if ( inpioobj_ && inpioobj_->key() == ioobj.key() )
	return;

    delete inpioobj_;
    inpioobj_ = ioobj.clone();

    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *inpioobj_ );
    if ( !rdr ) return;

    TrcKeySampling hs;
    StepInterval<int> inlrg, crlrg;
    rdr->posData().getInlRange( inlrg );
    rdr->posData().getCrlRange( crlrg );
    hs.set( inlrg, crlrg );
    IOPar iop;
    Seis::RangeSelData(hs).fillPar( iop );
    subselfld_->usePar( iop );
}


bool uiPreStackOutputGroup::go()
{
    if ( !inpioobj_ )
	return false;

    const IOObj* outioobj = outpfld_->ioobj();
    if ( !outioobj )
	return false;

    PtrMan<Seis::SelData> sd = 0;
    if ( !subselfld_->isAll() )
    {
	IOPar iop; subselfld_->fillPar( iop );
	sd = Seis::SelData::get( iop );
    }

    SeisPSCopier copier( *inpioobj_, *outioobj, sd );
    float ofsrgstart = offsrgfld_->getFValue( 0 );
    float ofsrgstop = offsrgfld_->getFValue( 1 );
    if ( mIsUdf(ofsrgstart) )
	ofsrgstart = 0;

    copier.setOffsetRange( ofsrgstart, ofsrgstop );

    uiTaskRunner trunner( this );
    return trunner.execute( copier );
}


uiPreStackCopyDlg::uiPreStackCopyDlg( uiParent* p, const MultiID& key )
    : uiDialog(p,uiDialog::Setup(tr("Copy Prestack Data"),
				 uiString::emptyString(),
                                 mODHelpKey(mPreStackCopyDlgHelpID) ))
{
    const CallBack selcb( mCB(this,uiPreStackCopyDlg,objSel) );
    inpfld_ = new uiIOObjSel( this, mIOObjContext(SeisPS3D),
				tr("Input Prestack Data Store") );
    inpfld_->setInput( key );
    inpfld_->selectionDone.notify( selcb );

    outgrp_ = new uiPreStackOutputGroup( this );
    outgrp_->attach( alignedBelow, inpfld_ );

    postFinalize().notify( selcb );
}


void uiPreStackCopyDlg::objSel( CallBacker* )
{
    const IOObj* ioobj = inpfld_->ioobj( true );
    if ( ioobj )
	outgrp_->setInput( *ioobj );
}


bool uiPreStackCopyDlg::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;

    outgrp_->setInput( *inioobj );
    return outgrp_->go();
}
