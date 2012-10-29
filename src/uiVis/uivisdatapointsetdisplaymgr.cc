/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uivisdatapointsetdisplaymgr.h"

#include "uimenuhandler.h"
#include "uivispartserv.h"
#include "uimaterialdlg.h"
#include "uicombobox.h"
#include "uicreatepicks.h"
#include "uigeninput.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "uislider.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "emrandomposbody.h"
#include "emmanager.h"
#include "pickset.h"
#include "picksettr.h"
#include "pixmap.h"
#include "visdata.h"
#include "visrandomposbodydisplay.h"
#include "vissurvscene.h"
#include "vispointsetdisplay.h"
#include "vispointset.h"

uiVisDataPointSetDisplayMgr::uiVisDataPointSetDisplayMgr(uiVisPartServer& serv )
    : DataPointSetDisplayMgr()
    , visserv_( serv )
    , vismenu_( visserv_.getMenuHandler() )
    , createbodymnuitem_( "Create Body" )
    , storepsmnuitem_( "Save as Pickset ..." )
    , removemnuitem_( "Remove points inside polygon" )
    , propmenuitem_( "Properites.." )
    , treeToBeAdded( this )
{
    vismenu_->ref();
    vismenu_->createnotifier.notify(
	    mCB(this,uiVisDataPointSetDisplayMgr,createMenuCB) );
    vismenu_->handlenotifier.notify(
	    mCB(this,uiVisDataPointSetDisplayMgr,handleMenuCB) );
}


uiVisDataPointSetDisplayMgr::~uiVisDataPointSetDisplayMgr()
{
    if ( vismenu_ )
    {
	vismenu_->createnotifier.remove(
		mCB(this,uiVisDataPointSetDisplayMgr,createMenuCB) );
	vismenu_->handlenotifier.remove(
		mCB(this,uiVisDataPointSetDisplayMgr,handleMenuCB) );
	vismenu_->unRef();
    }

    deepErase( displayinfos_ );
}


class uiSetSizeDlg : public uiDialog
{
public:
uiSetSizeDlg( uiParent * p, visSurvey::PointSetDisplay* disp )
    : uiDialog( p, uiDialog::Setup("Set size of points","","") )
    , pointsetdisp_(disp)
{
    setCtrlStyle( uiDialog::LeaveOnly );
    const float fsz = (float)pointsetdisp_->getPointSize();
    slider_ = new uiSliderExtra( this, uiSliderExtra::Setup("Size"), "Size" );
    slider_->sldr()->setInterval( StepInterval<float>(fsz-10.0f,
							   fsz+10.0f, 1.0f) );
    slider_->sldr()->setMinValue( 1 );
    slider_->sldr()->setMaxValue( 15 );
    slider_->sldr()->setValue( fsz );
    slider_->sldr()->setTickMarks( uiSlider::Below );
    slider_->sldr()->setTickStep( 1 );
    slider_->sldr()->sliderMoved.notify( mCB(this,uiSetSizeDlg,sizeChangedCB) );
}

void sizeChangedCB( CallBacker* )
{
    pointsetdisp_->setPointSize( slider_->sldr()->getIntValue() );
}

    uiSliderExtra*		slider_;
    visSurvey::PointSetDisplay* pointsetdisp_;
};


void uiVisDataPointSetDisplayMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !menu ) return;

    const int displayid = menu->menuID();
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;
    
    menu->removeItems();

    bool dispcorrect = false;
    for ( int idx=0; idx<displayinfos_.size(); idx++ )
    {
	const TypeSet<int> visids = displayinfos_[idx]->visids_;
	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    if ( visids[idy] == displayid )
		dispcorrect = true;
	}
    }

    if ( !dispcorrect ) return;

    mAddMenuItem( menu, &propmenuitem_, true, false );
    propmenuitem_.iconfnm = "disppars.png";
    mAddMenuItem( menu, &createbodymnuitem_, true, false );
    mAddMenuItem( menu, &storepsmnuitem_, true, false );
    mAddMenuItem( menu, &removemnuitem_, true, false );
}


