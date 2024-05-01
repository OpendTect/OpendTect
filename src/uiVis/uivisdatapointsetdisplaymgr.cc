/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivisdatapointsetdisplaymgr.h"

#include "uicombobox.h"
#include "uicreatepicks.h"
#include "uigeninput.h"
#include "uiioobj.h"
#include "uimaterialdlg.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uislider.h"
#include "uivispartserv.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "datapointset.h"
#include "emrandomposbody.h"
#include "emmanager.h"
#include "executor.h"
#include "pickset.h"
#include "picksettr.h"
#include "visdata.h"
#include "visrandomposbodydisplay.h"
#include "vissurvscene.h"
#include "vispointsetdisplay.h"
#include "vispointset.h"

uiVisDataPointSetDisplayMgr::uiVisDataPointSetDisplayMgr(uiVisPartServer& serv )
    : DataPointSetDisplayMgr()
    , visserv_(serv)
    , vismenu_(visserv_.getMenuHandler())
    , createbodymnuitem_(tr("Create Geobody"))
    , storepsmnuitem_(m3Dots(tr("Save as PointSet")))
    , removemnuitem_(tr("Remove points inside polygon"))
    , propmenuitem_(m3Dots(uiStrings::sProperties()))
    , treeToBeAdded(this)
{
    if ( vismenu_ )
    {
	mAttachCB( vismenu_->createnotifier,
		   uiVisDataPointSetDisplayMgr::createMenuCB );
	mAttachCB( vismenu_->handlenotifier,
		   uiVisDataPointSetDisplayMgr::handleMenuCB );
    }
}


uiVisDataPointSetDisplayMgr::~uiVisDataPointSetDisplayMgr()
{
    detachAllNotifiers();
    vismenu_ = nullptr;
    deepErase( displayinfos_ );
}


class uiSetSizeDlg : public uiDialog
{ mODTextTranslationClass(uiSetSizeDlg)
public:
uiSetSizeDlg( uiParent * p, visSurvey::PointSetDisplay* disp )
    : uiDialog( p, uiDialog::Setup(tr("Set size of points"),
		mNoDlgTitle, mNoHelpKey) )
    , pointsetdisp_(disp)
{
    setCtrlStyle( uiDialog::CloseOnly );
    const float fsz = (float)pointsetdisp_->getPointSize();
    slider_ = new uiSlider( this, uiSlider::Setup(uiStrings::sSize()), "Size" );
    slider_->setInterval( StepInterval<float>(fsz-10.0f,fsz+10.0f,1.0f) );
    slider_->setMinValue( 1 );
    slider_->setMaxValue( 15 );
    slider_->setValue( fsz );
    slider_->setTickMarks( uiSlider::Below );
    slider_->setTickStep( 1 );
    mAttachCB( slider_->sliderMoved, uiSetSizeDlg::sizeChangedCB );
}

~uiSetSizeDlg()
{
    detachAllNotifiers();
}

void sizeChangedCB( CallBacker* )
{
    pointsetdisp_->setPointSize( slider_->getIntValue() );
}

    uiSlider*			slider_;
    RefMan<visSurvey::PointSetDisplay> pointsetdisp_;
};


void uiVisDataPointSetDisplayMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !menu ) return;

    const VisID displayid( menu->menuID() );
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;

    bool dispcorrect = false;
    for ( const auto* displayinfo : displayinfos_ )
    {
	dispcorrect = displayinfo->visids_.isPresent( displayid );
	if ( dispcorrect )
	    break;
    }

    if ( !dispcorrect )
	return;

    menu->removeItems();
    mAddMenuItem( menu, &propmenuitem_, true, false );
    propmenuitem_.iconfnm = "disppars";

    /*TODO: Make a MarchingCube based implementation before enabling.
    mAddMenuItem( menu, &createbodymnuitem_, true, false ); */

    mAddMenuItem( menu, &storepsmnuitem_, true, false );
    mAddMenuItem( menu, &removemnuitem_, true, false );
}


