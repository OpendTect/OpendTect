/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "uihorizonattrib.h"

#include "uicalcpoly2horvol.h"
#include "uicontourtreeitem.h"
#include "uidatapointsetpickdlg.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uiisopachmaker.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodhortreeitem.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uipickpartserv.h"
#include "uistratamp.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "vishorizondisplay.h"
#include "vispicksetdisplay.h"

#include "attribsel.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "odplugin.h"
#include "survinfo.h"


static const char* sKeyContours = "Contours";


mDefODPluginInfo(uiHorizonAttrib)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Horizon-Attribute (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Nanne Hemstra)",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons. "
	"Not to be confused with calculating attributes on horizons.\n"
	"Also, the Stratal Amplitude and Isochron is provided by this plugin, "
	"as well as the writing of flattened cubes" ))
    return &retpi;
}


class uiHorAttribPIMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiHorAttribPIMgr)
public:
			uiHorAttribPIMgr();
			~uiHorAttribPIMgr();

private:

    void		beforeSurveyChange() override { cleanup(); }
    void		dTectMenuChanged() override;
    void		cleanup();

    void		makeStratAmp(CallBacker*);
    void		doFlattened(CallBacker*);
    void		doIsochron(CallBacker*);
    void		doIsochronThruMenu(CallBacker*);
    void		doContours(CallBacker*);
    void		calcPolyVol(CallBacker*);
    void		calcHorVol(CallBacker*);
    void		pickData(CallBacker*);
    void		dataReadyCB(CallBacker*);

    uiVisMenuItemHandler flattenmnuitemhndlr_;
    uiVisMenuItemHandler isochronmnuitemhndlr_;
    uiVisMenuItemHandler contourmnuitemhndlr_;
    uiVisMenuItemHandler horvolmnuitemhndlr_;
    uiVisMenuItemHandler pickdatamnuitemhndlr_;
    uiPickSetPolygonMenuItemHandler polyvolmnuitemhndlr_;

    uiEMDataPointSetPickDlg*	dpspickdlg_	= nullptr;
    uiStratAmpCalc*		stratampdlg_	= nullptr;
};


#define mMkPars(txt,fun) \
    visSurvey::HorizonDisplay::sFactoryKeyword(), \
    *appl().applMgr().visServer(),txt,mCB(this,uiHorAttribPIMgr,fun)

uiHorAttribPIMgr::uiHorAttribPIMgr()
	: uiPluginInitMgr()
	, flattenmnuitemhndlr_(
		mMkPars(tr("Write Flattened cube ..."),doFlattened),"Workflows")
	, isochronmnuitemhndlr_(
		mMkPars(tr("Calculate Isochron ..."),doIsochron),"Workflows")
	, contourmnuitemhndlr_(
		mMkPars(tr("Contour Display"),doContours),"Add",995)
	, horvolmnuitemhndlr_(
		mMkPars(tr("Calculate Volume ..."),calcHorVol),"Workflows")
	, pickdatamnuitemhndlr_(
		mMkPars(tr("Pick Horizon Data ..."),pickData),"Workflows")
	, polyvolmnuitemhndlr_(
		*appl().applMgr().visServer(),tr("Calculate Volume ..."),
		mCB(this,uiHorAttribPIMgr,calcPolyVol),nullptr,996)
{
    init();
    polyvolmnuitemhndlr_.addWhenPickSet( false );
}


uiHorAttribPIMgr::~uiHorAttribPIMgr()
{
    detachAllNotifiers();
}


void uiHorAttribPIMgr::dTectMenuChanged()
{
    uiODMenuMgr& mnumgr = appl().menuMgr();
    uiActionSeparString gridprocstr( "Create Horizon Output" );
    const uiAction* itm = mnumgr.procMnu()->findAction( gridprocstr );
    if ( !itm || !itm->getMenu() ) return;

    uiMenu* mnu = const_cast<uiMenu*>( itm->getMenu() );
    if ( SI().has3D() )
	mnu->insertAction( new uiAction(tr("Stratal Amplitude ..."),
			   mCB(this,uiHorAttribPIMgr,makeStratAmp)) );

    mnu->insertAction( new uiAction(tr("Isochron ..."),
		       mCB(this,uiHorAttribPIMgr,doIsochronThruMenu)) );
}


void uiHorAttribPIMgr::cleanup()
{
    closeAndZeroPtr( stratampdlg_ );
    closeAndZeroPtr( dpspickdlg_ );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    if ( !stratampdlg_ )
    {
	stratampdlg_ = new uiStratAmpCalc( &appl() );
	stratampdlg_->setModal( false );
    }
    else
	stratampdlg_->init();

    stratampdlg_->show();
}


void uiHorAttribPIMgr::doFlattened( CallBacker* )
{
    const int displayid = flattenmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd )
	return;

    appl().applMgr().EMAttribServer()->createHorizonOutput(
			uiEMAttribPartServer::FlattenSingle, hd->getMultiID() );
}