class uiCreateBodyDlg : public uiDialog
{
public:
uiCreateBodyDlg( uiParent* p, const DataPointSetDisplayProp& dispprop )
    : uiDialog(p,uiDialog::Setup("Body Creation","Create new Body",mNoHelpID))
    , selfld_( 0 )
    , rgfld_( 0 )
{
    if ( dispprop.showSelected() )
    {
	uiLabeledComboBox* cbx = new uiLabeledComboBox(this,"Selection Group");
	selfld_ = cbx->box();
	BufferStringSet selgrpnms = dispprop.selGrpNames();
	TypeSet<Color> selgrpcols = dispprop.selGrpColors();
	for ( int idx=0; idx<selgrpnms.size(); idx++ )
	{
	    selfld_->addItem( selgrpnms[0]->buf() );
	    ioPixmap pixmap( 20, 20 );
	    Color col = selgrpcols[ idx ];
	    pixmap.fill( col );
	    selfld_->setPixmap( pixmap, idx );
	}
    }
    else
    {
	rgfld_ = new uiGenInput( this, "Create body from value range",
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

    uiComboBox*		selfld_;
    uiGenInput*		rgfld_;
};


class uiCreatePicksDlg : public uiCreatePicks
{
public:

uiCreatePicksDlg( uiParent* p, const DataPointSetDisplayProp& dispprop )
    : uiCreatePicks( p )
    , selfld_( 0 )
    , rgfld_( 0 )
{
    if ( dispprop.showSelected() )
    {
	uiLabeledComboBox* cbx = new uiLabeledComboBox(this,"Selection Group");
	selfld_ = cbx->box();
	BufferStringSet selgrpnms = dispprop.selGrpNames();
	TypeSet<Color> selgrpcols = dispprop.selGrpColors();
	for ( int idx=0; idx<selgrpnms.size(); idx++ )
	{
	    selfld_->addItem( selgrpnms[0]->buf() );
	    ioPixmap pixmap( 20, 20 );
	    Color col = selgrpcols[ idx ];
	    pixmap.fill( col );
	    selfld_->setPixmap( pixmap, idx );
	}

	cbx->attach( alignedAbove, nmfld_ );
    }
    else
    {
	rgfld_ = new uiGenInput( this, "Create body from value range",
				 FloatInpIntervalSpec(false) );
	rgfld_->setValue( dispprop.colMapperSetUp().range_ );
	rgfld_->attach( alignedAbove, nmfld_ );
    }
}

int selGrpIdx() const
{ return selfld_ ? selfld_->currentItem() : -1; }

Interval<float> getValRange() const
{
    return rgfld_ ? rgfld_->getFInterval()
		  : Interval<float>(mUdf(float),mUdf(float));
}

    uiComboBox*		selfld_;
    uiGenInput*		rgfld_;
};


void uiVisDataPointSetDisplayMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 ) return;
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( !menu ) return;

    const int displayid = menu->menuID();
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;

    bool dispcorrect = false;
    for ( int idx=0; idx<displayinfos_.size(); idx++ )
    {
	const TypeSet<int> visids = displayinfos_[idx]->visids_;
	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    if ( visids[idy] == displayid )
		dispcorrect = true;
	}
    }

    if ( !dispcorrect ) return;

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

	Pick::Set& pickset = *dlg.getPickSet();

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
		pickset +=
		    Pick::Location( Coord3(data->coord(rid),data->z(rid)) );
	}

	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
	ctio->setName( pickset.name() );

	if ( uiIOObj::fillCtio(*ctio,true) )
	{
	    BufferString bs;
	    if ( !PickSetTranslator::store( pickset, ctio->ioobj, bs ) )
	    uiMSG().error(bs);
	}
    }
    else if ( mnuid == removemnuitem_.id )
    {
	visSurvey::Scene* scene = display->getScene();
	if ( !scene || !scene->getSelector() )
	    return;
	display->removeSelection( *scene->getSelector() );
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
    visserv_.getChildIds( -1, allsceneids_ );
    availableviewers_ = allsceneids_;
}


void uiVisDataPointSetDisplayMgr::unLock()
{ lock_.unLock(); }


int uiVisDataPointSetDisplayMgr::getNrViewers() const
{
    return allsceneids_.size();
}


const char* uiVisDataPointSetDisplayMgr::getViewerName( int parentidx ) const
{
    RefMan<visBase::DataObject> scene =
	visserv_.getObject( allsceneids_[parentidx] );
    return scene ? scene->name() : 0;
}