class uiCreateBodyDlg : public uiDialog
{ mODTextTranslationClass(uiCreateBodyDlg)
public:
uiCreateBodyDlg( uiParent* p, const DataPointSetDisplayProp& dispprop )
    : uiDialog(p,uiDialog::Setup(tr("Create New Geobody"),mNoDlgTitle,
				 mNoHelpKey))
{
    if ( dispprop.showSelected() )
    {
	auto* cbx = new uiLabeledComboBox( this, tr("Selection Group") );
	selfld_ = cbx->box();
	BufferStringSet selgrpnms = dispprop.selGrpNames();
	TypeSet<OD::Color> selgrpcols = dispprop.selGrpColors();
	for ( int idx=0; idx<selgrpnms.size(); idx++ )
	{
	    selfld_->addItem( toUiString(selgrpnms[idx]->buf()) );
	    uiPixmap pixmap( 20, 20 );
	    OD::Color col = selgrpcols[ idx ];
	    pixmap.fill( col );
	    selfld_->setPixmap( idx, pixmap );
	}
    }
    else
    {
	rgfld_ = new uiGenInput( this, tr("Create geobody from value range"),
				 FloatInpIntervalSpec(false) );
	rgfld_->setValue( dispprop.colMapperSetUp().range_ );
    }
}

int selGrpIdx() const
{ return selfld_ ? selfld_->currentItem() : -1; }

Interval<float> getValRange() const
{
    return rgfld_ ? rgfld_->getFInterval()
		  : Interval<float>(mUdf(float),mUdf(float));
}

    uiComboBox*		selfld_ = nullptr;
    uiGenInput*		rgfld_	= nullptr;
};


class uiCreatePicksDlg : public uiCreatePicks
{ mODTextTranslationClass(uiCreatePicksDlg);
public:

uiCreatePicksDlg( uiParent* p, const DataPointSetDisplayProp& dispprop )
    : uiCreatePicks(p,false,false)
{
    if ( dispprop.showSelected() )
    {
	auto* cbx = new uiLabeledComboBox( this, tr("Selection Group") );
	selfld_ = cbx->box();
	BufferStringSet selgrpnms = dispprop.selGrpNames();
	TypeSet<OD::Color> selgrpcols = dispprop.selGrpColors();
	for ( int idx=0; idx<selgrpnms.size(); idx++ )
	{
	    selfld_->addItem( toUiString(selgrpnms[idx]->buf()) );
	    uiPixmap pixmap( 20, 20 );
	    OD::Color col = selgrpcols[ idx ];
	    pixmap.fill( col );
	    selfld_->setPixmap( idx, pixmap );
	}

	addStdFields( cbx->attachObj() );
    }
    else
    {
	rgfld_ = new uiGenInput( this, tr("Create geobody from value range"),
				 FloatInpIntervalSpec(false) );
	rgfld_->setValue( dispprop.colMapperSetUp().range_ );
	addStdFields( rgfld_->attachObj() );
    }
}

int selGrpIdx() const
{ return selfld_ ? selfld_->currentItem() : -1; }

Interval<float> getValRange() const
{
    return rgfld_ ? rgfld_->getFInterval()
		  : Interval<float>(mUdf(float),mUdf(float));
}

    uiComboBox*		selfld_ = nullptr;
    uiGenInput*		rgfld_	= nullptr;
};


void uiVisDataPointSetDisplayMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 ) return;
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( !menu ) return;

    const VisID displayid( menu->menuID() );
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;

    bool dispcorrect = false;
    for ( const auto* displayinfo : displayinfos_ )
    {
	dispcorrect = displayinfo->visids_.isPresent( displayid );
	if ( dispcorrect )
	    break;
    }

    if ( !dispcorrect )
	return;

    if ( mnuid == createbodymnuitem_.id )
    {
	uiCreateBodyDlg dlg( visserv_.appserv().parent(), *dispprop_ );
	if ( dlg.go() )
	{
	    RefMan<EM::EMObject> emobj =
		    EM::EMM().createTempObject( EM::RandomPosBody::typeStr() );
	    const DataPointSet* data = display->getDataPack();
	    mDynamicCastGet( EM::RandomPosBody*, emps, emobj.ptr() );
	    if ( !emps )
		return;

	    if ( dispprop_->showSelected() )
		emps->copyFrom( *data, dlg.selGrpIdx() );
	    else
		emps->copyFrom( *data, dispprop_->dpsColID(),
				dlg.getValRange() );

	    treeToBeAdded.trigger( emps->id() );
	}
    }
    else if ( mnuid == storepsmnuitem_.id )
    {
	uiCreatePicksDlg dlg( visserv_.appserv().parent(), *dispprop_ );
	if ( !dlg.go() )
	    return;

	RefMan<Pick::Set> pickset = dlg.getPickSet();
	if ( !pickset )
	    return;

	const DataPointSet* data = display->getDataPack();
	for ( int rid=0; rid<data->size(); rid++ )
	{
	    bool useloc = false;
	    if ( dispprop_->showSelected() )
		useloc = data->selGroup(rid) == dlg.selGrpIdx();
	    else
		useloc = dlg.getValRange().includes(
		      data->value(dispprop_->dpsColID(),rid),true);

	    if ( useloc )
		pickset->add( data->coord(rid), data->z(rid) );
	}

	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
	ctio->setName( pickset->name() );

	if ( uiIOObj::fillCtio(*ctio,true) )
	{
	    uiString errmsg;
	    if ( !PickSetTranslator::store(*pickset,ctio->ioobj_,errmsg) )
		uiMSG().error( errmsg );
	}
    }
    else if ( mnuid == removemnuitem_.id )
    {
	display->removeSelections( 0 );
    }
    else if ( mnuid == propmenuitem_.id )
    {
	uiSetSizeDlg dlg( visserv_.appserv().parent(), display );
	dlg.go();
    }
}


void uiVisDataPointSetDisplayMgr::lock()
{
    lock_.lock();
    visserv_.getSceneIds( allsceneids_ );
    for ( const auto& sceneid : allsceneids_ )
	availableviewers_ += ParentID( sceneid.asInt() );
}


void uiVisDataPointSetDisplayMgr::unLock()
{ lock_.unLock(); }


int uiVisDataPointSetDisplayMgr::getNrViewers() const
{
    return allsceneids_.size();
}


const char* uiVisDataPointSetDisplayMgr::getViewerName( int parentidx ) const
{
    ConstRefMan<visBase::DataObject> scene =
				visserv_.getScene( allsceneids_[parentidx] );
    return scene ? scene->name().str() : nullptr;
}


static visSurvey::PointSetDisplay* getPSD( uiVisPartServer& vps,
					   const VisID& visid )
{
    if ( !visid.isValid() )
	return nullptr;

    mDynamicCastGet(visSurvey::PointSetDisplay*,display,vps.getObject(visid))
    return display;
}


DataPointSetDisplayMgr::DispID
uiVisDataPointSetDisplayMgr::getDisplayID( const DataPointSet& dps ) const
{
    for ( const auto* displayinfo : displayinfos_ )
    {
	const TypeSet<VisID>& visids = displayinfo->visids_;
	for ( const auto& visid : visids )
	{
	    ConstRefMan<visSurvey::PointSetDisplay> psd =
						getPSD( visserv_, visid );
	    ConstRefMan<DataPack> dp = psd ? psd->getDataPack() : nullptr;
	    if ( dp && dp->id() == dps.id() )
		return displayinfo->dispid_;
	}
    }

    return DispID::udf();
}


DataPointSetDisplayMgr::DispID
uiVisDataPointSetDisplayMgr::getDisplayID( const VisID& visid ) const
{
    for ( const auto* displayinfo : displayinfos_ )
	if ( displayinfo->visids_.isPresent(visid) )
	    return displayinfo->dispid_;

    return DispID::udf();
}



