/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/

#include "uihorizonrelations.h"

#include "uibutton.h"
#include "mousecursor.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uihorizonsortdlg.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "arraynd.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "ctxtioobj.h"
#include "horizonmodifier.h"
#include "horizonrelation.h"
#include "horizonsorter.h"
#include "ioman.h"
#include "ioobj.h"
#include "filepath.h"
#include "iopar.h"
#include "od_helpids.h"


uiHorizonRelationsDlg::uiHorizonRelationsDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup(tr("Horizon relations"),mNoDlgTitle,
		       mODHelpKey(mHorizonRelationsDlgHelpID) ))
    , is2d_( is2d )
{
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Order (top to bottom)"),
			 uiListBox::AboveLeft );
    relationfld_ = new uiListBox( this, su );
    relationfld_->setHSzPol( uiObject::Wide );

    uiPushButton* clearbut =
	new uiPushButton( relationfld_, tr("Clear Order"), true );
    clearbut->activated.notify( mCB(this,uiHorizonRelationsDlg,clearCB) );
    clearbut->attach( rightTo, relationfld_->box() );

    uiPushButton* orderbut =
		new uiPushButton( relationfld_, tr("Read Horizons"), false );
    orderbut->activated.notify( mCB(this,uiHorizonRelationsDlg,readHorizonCB) );
    orderbut->attach( alignedBelow, clearbut );

    crossbut_ = new uiPushButton( relationfld_, tr("Check crossings"), false );
    crossbut_->activated.notify(
		mCB(this,uiHorizonRelationsDlg,checkCrossingsCB) );
    crossbut_->attach( alignedBelow, orderbut );

    waterbut_ = new uiPushButton( relationfld_, tr("Make watertight"), false );
    waterbut_->activated.notify(
		mCB(this,uiHorizonRelationsDlg,makeWatertightCB) );
    waterbut_->attach( alignedBelow, crossbut_ );
    waterbut_->display( false );

    EM::RelationTree::getSorted( is2d, hornames_ );
    EM::RelationTree::getSorted( is2d, horids_ );
    fillRelationField( hornames_ );
    setCtrlStyle( CloseOnly );
}


void uiHorizonRelationsDlg::clearCB( CallBacker* )
{
    if ( !uiMSG().askGoOn(tr("Remove all exising horizon relations?")) )
	return;

    if ( !EM::RelationTree::clear(is2d_,true) )
	return;

    hornames_.erase();
    horids_.erase();
    fillRelationField( hornames_ );
}


void uiHorizonRelationsDlg::readHorizonCB( CallBacker* )
{
    uiHorizonSortDlg dlg( this, is2d_, false );
    if ( !dlg.go() ) return;

    hornames_.erase();
    horids_.erase();
    dlg.getSortedHorizonIDs( horids_ );
    for ( int idx=0; idx<horids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( horids_[idx] );
	if ( !ioobj )
	    horids_.removeSingle( idx-- );

	hornames_.add( ioobj->name() );
    }

    fillRelationField( hornames_ );
}


void uiHorizonRelationsDlg::fillRelationField( const BufferStringSet& strs )
{
    relationfld_->setEmpty();
    relationfld_->addItems( strs );
    crossbut_->setSensitive( strs.size() > 1 );
}