static visSurvey::PointSetDisplay* getPSD( uiVisPartServer& vps, int visid )
{
    if ( visid < 0 )
	return 0;

    mDynamicCastGet(visSurvey::PointSetDisplay*,display,vps.getObject(visid))
    return display;
}


int uiVisDataPointSetDisplayMgr::getDisplayID( const DataPointSet& dps ) const
{
    for ( int idx=0; idx<displayinfos_.size(); idx++ )
    {
	const TypeSet<int>& visids = displayinfos_[idx]->visids_;
	for ( int visidx=0; visidx<visids.size(); visidx++ )
	{
	    visSurvey::PointSetDisplay* psd = getPSD( visserv_, visids[visidx]);
	    if ( psd && psd->getDataPack()->id() == dps.id() )
		return ids_[idx];
	}
    }

    return -1;
}


int uiVisDataPointSetDisplayMgr::addDisplay(const TypeSet<int>& parents,
					    const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_
    if ( !parents.size() )
	return -1;

    DisplayInfo* displayinfo = new DisplayInfo;
    if ( !displayinfo )
	return -1;

    int id = 0;
    while ( ids_.indexOf(id)!=-1 ) id++;

    for ( int idx=0; idx<parents.size(); idx++ )
    {
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.addObject( display, parents[idx], true );
	display->setDispProp( dispprop_ );
	display->setDataPack( dps.id() );

	displayinfo->sceneids_ += allsceneids_[idx];
	displayinfo->visids_ += display->id();
    }

    if ( !displayinfo->sceneids_.size() )
    {
	delete displayinfo;
	return -1;
    }

    displayinfos_ += displayinfo;
    ids_ += id;

    return id;
}


void uiVisDataPointSetDisplayMgr::updateDisplay( DispID id,
						 const DataPointSet& dps )
{ updateDisplay( id, availableViewers(), dps ); }


void uiVisDataPointSetDisplayMgr::updateDisplay( DispID id,
						 const TypeSet<int>& parents,
						 const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_

    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    TypeSet<int> wantedscenes;
    for ( int idy=0; idy<parents.size(); idy++ )
	wantedscenes += parents[idy];

    TypeSet<int> scenestoremove = displayinfo.sceneids_;
    scenestoremove.createDifference( wantedscenes );

    TypeSet<int> scenestoadd = wantedscenes;
    scenestoadd.createDifference( displayinfo.sceneids_ );

    for ( int idy=0; idy<scenestoremove.size(); idy++ )
    {
	const int sceneid = scenestoremove[idx];
	const int index = displayinfo.sceneids_.indexOf( sceneid );
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	const int objid = scene->getFirstIdx( displayinfo.visids_[index] );
	if ( objid >= 0 )
	    scene->removeObject( objid );

	displayinfo.sceneids_.removeSingle( index );
	displayinfo.visids_.removeSingle( index );
    }

    for ( int idy=0; idy<scenestoadd.size(); idy++ )
    {
	const int sceneid = scenestoadd[idy];
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.addObject( display, parents[idx], true );

	displayinfo.sceneids_ += sceneid;
	displayinfo.visids_ += display->id();
    }

    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int displayid = displayinfo.visids_[idy];
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( !display )
	    continue;

	display->setDispProp( dispprop_ );
	display->setDataPack( dps.id() );
    }
}


void uiVisDataPointSetDisplayMgr::clearDisplays()
{
    DataPackMgr& dpm = DPM( DataPackMgr::PointID() );
    for ( int dpidx=0; dpidx<dpm.packs().size(); dpidx++ )
    {
	const DataPack* datapack = dpm.packs()[dpidx];
	mDynamicCastGet(const DataPointSet*,dps,datapack);
	if ( !dps ) continue;
	DispID id = getDisplayID( *dps );
	removeDisplay( id );
    }
}


void uiVisDataPointSetDisplayMgr::removeDisplay( DispID id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int sceneid = displayinfo.sceneids_[idy];
	RefMan<visBase::DataObject> sceneptr = visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.removeObject( displayinfo.visids_[idy],
			       displayinfo.sceneids_[idy] );
    }

    ids_.removeSingle( idx );
    delete displayinfos_.removeSingle( idx );
}


void uiVisDataPointSetDisplayMgr::getIconInfo( BufferString& fnm,
					       BufferString& tooltip ) const
{
    fnm = "picks";
    tooltip = "Show points in 3D scene";
}