DataPointSetDisplayMgr::DispID
uiVisDataPointSetDisplayMgr::addDisplay( const TypeSet<ParentID>& parents,
					 const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_
    if ( parents.isEmpty() )
	return DispID::udf();

    const DispID dispid( displayinfos_.isEmpty() ? 0
			: displayinfos_.last()->dispid_.asInt() + 1 );
    PtrMan<DisplayInfo> displayinfo = new DisplayInfo( dispid );
    for ( int idx=0; idx<parents.size(); idx++ )
    {
	if ( !allsceneids_.validIdx(idx) )
	    continue;

	RefMan<visBase::DataObject> scene =
				visserv_.getScene( allsceneids_[idx] );
	if ( !scene )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
					    new visSurvey::PointSetDisplay;
	if ( !display )
	    continue;

	visserv_.addObject( display, SceneID(parents[idx].asInt()), true );
	display->setDispProp( dispprop_ );
	display->setDataPack( dps.id() );
	uiTaskRunner taskrunner( visserv_.appserv().parent() );
	display->update( &taskrunner );

	displayinfo->sceneids_ += allsceneids_[idx];
	displayinfo->visids_ += display->id();
    }

    if ( displayinfo->sceneids_.isEmpty() )
	return DispID::udf();

    displayinfos_ += displayinfo.release();

    return dispid;
}


bool uiVisDataPointSetDisplayMgr::addDisplays( const TypeSet<ParentID>& parents,
					const ObjectSet<DataPointSet>& dpsset,
					TypeSet<DispID>& dispids )
{
    if ( parents.isEmpty() )
	return false;

    const int nrdisplays = dpsset.size();
    if ( !nrdisplays )
	return false;

    if ( nrdisplays == 1 )
    {
	dispids += addDisplay( parents, *dpsset[0] );
	return true;
    }

    ExecutorGroup dispupdatergrp( "Creating PointSet Display" );
    for ( int didx=0; didx<nrdisplays; didx++ )
    {
	const DataPointSet& dps = *dpsset[didx];
	const DispID dispid( displayinfos_.isEmpty() ? 0
			    : displayinfos_.last()->dispid_.asInt() + 1 );
	PtrMan<DisplayInfo> displayinfo = new DisplayInfo( dispid );
	for ( int idx=0; idx<parents.size(); idx++ )
	{
	    RefMan<visSurvey::Scene> scene =
				    visserv_.getScene( allsceneids_[idx] );
	    if ( !scene )
		continue;

	    RefMan<visSurvey::PointSetDisplay> display =
						new visSurvey::PointSetDisplay;
	    if ( !display )
		continue;

	    visserv_.addObject( display, SceneID(parents[idx].asInt()), true );
	    display->setDispProp( dispprop_ );
	    display->setDataPack( dps.id() );
	    dispupdatergrp.add( display->getUpdater() );

	    displayinfo->sceneids_ += allsceneids_[idx];
	    displayinfo->visids_ += display->id();
	}

	if ( displayinfo->sceneids_.isEmpty() )
	    continue;

	displayinfos_ += displayinfo.release();
	dispids += dispid;
    }

    uiTaskRunner taskrunner( visserv_.appserv().parent() );
    return TaskRunner::execute( &taskrunner, dispupdatergrp );
}


uiVisDataPointSetDisplayMgr::DisplayInfo*
uiVisDataPointSetDisplayMgr::getInfo( const DispID& dispid )
{
    for ( auto* displayinfo : displayinfos_ )
	if ( displayinfo->dispid_ == dispid )
	    return displayinfo;

    return nullptr;
}


const uiVisDataPointSetDisplayMgr::DisplayInfo*
uiVisDataPointSetDisplayMgr::getInfo( const DispID& dispid ) const
{
    return mSelf().getInfo( dispid );
}


void uiVisDataPointSetDisplayMgr::turnOn( const DispID& id, bool yn )
{
    DisplayInfo* displayinfo = getInfo( id );
    if ( !displayinfo )
	return;

    for ( const auto& displayid : displayinfo->visids_ )
    {
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( display )
	    display->turnOn( yn );
    }
}


void uiVisDataPointSetDisplayMgr::updateDisplay( const DispID& id,
						 const DataPointSet& dps )
{ updateDisplay( id, availableViewers(), dps ); }


