/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "uiodprobeparenttreeitem.h"

#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uioddatatreeitem.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"

#include "attribdescsetsholder.h"
#include "attribprobelayer.h"
#include "coltabsequence.h"
#include "probemanager.h"
#include "welldata.h"
#include "wellman.h"
#include "zdomain.h"


static uiODSceneProbeParentTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODSceneProbeParentTreeItem::Default; break;
	case 1: return uiODSceneProbeParentTreeItem::Select; break;
	case 2: return uiODSceneProbeParentTreeItem::Empty; break;
	case 3: return uiODSceneProbeParentTreeItem::RGBA; break;
	default: return uiODSceneProbeParentTreeItem::Empty;
    }
}


uiString uiODSceneProbeParentTreeItem::sAddEmptyPlane()
{ return tr("Add Empty Plane"); }

uiString uiODSceneProbeParentTreeItem::sAddAndSelectData()
{ return tr("Add and Select Data"); }

uiString uiODSceneProbeParentTreeItem::sAddDefaultData()
{ return tr("Add Default Data"); }

uiString uiODSceneProbeParentTreeItem::sAddColorBlended()
{ return uiStrings::sAddColBlend(); }

uiString uiODSceneProbeParentTreeItem::sAddAtWellLocation()
{ return m3Dots(tr("Add at Well Location")); }

uiODSceneProbeParentTreeItem::uiODSceneProbeParentTreeItem( const uiString& nm )
    : uiODSceneParentTreeItem( nm )
{}


const char* uiODSceneProbeParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


bool uiODSceneProbeParentTreeItem::showSubMenu()
{
    if ( !canShowSubMenu() )
	return false;

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem(
	new uiAction(uiODSceneProbeParentTreeItem::sAddDefaultData()), 0 );
    mnu.insertItem(
	new uiAction(uiODSceneProbeParentTreeItem::sAddAndSelectData()), 1 );
    if ( canAddFromWell() )
	mnu.insertItem( new uiAction(
		    uiODSceneProbeParentTreeItem::sAddAtWellLocation()), 2 );
    mnu.insertItem(
	new uiAction(uiODSceneProbeParentTreeItem::sAddColorBlended()), 3 );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();
    Type type = getType( mnuid );
    if ( mnuid==0 || mnuid==1 || mnuid==3 )
    {
	Probe* newprobe = createNewProbe();
	if ( !newprobe )
	    return false;

	fillProbe( *newprobe, type );
	ProbePresentationInfo probeprinfo( ProbeMGR().getID(*newprobe) );
	uiODPrManagedTreeItem* newitem = addChildItem( probeprinfo );
	newitem->emitPRRequest( OD::Add );
    }
    else if ( mnuid==2 )
    {
	DBKeySet wellids;
	if ( !applMgr()->wellServer()->selectWells(wellids) )
	    return true;

	for ( int idx=0;idx<wellids.size(); idx++ )
	{
	    Well::Data* wd = Well::MGR().get( wellids[idx] );
	    if ( !wd ) continue;
	    Probe* newprobe = createNewProbe();
	    if ( !newprobe )
		return false;

	    fillProbe( *newprobe, type );
	    ProbePresentationInfo probeprinfo( ProbeMGR().getID(*newprobe) );
	    uiODPrManagedTreeItem* newitem = addChildItem( probeprinfo );

	    setMoreObjectsToDoHint( idx<wellids.size()-1 );
	    //TODO PrIMPL newitm->setAtWellLocation( *wd );
	    newitem->emitPRRequest( OD::Add );
	}
    }
    handleStandardItems( mnuid );
    return true;
}


bool uiODSceneProbeParentTreeItem::fillProbe( Probe& newprobe, Type type )
{
    if ( type == Default )
	return setDefaultAttribLayer( newprobe );
    else if ( type == Select )
	return setSelAttribProbeLayer( newprobe );
    else if ( type == RGBA )
	return setRGBProbeLayers( newprobe );
    return false;

}


bool uiODSceneProbeParentTreeItem::setDefaultAttribLayer( Probe& probe ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( probe.is2D(), true );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );

    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    attriblayer->setSelSpec( as );
    attriblayer->useStoredColTabPars();
    TrcKeyZSampling probepos = probe.position();
    probe.addLayer( attriblayer );
    return true;
}


bool uiODSceneProbeParentTreeItem::setSelAttribProbeLayer( Probe& probe ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    const Pos::GeomID probegeomid = probe.position().hsamp_.getGeomID();
    const ZDomain::Info* zdinf = applMgr()->visServer()->zDomainInfo(sceneID());
    const bool issi = !zdinf || zdinf->def_.isSI();
    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    Attrib::SelSpec attrlayselspec = attriblayer->getSelSpec();
    bool selok = applMgr()->attrServer()->selectAttrib(
			attrlayselspec, issi ? 0 : zdinf,
			probegeomid, tr("first layer") );
    if ( !selok )
    {
	delete attriblayer;
	return false;
    }

    attriblayer->setSelSpec( attrlayselspec );
    attriblayer->useStoredColTabPars();
    probe.addLayer( attriblayer );
    return true;
}


bool uiODSceneProbeParentTreeItem::setRGBProbeLayers( Probe& probe ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    const Pos::GeomID probegeomid = probe.position().hsamp_.getGeomID();
    const ZDomain::Info* zdinf = applMgr()->visServer()->zDomainInfo(sceneID());

    TypeSet<Attrib::SelSpec> rgbaspecs;
    const bool selok =
	applMgr()->attrServer()->selectRGBAttribs(rgbaspecs,zdinf,probegeomid);
    if ( !selok ) return false;

    for ( int idx=0; idx<rgbaspecs.size(); idx++ )
    {
	AttribProbeLayer* attriblayer =
	    new AttribProbeLayer( AttribProbeLayer::RGB );
	attriblayer->setSelSpec( rgbaspecs[idx] );
	probe.addLayer( attriblayer );
    }

    return true;
}


uiODSceneProbeTreeItem::uiODSceneProbeTreeItem( Probe& prb )
    : uiODDisplayTreeItem()
{
    setDataObj( &prb );
}


uiODSceneProbeTreeItem::~uiODSceneProbeTreeItem()
{
    detachAllNotifiers();
}


bool uiODSceneProbeTreeItem::init()
{
    Probe* probe = getProbe();
    if ( !probe )
    {
	pErrMsg( "Shared Object not of type Probe" );
	return false;
    }

    if ( !uiODDisplayTreeItem::init() )
	return false;

    for ( int idx=0; idx<probe->nrLayers(); idx++ )
    {
	ProbeLayer* probelay = probe->getLayerByIdx( idx );
	uiODDataTreeItem* item = uiODDataTreeItem::fac().create( *probelay );
	if ( item )
	{
	    addChild( item, false );
	    item->setChecked( visserv_->isAttribEnabled(displayid_,idx));
	}
    }

    return true;
}


const Probe* uiODSceneProbeTreeItem::getProbe() const
{
    mDynamicCastGet(const Probe*,probe,dataobj_.ptr());
    return probe;
}

Probe* uiODSceneProbeTreeItem::getProbe()
{
    mDynamicCastGet(Probe*,probe,dataobj_.ptr());
    return probe;
}
