/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "uiodhorattribmgr.h"
#include "uiodcontourtreeitem.h"
#include "uidatapointsetpickdlg.h"
#include "uiempartserv.h"
#include "uistratamp.h"
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
#include "uivispartserv.h"
#include "uipickpartserv.h"
#include "attribsel.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "emhorizon3d.h"
#include "ioobj.h"
#include "odplugin.h"
#include "survinfo.h"


static const char* sKeyContours = "Contours";


#define mMkPars(txt,fun) \
    visSurvey::HorizonDisplay::sFactoryKeyword(), \
    *a->applMgr().visServer(),txt,mCB(this,uiODHorAttribMgr,fun)

uiODHorAttribMgr::uiODHorAttribMgr( uiODMain* a )
	: appl_(a)
	, dpspickdlg_(0)
	, isochronmnuitemhndlr_(
		mMkPars(m3Dots(tr("Calculate %1")
		.arg(uiStrings::sIsoMapType(SI().zIsTime()))),doIsochron),
		"Workflows")
	, contourmnuitemhndlr_(
		mMkPars(tr("Contour Display"),doContours),"Add",995)
	, horvolmnuitemhndlr_(
		mMkPars(m3Dots(tr("Calculate Volume")),calcHorVol),"Workflows")
	, pickdatamnuitemhndlr_(
		mMkPars(m3Dots(tr("Pick Horizon Data")),pickData),"Workflows")
	, polyvolmnuitemhndlr_(
		*a->applMgr().visServer(),m3Dots(tr("Calculate Volume")),
		mCB(this,uiODHorAttribMgr,calcPolyVol),0,996)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mAttachCB( mnumgr.dTectMnuChanged, uiODHorAttribMgr::updateMenu );
    updateMenu(0);

    polyvolmnuitemhndlr_.addWhenPickSet( false );
}


uiODHorAttribMgr::~uiODHorAttribMgr()
{
    detachAllNotifiers();
}


void uiODHorAttribMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    uiActionSeparString gridprocstr( "Create Horizon Output" );
    uiMenu* mnu = mnumgr.createHorOutputMenu();

    if ( SI().has3D() )
	mnu->insertAction( new uiAction(m3Dots(tr("Stratal Amplitude")),
		    mCB(this,uiODHorAttribMgr,makeStratAmp), "stratalampl" ));

    mnu->insertAction( new uiAction(
		m3Dots(uiStrings::sIsoMapType(SI().zIsTime())),
		mCB(this,uiODHorAttribMgr,doIsochronThruMenu), "isochron") );
}


void uiODHorAttribMgr::makeStratAmp( CallBacker* )
{
    uiStratAmpCalc dlg( appl_ );
    dlg.go();
}



#define mUiMsg() gUiMsg()

void uiODHorAttribMgr::doIsochron( CallBacker* )
{
    const int displayid = isochronmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
	{ mUiMsg().error(tr("Cannot add extra attribute layers")); return; }

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd )
	return;
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent )
	return;

    uiIsochronMakerDlg dlg( appl_, hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttribID(),
			     false, 0 );
    visserv->setSelSpec( displayid, attrid, selspec );
    visserv->setRandomPosData( displayid, attrid, &dlg.getDPS() );
    uiODAttribTreeItem* itm = new uiODEarthModelSurfaceDataTreeItem(
		hd->getObjectID(), 0, typeid(*parent).name() );
    parent->addChild( itm, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    parent->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODHorAttribMgr::doIsochronThruMenu( CallBacker* )
{
    uiIsochronMakerBatch dlg( appl_ );
    if ( !dlg.go() )
	return;
}


class uiSelContourAttribDlg : public uiDialog
{ mODTextTranslationClass(uiSelContourAttribDlg)
public:

uiSelContourAttribDlg( uiParent* p, const DBKey& id )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
	       tr("Attribute to contour")), mNoDlgTitle, mNoHelpKey))
{
    EM::IOObjInfo eminfo( id );
    BufferStringSet attrnms;
    attrnms.add( uiODContourTreeItem::sKeyZValue() );
    eminfo.getAttribNames( attrnms );

    const uiString lbl = toUiString( id.name() );
    uiListBox::Setup su( OD::ChooseOnlyOne, lbl, uiListBox::AboveMid );
    attrlb_ = new uiListBox( this, su );
    attrlb_->addItems( attrnms );
}

int nrAttribs() const
{
    return attrlb_->size();
}

const char* attribName() const
{
    return attrlb_->getText();
}

    uiListBox*	attrlb_;
};


void uiODHorAttribMgr::doContours( CallBacker* cb )
{
    const int displayid = contourmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd )
	return;

    EM::Object* emobj = EM::MGR().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor )
	{ mUiMsg().error(tr("Internal: cannot find horizon")); return; }

    uiSelContourAttribDlg dlg( appl_, emobj->id() );
    if ( dlg.nrAttribs()>1 && !dlg.go() )
	return;

    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent )
	return;

    const uiTreeItem* item = parent->findChild( sKeyContours );
    if ( item )
    {
	mDynamicCastGet( const uiODContourTreeItem*, conitm, item );
	if ( conitm )
	    return;
    }

    if ( !visserv->canAddAttrib(displayid) )
	{ mUiMsg().error(tr("Cannot add extra attribute layers")); return; }

    const int attrib = visserv->addAttrib( displayid );
    Attrib::SelSpec spec( sKeyContours, Attrib::SelSpec::cAttribNotSelID(),
			  false, 0 );
    spec.setZDomainKey( dlg.attribName() );
    spec.setDefString( uiODContourTreeItem::sKeyContourDefString() );
    visserv->setSelSpec( displayid, attrib, spec );

    uiODContourTreeItem* newitem =
	new uiODContourTreeItem( typeid(*parent).name() );
    newitem->setAttribName( dlg.attribName() );
    parent->addChild( newitem, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiODHorAttribMgr::calcPolyVol( CallBacker* )
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


void uiODHorAttribMgr::calcHorVol( CallBacker* )
{
    const int displayid = horvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    EM::Object* emobj = EM::MGR().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor )
	{ mUiMsg().error(tr("Internal: cannot find horizon")); return; }
    uiCalcHorPolyVol dlg( appl_, *hor );
    dlg.go();
}


void uiODHorAttribMgr::pickData( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    delete dpspickdlg_;
    dpspickdlg_ = new uiEMDataPointSetPickDlg( appl_, hd->getScene()->id(),
					       hd->getObjectID() );
    dpspickdlg_->readyForDisplay.notify(
				mCB(this,uiODHorAttribMgr,dataReadyCB) );
    dpspickdlg_->show();
}


void uiODHorAttribMgr::dataReadyCB( CallBacker* )
{
    const int displayid = pickdatamnuitemhndlr_.getDisplayID();
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    mDynamicCastGet(uiODHorizonTreeItem*,horitm,parent)
    if ( !horitm )
        return;

    uiODDataTreeItem* itm = horitm->addAttribItem();
    mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm);
    if ( emitm ) emitm->setDataPointSet( dpspickdlg_->getData() );
}