void uiHorAttribPIMgr::doIsochron( CallBacker* )
{
    const int displayid = isochronmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error(tr("Cannot add extra attribute layers"));
	return;
    }

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;
    uiTreeItem* parent = appl().sceneMgr().findItem( displayid );
    if ( !parent ) return;

    uiIsochronMakerDlg dlg( &appl(), hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttrib(),
			     false, nullptr );
    visserv->setSelSpec( displayid, attrid, selspec );
    visserv->setRandomPosData( displayid, attrid, &dlg.getDPS() );
    uiODAttribTreeItem* itm = new uiODEarthModelSurfaceDataTreeItem(
		hd->getObjectID(), 0, typeid(*parent).name() );
    parent->addChild( itm, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    parent->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiHorAttribPIMgr::doIsochronThruMenu( CallBacker* )
{
    uiIsochronMakerBatch dlg( &appl() );
    if ( !dlg.go() )
	return;
}


class uiSelContourAttribDlg : public uiDialog
{ mODTextTranslationClass(uiSelContourAttribDlg)
public:

uiSelContourAttribDlg( uiParent* p, const EM::ObjectID id )
    : uiDialog(p,uiDialog::Setup(tr("Select Attribute to contour"),
				mNoDlgTitle,mNoHelpKey))
{
    const MultiID mid = EM::EMM().getMultiID( id );
    const EM::IOObjInfo eminfo( mid );

    BufferStringSet attrnms;
    attrnms.add( uiContourTreeItem::sKeyZValue() );
    eminfo.getAttribNames( attrnms );

    const uiString lbl = toUiString( eminfo.name() );
    uiListBox::Setup su( OD::ChooseOnlyOne, lbl, uiListBox::AboveMid );
    attrlb_ = new uiListBox( this, su );
    attrlb_->addItems( attrnms );
}

int nrAttribs() const
{
    return attrlb_->size();
}

const char* getAttribName() const
{
    return attrlb_->getText();
}

    uiListBox*	attrlb_;
};


void uiHorAttribPIMgr::doContours( CallBacker* )
{
    const int displayid = contourmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd )
	return;

    const EM::EMObject* emobj = EM::EMM().getObject( hd->getObjectID() );
    mDynamicCastGet(const EM::Horizon3D*,hor,emobj)
    if ( !hor )
    {
	uiMSG().error( tr("Internal: cannot find horizon") );
	return;
    }

    const MultiID mid = EM::EMM().getMultiID( emobj->id() );
    const EM::IOObjInfo eminfo( mid );
    if ( !eminfo.isOK() )
    {
	uiMSG().error(
		tr("Cannot find this horizon in project's database.\n"
		"If this is a newly tracked horizon, please save first\n"
		"before contouring.") );
	return;
    }

    uiSelContourAttribDlg dlg( &appl(), emobj->id() );
    if ( dlg.nrAttribs()>1 && !dlg.go() )
	return;

    uiTreeItem* parent = appl().sceneMgr().findItem( displayid );
    if ( !parent )
	return;

    const uiTreeItem* item = parent->findChild( sKeyContours );
    if ( item )
    {
	mDynamicCastGet(const uiContourTreeItem*,conitm,item);
	if ( conitm )
	    return;
    }

    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error(tr("Cannot add extra attribute layers"));
	return;
    }

    const int attrib = visserv->addAttrib( displayid );
    Attrib::SelSpec spec( sKeyContours, Attrib::SelSpec::cAttribNotSel(),
			  false, 0 );
    spec.setZDomainKey( dlg.getAttribName() );
    spec.setDefString( uiContourTreeItem::sKeyContourDefString() );
    visserv->setSelSpec( displayid, attrib, spec );

    uiContourTreeItem* newitem =
	new uiContourTreeItem( typeid(*parent).name() );
    newitem->setAttribName( dlg.getAttribName() );
    parent->addChild( newitem, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiHorAttribPIMgr::calcPolyVol( CallBacker* )
{
    const int displayid = polyvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid))
    if ( !psd || !psd->getSet() )
	{ pErrMsg("Can't get PickSetDisplay"); return; }

    uiCalcPolyHorVol dlg( &appl(), *psd->getSet() );
    dlg.go();
}


void uiHorAttribPIMgr::calcHorVol( CallBacker* )
{
    const int displayid = horvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    EM::EMObject* emobj = EM::EMM().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) { uiMSG().error(tr("Internal: cannot find horizon")); return; }
    uiCalcHorPolyVol dlg( &appl(), *hor );
    dlg.go();
}


void uiHorAttribPIMgr::pickData( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    delete dpspickdlg_;
    dpspickdlg_ = new uiEMDataPointSetPickDlg( &appl(), hd->getScene()->id(),
					       hd->getObjectID() );
    mAttachCB( dpspickdlg_->readyForDisplay, uiHorAttribPIMgr::dataReadyCB );
    dpspickdlg_->show();
}


void uiHorAttribPIMgr::dataReadyCB( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiTreeItem* parent = appl().sceneMgr().findItem( displayid );
    mDynamicCastGet(uiODHorizonTreeItem*,horitm,parent)
    if ( !horitm )
	return;

    uiODDataTreeItem* itm = horitm->addAttribItem();
    mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm);
    if ( emitm && dpspickdlg_ )
	emitm->setDataPointSet( dpspickdlg_->getData() );
}



mDefODInitPlugin(uiHorizonAttrib)
{
    mDefineStaticLocalObject( PtrMan<uiHorAttribPIMgr>, theinst_,
			    = new uiHorAttribPIMgr() );
    if ( !theinst_ )
	return "Cannot instantiate the HorizonAttrib plugin";

    uiHorizonAttrib::initClass();
    uiContourTreeItem::initClass();

    return nullptr;
}
