/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodattribtreeitem.h"

#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uiattribpartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vishorizondisplay.h"
#include "vissurvobj.h"
#include "vissurvscene.h"



uiString uiODAttribTreeItem::sKeySelAttribMenuTxt()
{ return uiStrings::sSelAttrib(); }

uiString uiODAttribTreeItem::sKeyColSettingsMenuTxt()
{ return tr("Save Color Settings"); }

uiString uiODAttribTreeItem::sKeyUseColSettingsMenuTxt()
{ return tr("Use Saved Color Settings"); }


uiODAttribTreeItem::uiODAttribTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selattrmnuitem_( sKeySelAttribMenuTxt() )
    , colsettingsmnuitem_( sKeyColSettingsMenuTxt() )
    , usecolsettingsmnuitem_( sKeyUseColSettingsMenuTxt() )
{}


uiODAttribTreeItem::~uiODAttribTreeItem()
{}


bool uiODAttribTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->canSetColTabSequence( displayID() ) &&
	 !visserv->getColTabMapperSetup( displayID(), attribNr() ) )
	return false;
//  if ( !visserv->isClassification( displayID(), attribNr() ) )
    applMgr()->updateColorTable( displayID(), attribNr() );

    return true;
}


#define mCreateDepthDomMnuItemIfNeeded( is2d, needext ) \
{\
    if ( scene && scene->getZAxisTransform() ) \
    {\
	subitem = attrserv->zDomainAttribMenuItem( *as,\
	    scene->zDomainInfo(), is2d, needext );\
	if ( subitem ) \
	    mAddMenuItem(&mnu,subitem,subitem->nrItems(),subitem->checked);\
    }\
}


#define mCreateItemsList( is2d, needext ) \
{ \
    if ( cantransform ) \
    { \
	subitem = attrserv->storedAttribMenuItem( *as, is2d, false ); \
	mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
	subitem = attrserv->calcAttribMenuItem( *as, is2d, needext ); \
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
	subitem = attrserv->nlaAttribMenuItem( *as, is2d, needext ); \
	if ( subitem && subitem->nrItems() ) \
	    mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
	subitem = attrserv->storedAttribMenuItem( *as, is2d, true ); \
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    } \
    mCreateDepthDomMnuItemIfNeeded( is2d, needext ); \
}


void uiODAttribTreeItem::createSelMenu( MenuItem& mnu, VisID visid, int attrib,
					SceneID sceneid )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( as && visserv->hasAttrib(visid) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(visid));
	if ( !so ) return;

	Pol2D3D p2d3d = so->getAllowedDataType();
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid));

	const bool needtransform = !scene->zDomainInfo().def_.isSI();
	const bool cantransform = !needtransform || scene->getZAxisTransform();

	bool need2dlist = SI().has2D() && p2d3d != Only3D;
	bool need3dlist = SI().has3D() && p2d3d != Only2D;

	MenuItem* subitem;
	attrserv->resetMenuItems();
	if ( need3dlist )
	    mCreateItemsList( false, need2dlist );
	if ( need2dlist && p2d3d != Only3D )
	    mCreateItemsList( true, need3dlist );
    }
}


void uiODAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    bool isonly2d = false;
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(sceneID()));
    if ( so ) isonly2d = so->getAllowedDataType() == Only2D;

    if ( !istb )
    {
	selattrmnuitem_.removeItems();
	createSelMenu( selattrmnuitem_, displayID(), attribNr(), sceneID() );
    }

    if ( selattrmnuitem_.nrItems() || isonly2d )
    {
	mAddMenuOrTBItem( istb, 0, menu, &selattrmnuitem_,
		      !visserv->isLocked(displayID()), false );
    }

    const uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    PtrMan<IOObj> ioobj = as ? attrserv->getIOObj(*as) : 0;
    if ( as && ioobj )
    {
	mAddMenuOrTBItem( istb, 0, menu, &colsettingsmnuitem_, true, false );
	mAddMenuOrTBItem( istb, 0, menu, &usecolsettingsmnuitem_, true, false );
    }

    uiODDataTreeItem::createMenu( menu, istb );
}


void uiODAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == colsettingsmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->saveDefColTab( displayID(), attribNr() );
    }
    else if ( mnuid == usecolsettingsmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->useDefColTab( displayID(), attribNr() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( handleSelMenu( mnuid, displayID(), attribNr()) )
    {
	menu->setIsHandled(true);
	applMgr()->useDefColTab( displayID(), attribNr() );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
}


bool uiODAttribTreeItem::handleSelMenu( int mnuid, VisID visid, int attrib )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==-1 || visserv->isLocked(visid) )
	return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as ) return false;

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();

    Attrib::SelSpec myas( *as );
    bool dousemulticomp = false;
    if ( attrserv->handleAttribSubMenu(mnuid,myas,dousemulticomp) )
    {
	if ( dousemulticomp )
	{
	    mDynamicCastGet( visSurvey::SurveyObject*, so,
			     visserv->getObject(visid));
	    mDynamicCastGet( visSurvey::HorizonDisplay*, hd, so );

	    if ( so && !so->canHaveMultipleTextures() && !hd )
	    {
		const TypeSet<Attrib::SelSpec>& selspecs =
						attrserv->getTargetSelSpecs();
		if ( selspecs.size() )
		{
		    uiMSG().warning( tr("This object cannot yet display more "
					"than the first component selected") );
		    myas = selspecs[0];
		    dousemulticomp = false;
		}
	    }
	}

	if ( dousemulticomp )
	{
	    Attrib::SelSpec mtas( "Multi-Textures",
				  Attrib::SelSpec::cOtherAttrib() );
	    if ( !ODMainWin()->applMgr().calcMultipleAttribs( mtas ) )
		return false;
	}
	else
	{
	    visserv->setSelSpec( visid, attrib, myas );
	    if ( !visserv->calcManipulatedAttribs(visid) )
		visserv->calculateAttrib( visid, attrib, false );
	}
	return true;
    }

    return false;
}


uiString uiODAttribTreeItem::createDisplayName( VisID visid, int attrib )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    uiString dispname = uiString::emptyString();
    if ( as )
    {
	const int nrtextures = visserv->nrTextures( visid, attrib );
	const int curidx = visserv->selectedTexture( visid, attrib );
	if ( nrtextures > 1 )
	{
	    BufferString str;
	    str.add( curidx+1 ).add( "/" ).add( nrtextures ).addSpace();
	    dispname.append( toUiString(str) );
	}
	dispname.append( toUiString(as->userRef()) );
    }

    if ( as && as->isNLA() )
    {
	dispname = toUiString(as->objectRef());
	uiString nodenm = toUiString( as->userRef());
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = toUiString(IOM().nameOf( as->userRef() ));
	dispname = toUiString("%1 (%2)").arg( as->objectRef() ).arg( nodenm );
    }

    if ( as && as->id().asInt()==Attrib::SelSpec::cAttribNotSel().asInt() )
	dispname = uiStrings::sRightClick();
    else if ( !as )
	dispname = visserv->getUiObjectName( visid );
    else if ( as->id().asInt() == Attrib::SelSpec::cNoAttrib().asInt() )
	dispname = uiString::emptyString();

    return dispname;
}


uiString uiODAttribTreeItem::createDisplayName() const
{
    return createDisplayName( displayID(), attribNr() );
}


void uiODAttribTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject( displayID() ))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}

	if ( visserv->getDisplayedDataPackID(displayID(),attribNr()) ==
							DataPack::cNoID() )
	    uitreeviewitem_->setIcon( uiODSceneMgr::cColorColumn(), "warning" );
	else
	    displayMiniCtab( so->getColTabSequence(attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}