void uiVisDataPointSetDisplayMgr::updateDisplay( const DispID& id,
					     const TypeSet<ParentID>& parents,
					     const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_
    DisplayInfo* displayinfo = getInfo( id );
    if ( !displayinfo )
	return;

    const int idx = displayinfos_.indexOf( displayinfo );
    TypeSet<SceneID> wantedscenes;
    for ( const auto& parent : parents )
	wantedscenes += SceneID( parent.asInt() );

    TypeSet<SceneID> scenestoremove = displayinfo->sceneids_;
    scenestoremove.createDifference( wantedscenes );

    TypeSet<SceneID> scenestoadd = wantedscenes;
    scenestoadd.createDifference( displayinfo->sceneids_ );

    for ( int idy=0; idy<scenestoremove.size(); idy++ )
    {
	RefMan<visSurvey::Scene> scene =
				    visserv_.getScene( allsceneids_[idx] );
	if ( !scene )
	    continue;

	const SceneID sceneid = scenestoremove[idx];
	const int index = displayinfo->sceneids_.indexOf( sceneid );
	const int objid = scene->getFirstIdx( displayinfo->visids_[index] );
	if ( objid >= 0 )
	    scene->removeObject( objid );

	displayinfo->sceneids_.removeSingle( index );
	displayinfo->visids_.removeSingle( index );
    }

    for ( int idy=0; idy<scenestoadd.size(); idy++ )
    {
	const SceneID sceneid = scenestoadd[idy];
	RefMan<visSurvey::Scene> scene = visserv_.getScene( sceneid );
	if ( !scene )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
					new visSurvey::PointSetDisplay;
	if ( !display )
	    continue;

	visserv_.addObject( display, SceneID( parents[idx].asInt() ), true );

	displayinfo->sceneids_ += sceneid;
	displayinfo->visids_ += display->id();
    }

    for ( int idy=0; idy<displayinfo->visids_.size(); idy++ )
    {
	const VisID displayid = displayinfo->visids_[idy];
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( !display )
	    continue;

	display->setDispProp( dispprop_ );
	display->setDataPack( dps.id() );
	uiTaskRunner taskrunner( visserv_.appserv().parent() );
	display->update( &taskrunner );
    }
}


void uiVisDataPointSetDisplayMgr::updateColorsOnly( const DispID& id )
{
    DisplayInfo* displayinfo = getInfo( id );
    if ( !displayinfo )
	return;

    for ( const auto& displayid : displayinfo->visids_ )
    {
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( !display )
	    continue;

	display->setDispProp( dispprop_ );
	display->updateColors();
    }
}


void uiVisDataPointSetDisplayMgr::clearDisplays()
{
    for ( const auto& sceneid : allsceneids_ )
	visserv_.setMoreObjectsToDoHint( sceneid, true );

    while( !displayinfos_.isEmpty() )
	removeDisplay( *displayinfos_.last() );

    for ( const auto& sceneid : allsceneids_ )
	visserv_.setMoreObjectsToDoHint( sceneid, false );
}


void uiVisDataPointSetDisplayMgr::removeDisplay( DisplayInfo& displayinfo )
{
    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const SceneID sceneid = displayinfo.sceneids_[idy];
	RefMan<visSurvey::Scene> scene = visserv_.getScene( sceneid );
	if ( !scene )
	    continue;

	visserv_.removeObject( displayinfo.visids_[idy],
			       displayinfo.sceneids_[idy] );
    }

    displayinfos_ -= &displayinfo;
    delete &displayinfo;
}


void uiVisDataPointSetDisplayMgr::removeDisplay( const DispID& id )
{
    DisplayInfo* displayinfo = getInfo( id );
    if ( displayinfo )
	removeDisplay( *displayinfo );
}


void uiVisDataPointSetDisplayMgr::getIconInfo( BufferString& fnm,
					       BufferString& tooltip ) const
{
    fnm = "picks";
    tooltip = "Show points in 3D scene";
}


// uiVisDataPointSetDisplayMgr::DisplayInfo

uiVisDataPointSetDisplayMgr::DisplayInfo::DisplayInfo( const DispID& dispid )
    : dispid_(dispid)
{
}


uiVisDataPointSetDisplayMgr::DisplayInfo::~DisplayInfo()
{
}
