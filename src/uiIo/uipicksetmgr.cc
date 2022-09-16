/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipicksetmgr.h"
#include "uiimppickset.h"
#include "uiioobj.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitextedit.h"

#include "ctxtioobj.h"
#include "filepath.h"
#include "keystrs.h"
#include "uimain.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "keyboardevent.h"
#include "ptrman.h"
#include "od_helpids.h"

uiPickSetMgr::uiPickSetMgr( uiParent* p, Pick::SetMgr& m )
    : setmgr_(m)
    , parent_(p)
{
    PickSetTranslator::tagLegacyPickSets();

    mAttachCB(uiMain::keyboardEventHandler().keyPressed,
	uiPickSetMgr::keyPressedCB);
    mAttachCB( IOM().surveyChanged, uiPickSetMgr::surveyChangeCB );
}


uiPickSetMgr::~uiPickSetMgr()
{
    detachAllNotifiers();
}


void uiPickSetMgr::surveyChangeCB( CallBacker* )
{
    PickSetTranslator::tagLegacyPickSets();
}


bool uiPickSetMgr::storeNewSet( const Pick::Set& ps ) const
{
    return storeNewSet( ps, false );
}


bool uiPickSetMgr::storeNewSet( const Pick::Set& ps, bool noconf ) const
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->setName( ps.name() );
    if ( uiIOObj::fillCtio(*ctio,!noconf) )
    {
	PtrMan<IOObj> ioobj = ctio->ioobj_;
	if ( !doStore(ps,*ioobj) )
	    return false;

	setmgr_.set( ioobj->key(), cCast(Pick::Set*,&ps) );
	return true;
    }

    const IODir iodir( ctio->ctxt_.getSelKey() );
    FilePath fp = iodir.dirName(); fp.add( ".omf" );
    uiMSG().error( tr("Cannot add PointSet to database.\n"
		      "Please check write permission of\n%1")
		    .arg(fp.fullPath()) );

    return false;
}


IOObj* uiPickSetMgr::getSetIOObj( const Pick::Set& ps ) const
{
    const int setidx = setmgr_.indexOf( ps );
    if ( setidx < 0 )
	return nullptr;

    IOObj* ioobj = IOM().get( setmgr_.id(setidx) );
    if ( !ioobj )
    {
	uiString msg = tr("The PointSet '%1' no longer has "
			  "an entry in the data store.\n"
			  "Please use 'Save As' to store this set.")
		     .arg(ps.name());
	uiMSG().warning( msg );
    }
    return ioobj;
}


bool uiPickSetMgr::storeSet( const Pick::Set& ps )
{
    PtrMan<IOObj> ioobj = getSetIOObj( ps );
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    setmgr_.setUnChanged( setmgr_.indexOf(ps) );
    return true;
}


bool uiPickSetMgr::storeSets()
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( !setmgr_.isChanged(idx) )
	    continue;

	storeSet( *setmgr_.get(idx) );
    }

    return true;
}


bool uiPickSetMgr::pickSetsStored() const
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( setmgr_.isChanged(idx) )
	    return false;
    }
    return true;
}


bool uiPickSetMgr::storeSetAs( const Pick::Set& ps )
{
    const bool ispoly =
	ps.isPolygon() || ps.disp_.connect_ != Pick::Set::Disp::None;
    const BufferString oldname = ps.name();
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( PickSet );
    ctio->ctxt_.forread_ = false;
    PickSetTranslator::fillConstraints( ctio->ctxt_, ispoly );
    ctio->setName( oldname );
    uiIOObjSelDlg dlg( parent_, *ctio );
    if ( !dlg.go() )
	return false;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    const_cast<Pick::Set&>(ps).setName( ioobj->name() );
    const int psidx = setmgr_.indexOf( ps );
    if ( psidx >= 0 )
	setmgr_.setID( psidx, ioobj->key() );
    setmgr_.reportChange( this, ps );
    return true;
}


bool uiPickSetMgr::doStore( const Pick::Set& ps, const IOObj& ioobj ) const
{
    IOM().commitChanges( ioobj );
    BufferString bs;
    if ( !PickSetTranslator::store(ps,&ioobj,bs) )
    {
	uiMSG().error( toUiString(bs) );
	return false;
    }

    return true;
}