class HorizonModifyDlg : public uiDialog
{ mODTextTranslationClass(HorizonModifyDlg)
public:
HorizonModifyDlg( uiParent* p, const MultiID& mid1, const MultiID& mid2,
		  bool is2d, int nrcross )
    : uiDialog(p,Setup(tr("Horizon relations (Solve crossings)"),
		       uiString::emptyString(),
			mODHelpKey(HorizonModifyDlgHelpID) ))
    , mid1_(mid1)
    , mid2_(mid2)
    , is2d_(is2d)
    , ctio_(is2d ? mMkCtxtIOObj(EMHorizon2D) : mMkCtxtIOObj(EMHorizon3D))
{
    BufferStringSet hornms;
    hornms.add( EM::EMM().objectName(mid1) );
    hornms.add( EM::EMM().objectName(mid2) );

    uiString msg = tr("'%1' crosses '%2' at %3 positions").arg(hornms.get(0))
		 .arg(hornms.get(1)).arg( nrcross );
    uiLabel* lbl = new uiLabel( this, msg );

    horizonfld_ = new uiGenInput( this,
				  uiStrings::phrModify(uiStrings::sHorizon(1)),
				  StringListInpSpec(hornms) );
    horizonfld_->valuechanged.notify( mCB(this,HorizonModifyDlg,horSel) );
    horizonfld_->attach( leftAlignedBelow, lbl );

    modefld_ = new uiGenInput( this, tr("Modify action"),
			       BoolInpSpec(true,uiStrings::sShift(),
			       uiStrings::sRemove()) );
    modefld_->attach( alignedBelow, horizonfld_ );

    savefld_ = new uiGenInput( this, uiStrings::phrSave(
			       uiStrings::phrJoinStrings(tr("Modified"),
			       uiStrings::sHorizon(1))),
			       BoolInpSpec(true,tr("As new"),
			       uiStrings::sOverwrite()) );
    savefld_->valuechanged.notify( mCB(this,HorizonModifyDlg,saveCB) );
    savefld_->attach( alignedBelow, modefld_ );
    savefld_->setSensitive( EM::canOverwrite(mid1) );

    ctio_->ctxt_.forread_ = false;
    objfld_ = new uiIOObjSel( this, *ctio_, uiStrings::sHorizon() );
    objfld_->display( false );
    objfld_->attach( alignedBelow, savefld_ );

    saveCB(0);
    horSel(0);
}


~HorizonModifyDlg()
{
    delete ctio_->ioobj_;
    delete ctio_;
}


void saveCB( CallBacker* )
{
    objfld_->display( savefld_->getBoolValue() );
}


void horSel( CallBacker* )
{
    const bool topisstatic = horizonfld_->getIntValue() == 1;
    const MultiID& targetmid = topisstatic ? mid2_ : mid1_;
    BufferString hornm = EM::EMM().objectName( targetmid );
    hornm += "_edited";
    objfld_->setInputText( hornm );
    savefld_->setValue( true );
    savefld_->setSensitive( EM::canOverwrite(targetmid) );
    saveCB( 0 );
}


#define mErrRet(msg) { if ( !msg.isEmpty() ) uiMSG().error(msg); return false; }

bool acceptOK( CallBacker* ) override
{
    const bool saveas = savefld_->getBoolValue();
    const bool topisstatic = horizonfld_->getIntValue() == 1;
    const EM::ObjectID objid =
		EM::EMM().getObjectID( topisstatic ? mid2_ : mid1_ );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    MultiID outmid;
    RefMan<EM::EMObject> outemobj = 0;

    if ( saveas )
    {
	if ( !objfld_->commitInput() )
	    mErrRet((objfld_->isEmpty() ? uiStrings::phrSelect(
		    uiStrings::phrOutput(uiStrings::sSurface())) :
		    uiStrings::sEmptyString()))
	outmid = ctio_->ioobj_->key();
	EM::ObjectID outemobjid =
	    EM::EMM().createObject( emobj->getTypeStr(), ctio_->ioobj_->name());
	outemobj = EM::EMM().getObject( outemobjid );
	outemobj->setPreferredColor( emobj->preferredColor() );

	if ( !is2d_ )
	{
	    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
	    mDynamicCastGet(EM::Horizon3D*,outhor3d,outemobj.ptr());
	    if ( !hor3d || !outhor3d )
		return false;

	    Array2D<float>* arr2d;
	    arr2d = hor3d->createArray2D();
	    BinID start( hor3d->geometry().rowRange().start,
			 hor3d->geometry().colRange().start );
	    BinID step( hor3d->geometry().step().row(),
			hor3d->geometry().step().col() );
	    outhor3d->setMultiID( outmid );
	    outhor3d->geometry().geometryElement()->setArray(
		    start, step, arr2d, true );
	}
	else
	{
	    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
	    mDynamicCastGet(EM::Horizon2D*,outhor2d,outemobj.ptr());
	    if ( !hor2d || !outhor2d )
		return false;
	    for ( int idx=0; idx<hor2d->geometry().nrLines(); idx++ )
	    {
		const Pos::GeomID geomid = hor2d->geometry().geomID( idx );
		PtrMan< Array1D<float> > arr1d;
		arr1d = hor2d->createArray1D( geomid );
		outhor2d->geometry().addLine( geomid );
		outhor2d->setArray1D(*arr1d,geomid,false);
	    }
	}
    }

    HorizonModifier modifier( is2d_ );
    modifier.setHorizons( topisstatic ? mid1_ : saveas ? outmid : mid1_,
			  !topisstatic ? mid2_ : saveas ? outmid : mid2_ );
    modifier.setMode( modefld_->getBoolValue() ? HorizonModifier::Shift
					       : HorizonModifier::Remove );

    modifier.setStaticHorizon( topisstatic );
    modifier.doWork();

    PtrMan<Executor> exec = !saveas ? emobj->saver() : outemobj->saver();
    if ( !exec ) mErrRet(uiStrings::phrCannotSave(uiStrings::sHorizon(1)));

    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, *exec );
}

protected:
    bool	is2d_;
    MultiID	mid1_;
    MultiID	mid2_;

    CtxtIOObj*	ctio_;

    uiGenInput*	horizonfld_;
    uiGenInput*	modefld_;
    uiGenInput*	savefld_;
    uiIOObjSel*	objfld_;
};


void uiHorizonRelationsDlg::checkCrossingsCB( CallBacker* )
{
    MouseCursorChanger chgr( MouseCursor::Wait );

    HorizonSorter sorter( horids_, is2d_ );
    sorter.setName( "Check crossings" );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, sorter ) ) return;
    MouseCursorManager::restoreOverride();

    int count = 0;
    for ( int idx=0; idx<horids_.size(); idx++ )
    {
	for ( int idy=idx+1; idy<horids_.size(); idy++ )
	{
	    const int nrcrossings =
		sorter.getNrCrossings( horids_[idx], horids_[idy] );
	    if ( nrcrossings == 0 ) continue;

	    TypeSet<MultiID> sortedids;
	    sorter.getSortedList( sortedids );
	    const int idx1 = sortedids.indexOf( horids_[idx] );
	    const int idx2 = sortedids.indexOf( horids_[idy] );
	    HorizonModifyDlg dlg( this, sortedids[mMIN(idx1,idx2)],
				  sortedids[mMAX(idx1,idx2)], is2d_,
				  nrcrossings );
	    dlg.go();
	    count++;
	}
    }

    if ( count > 0 )
	return;

    uiMSG().message( tr("No crossings found") );
}


void uiHorizonRelationsDlg::makeWatertightCB( CallBacker* )
{
    uiMSG().message( tr("Not implemented yet") );
}
