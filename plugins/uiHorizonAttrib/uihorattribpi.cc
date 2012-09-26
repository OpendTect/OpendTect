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
#include "uiodemsurftreeitem.h"
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
    static PluginInfo retpi = {
	"Horizon-Attribute",
	"dGB - Nanne Hemstra",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons.\n"
    	"Not to be confused with calculating attributes on horizons.\n"
        "It can even be useful to apply the 'Horizon' attribute on horizons.\n"
        "Also, the Stratal Amplitude is provided by this plugin,\n"
	"and the writing of flattened cubes" };
    return &retpi;
}


class uiHorAttribPIMgr :  public CallBacker
{
public:
			uiHorAttribPIMgr(uiODMain*);

    void		updateMenu(CallBacker*);
    void		makeStratAmp(CallBacker*);
    void		doFlattened(CallBacker*);
    void		doIsopach(CallBacker*);
    void		doIsopachThruMenu(CallBacker*);
    void		doContours(CallBacker*);
    void		calcPolyVol(CallBacker*);
    void		calcHorVol(CallBacker*);

    uiODMain*		appl_;
    uiVisMenuItemHandler flattenmnuitemhndlr_;
    uiVisMenuItemHandler isopachmnuitemhndlr_;
    uiVisMenuItemHandler contourmnuitemhndlr_;
    uiVisMenuItemHandler horvolmnuitemhndlr_;
    uiVisMenuItemHandler polyvolmnuitemhndlr_;
};


#define mMkPars(txt,fun) \
    visSurvey::HorizonDisplay::getStaticClassName(), \
    *a->applMgr().visServer(),txt,mCB(this,uiHorAttribPIMgr,fun)

uiHorAttribPIMgr::uiHorAttribPIMgr( uiODMain* a )
	: appl_(a)
    	, flattenmnuitemhndlr_(
		mMkPars("Write &Flattened cube ...",doFlattened),"Workflows")
    	, isopachmnuitemhndlr_(
		mMkPars("Calculate &Isopach ...",doIsopach),"Workflows")
	, contourmnuitemhndlr_(
		mMkPars("&Contour Display",doContours),"Add",995)
    	, horvolmnuitemhndlr_(
		mMkPars("Calculate &Volume ...",calcHorVol),"Workflows")
	, polyvolmnuitemhndlr_(visSurvey::PickSetDisplay::getStaticClassName(),
		*a->applMgr().visServer(),"Calculate &Volume ...",
		mCB(this,uiHorAttribPIMgr,calcPolyVol),0,996)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.dTectMnuChanged.notify(mCB(this,uiHorAttribPIMgr,updateMenu));
    updateMenu(0);
}


void uiHorAttribPIMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    MenuItemSeparString gridprocstr( "Create &Horizon Output" );
    uiMenuItem* itm = mnumgr.procMnu()->find( gridprocstr );
    if ( !itm ) return;

    mDynamicCastGet(uiPopupItem*,gridpocitm,itm)
    if ( !gridpocitm ) return;

    if ( SI().has3D() )
	gridpocitm->menu().insertItem( new uiMenuItem("&Stratal Amplitude ...",
				     mCB(this,uiHorAttribPIMgr,makeStratAmp)) );

    gridpocitm->menu().insertItem( new uiMenuItem("&Isopach ...",
			    mCB(this,uiHorAttribPIMgr,doIsopachThruMenu)) );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    uiStratAmpCalc dlg( appl_ );
    dlg.go();
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


void uiHorAttribPIMgr::doIsopach( CallBacker* )
{
    const int displayid = isopachmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error( "Cannot add extra attribute layers" );
	return;
    }

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent ) return;

    uiIsopachMakerDlg dlg( appl_, hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttrib(),
	    		     false, 0 );
    visserv->setSelSpec( displayid, attrid, selspec );
    visserv->createAndDispDataPack( displayid, attrid, &dlg.getDPS() );
    uiODAttribTreeItem* itm = new uiODEarthModelSurfaceDataTreeItem(
	    	hd->getObjectID(), 0, typeid(*parent).name() );
    parent->addChild( itm, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    parent->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiHorAttribPIMgr::doIsopachThruMenu( CallBacker* )
{
    uiIsopachMakerBatch dlg( appl_ );
    if ( !dlg.go() )
	return;
}


class uiSelContourAttribDlg : public uiDialog
{
public:

uiSelContourAttribDlg( uiParent* p, const EM::ObjectID& id )
    : uiDialog(p,uiDialog::Setup("Select Attribute to contour","",""))
{
    const MultiID mid = EM::EMM().getMultiID( id );
    PtrMan<IOObj> emioobj = IOM().get( mid );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    attrnms.add( "ZValue" );
    eminfo.getAttribNames( attrnms );
    uiLabeledListBox* llb =
	new uiLabeledListBox( this, attrnms, emioobj->name(), 
			      false, uiLabeledListBox::AboveMid );
    attrlb_ = llb->box();
}

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
    if ( !hor ) { uiMSG().error("Internal: cannot find horizon"); return; }

    uiSelContourAttribDlg dlg( appl_, emobj->id() );
    if ( !dlg.go() )
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
	uiMSG().error( "Cannot add extra attribute layers" );
	return;
    }

    const int attrib = visserv->addAttrib( displayid );
    Attrib::SelSpec spec( sKeyContours, Attrib::SelSpec::cAttribNotSel(),
	    		  false, 0 );
    spec.setDefString( uiContourTreeItem::sKeyContourDefString() );
    visserv->setSelSpec( displayid, attrib, spec );

    uiContourTreeItem* newitem = new uiContourTreeItem(typeid(*parent).name() );
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
    if ( !hor ) { uiMSG().error("Internal: cannot find horizon"); return; }
    uiCalcHorPolyVol dlg( appl_, *hor );
    dlg.go();
}


mDefODInitPlugin(uiHorizonAttrib)
{
    uiHorizonAttrib::initClass();
    uiContourTreeItem::initClass();
    static uiHorAttribPIMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiHorAttribPIMgr( ODMainWin() );

    return 0;
}