class uiMergePickSets : public uiDialog
{ mODTextTranslationClass(uiMergePickSets);
public:

uiMergePickSets( uiParent* p, MultiID& mid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrMerge(uiStrings::sPointSet()),
				 tr("Specify sets to merge"),
				 mODHelpKey(mMergePickSetsHelpID) ))
    , ctioin_( PickSetTranslatorGroup::ioContext() )
    , ctioout_( PickSetTranslatorGroup::ioContext() )
    , mid_(mid)
{
    ctioin_.setObj( IOM().get(mid_) );
    selfld = new uiIOObjSelGrp( this, ctioin_, uiStrings::phrSelect(
	     uiStrings::phrJoinStrings(uiStrings::sPointSet(), tr("to Merge"))),
	     uiIOObjSelGrp::Setup(OD::ChooseZeroOrMore) );
    ctioout_.ctxt_.forread_ = false;
    outfld = new uiIOObjSel( this, ctioout_, uiStrings::phrOutput(
							    tr("merged set")) );
    outfld->attach( alignedBelow, selfld );
}


void setInputSets( const BufferStringSet& nms )
{
    selfld->getListField()->setChosen( nms );
}


bool acceptOK( CallBacker* ) override
{
    nrsel = selfld->nrChosen();
    if ( nrsel < 2 )
    { uiMSG().error(tr("Please select at least two sets")); return false; }
    else if (!outfld->commitInput())
    {
	uiMSG().error(uiStrings::phrCannotCreate(
					mToUiStringTodo(outfld->getInput()) ));
	return false;
    }

    if ( ctioout_.ioobj_ )
	mid_ = ctioout_.ioobj_->key();
    return true;
}

    uiIOObjSelGrp*	selfld;
    uiIOObjSel*		outfld;
    CtxtIOObj		ctioin_;
    CtxtIOObj		ctioout_;
    MultiID&		mid_;

    int			nrsel;

};


void uiPickSetMgr::mergeSets( MultiID& mid, const BufferStringSet* nms )
{
    uiMergePickSets dlg( parent_, mid );
    if ( nms )
	dlg.setInputSets( *nms );

    if ( !dlg.go() )
	return;

    uiStringSet errmsgs;
    RefMan<Pick::Set> mergedset = new Pick::Set( dlg.ctioout_.ioobj_->name() );
    for ( int idx=0; idx<dlg.nrsel; idx++ )
    {
	const MultiID& ky = dlg.selfld->chosenID( idx );
	const int setidx = setmgr_.indexOf( ky );
	if ( setidx >= 0 )
	    mergedset->append( *setmgr_.get(setidx) );
	else
	{
	    RefMan<Pick::Set> newset = new Pick::Set;
	    ConstPtrMan<IOObj> ioobj = IOM().get( ky );
	    BufferString msg;
	    if ( PickSetTranslator::retrieve(*newset,ioobj,true, msg) )
		mergedset->append( *newset );
	    else
		errmsgs.add( toUiString(msg) );
	}
    }

    if ( !errmsgs.isEmpty() )
	uiMSG().errorWithDetails( errmsgs, tr("Error during merge.") );

    BufferString msg;
    if ( !PickSetTranslator::store(*mergedset,dlg.ctioout_.ioobj_,msg) )
	uiMSG().error( toUiString(msg) );
}



void uiPickSetMgr::keyPressedCB( CallBacker* )
{
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    bool res = false;
    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    if ( KeyboardEvent::isUnDo(kbe) )
	res = setmgr_.undo().unDo( 1,true );

    if ( KeyboardEvent::isReDo(kbe) )
	res = setmgr_.undo().reDo( 1, true );

    uiMain::keyboardEventHandler().setHandled( res );
}



// uiPickSetMgrInfoDlg
uiPickSetMgrInfoDlg::uiPickSetMgrInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Pick::SetMgr Information"),
		mNoDlgTitle,mNoHelpKey).applybutton(true)
				.applytext(uiStrings::sReload()).modal(false))
{
    setCtrlStyle( uiDialog::CloseOnly );

    browser_ = new uiTextBrowser( this );
    mAttachCB( applyPushed, uiPickSetMgrInfoDlg::refresh );
    mAttachCB( windowShown, uiPickSetMgrInfoDlg::refresh );
}


uiPickSetMgrInfoDlg::~uiPickSetMgrInfoDlg()
{
    detachAllNotifiers();
}


void uiPickSetMgrInfoDlg::refresh( CallBacker* )
{
    StringPairSet infoset;
    Pick::SetMgr::dumpMgrInfo( infoset );
    BufferString text;
    infoset.dumpPretty( text );
    browser_->setText( text );
}
