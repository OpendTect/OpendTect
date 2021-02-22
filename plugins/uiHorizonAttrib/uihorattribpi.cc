/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uihorizonattrib.h"
#include "uicontourtreeitem.h"
#include "uidatapointsetpickdlg.h"
#include "uiempartserv.h"
#include "uistratamp.h"
#include "uiflattenedcube.h"
#include "uiisopachmaker.h"
#include "uicalcpoly2horvol.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodhortreeitem.h"
#include "vishorizondisplay.h"
#include "vispicksetdisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "uipickpartserv.h"
#include "attribsel.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "emhorizon3d.h"
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
	"dGB (Nanne Hemstra)",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons. "
	"Not to be confused with calculating attributes on horizons.\n"
	"Also, the Stratal Amplitude and Isochron is provided by this plugin, "
	"as well as the writing of flattened cubes" ) )
    return &retpi;
}


class uiHorAttribPIMgr :  public CallBacker
{ mODTextTranslationClass(uiHorAttribPIMgr)
public:
			uiHorAttribPIMgr(uiODMain*);
			~uiHorAttribPIMgr();

    void		cleanUp();
    void		updateMenuCB(CallBacker*);
    void		shutDownCB(CallBacker*);
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

    uiODMain*			appl_;
    uiEMDataPointSetPickDlg*	dpspickdlg_;
    uiStratAmpCalc*		stratampdlg_;
};


#define mMkPars(txt,fun) \
    visSurvey::HorizonDisplay::sFactoryKeyword(), \
    *a->applMgr().visServer(),txt,mCB(this,uiHorAttribPIMgr,fun)

uiHorAttribPIMgr::uiHorAttribPIMgr( uiODMain* a )
	: appl_(a)
	, dpspickdlg_(0)
	, stratampdlg_(0)
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
		*a->applMgr().visServer(),tr("Calculate Volume ..."),
		mCB(this,uiHorAttribPIMgr,calcPolyVol),0,996)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mAttachCB( mnumgr.dTectMnuChanged, uiHorAttribPIMgr::updateMenuCB );
    mAttachCB( IOM().applicationClosing, uiHorAttribPIMgr::shutDownCB );
    updateMenuCB(0);

    polyvolmnuitemhndlr_.addWhenPickSet( false );
}


uiHorAttribPIMgr::~uiHorAttribPIMgr()
{
    detachAllNotifiers();
    delete stratampdlg_;
    delete dpspickdlg_;
}


void uiHorAttribPIMgr::updateMenuCB( CallBacker* )
{
    cleanUp();
    uiODMenuMgr& mnumgr = appl_->menuMgr();
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


void uiHorAttribPIMgr::shutDownCB( CallBacker* )
{ cleanUp(); }

void uiHorAttribPIMgr::cleanUp()
{
    deleteAndZeroPtr( stratampdlg_ );
    deleteAndZeroPtr( dpspickdlg_ );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    if ( !stratampdlg_ )
    {
	stratampdlg_ = new uiStratAmpCalc( appl_ );
	stratampdlg_->setModal( false );
    }
    else
	stratampdlg_->init();

    stratampdlg_->show();
}


void uiHorAttribPIMgr::doFlattened( CallBacker* )
{
    const int displayid = flattenmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    uiWriteFlattenedCube dlg( appl_, hd->getObjectID() );
    dlg.go();
}


void uiHorAttribPIMgr::doIsochron( CallBacker* )
{
    const int displayid = isochronmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error(tr("Cannot add extra attribute layers"));
	return;
    }

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent ) return;

    uiIsochronMakerDlg dlg( appl_, hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttrib(),
			     false, 0 );
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
    uiIsochronMakerBatch dlg( appl_ );
    if ( !dlg.go() )
	return;
}


class uiSelContourAttribDlg : public uiDialog
{ mODTextTranslationClass(uiSelContourAttribDlg);
public:

uiSelContourAttribDlg( uiParent* p, const EM::ObjectID& id )
    : uiDialog(p,uiDialog::Setup(tr("Select Attribute to contour"),
				mNoDlgTitle,mNoHelpKey))
{
    const MultiID mid = EM::EMM().getMultiID( id );
    PtrMan<IOObj> emioobj = IOM().get( mid );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    attrnms.add( uiContourTreeItem::sKeyZValue() );
    eminfo.getAttribNames( attrnms );

    const uiString lbl = toUiString( emioobj->name() );
    uiListBox::Setup su( OD::ChooseOnlyOne, lbl, uiListBox::AboveMid );
    attrlb_ = new uiListBox( this, su );
    attrlb_->addItems( attrnms );
}

int nrAttribs() const { return attrlb_->size(); }

const char* getAttribName() const
{ return attrlb_->getText(); }

uiListBox*	attrlb_;
};


void uiHorAttribPIMgr::doContours( CallBacker* cb )
{
    const int displayid = contourmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    EM::EMObject* emobj = EM::EMM().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) { uiMSG().error(tr("Internal: cannot find horizon")); return; }

    uiSelContourAttribDlg dlg( appl_, emobj->id() );
    if ( dlg.nrAttribs()>1 && !dlg.go() )
	return;

    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
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
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid))
    if ( !psd || !psd->getSet() )
	{ pErrMsg("Can't get PickSetDisplay"); return; }

    uiCalcPolyHorVol dlg( appl_, *psd->getSet() );
    dlg.go();
}


void uiHorAttribPIMgr::calcHorVol( CallBacker* )
{
    const int displayid = horvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    EM::EMObject* emobj = EM::EMM().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) { uiMSG().error(tr("Internal: cannot find horizon")); return; }
    uiCalcHorPolyVol dlg( appl_, *hor );
    dlg.go();
}


void uiHorAttribPIMgr::pickData( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    delete dpspickdlg_;
    dpspickdlg_ = new uiEMDataPointSetPickDlg( appl_, hd->getScene()->id(),
					       hd->getObjectID() );
    dpspickdlg_->readyForDisplay.notify(
				mCB(this,uiHorAttribPIMgr,dataReadyCB) );
    dpspickdlg_->show();
}


void uiHorAttribPIMgr::dataReadyCB( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
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
    mDefineStaticLocalObject( PtrMan<uiHorAttribPIMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiHorAttribPIMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate HorizonAttrib plugin";

    uiHorizonAttrib::initClass();
    uiContourTreeItem::initClass();

    return 0;
}
