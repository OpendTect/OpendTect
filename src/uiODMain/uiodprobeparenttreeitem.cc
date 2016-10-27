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


static uiODProbeParentTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODProbeParentTreeItem::Default; break;
	case 1: return uiODProbeParentTreeItem::Select; break;
	case 2: return uiODProbeParentTreeItem::Empty; break;
	case 3: return uiODProbeParentTreeItem::RGBA; break;
	default: return uiODProbeParentTreeItem::Empty;
    }
}


uiString uiODProbeParentTreeItem::sAddEmptyPlane()
{ return tr("Add Empty Plane"); }

uiString uiODProbeParentTreeItem::sAddAndSelectData()
{ return tr("Add and Select Data"); }

uiString uiODProbeParentTreeItem::sAddDefaultData()
{ return tr("Add Default Data"); }

uiString uiODProbeParentTreeItem::sAddColorBlended()
{ return uiStrings::sAddColBlend(); }

uiString uiODProbeParentTreeItem::sAddAtWellLocation()
{ return m3Dots(tr("Add at Well Location")); }

uiODProbeParentTreeItem::uiODProbeParentTreeItem( const uiString& nm )
    : uiODSceneParentTreeItem( nm )
{}


const char* uiODProbeParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


bool uiODProbeParentTreeItem::showSubMenu()
{
    if ( !canShowSubMenu() )
	return false;

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem(
	new uiAction(uiODProbeParentTreeItem::sAddDefaultData()), 0 );
    mnu.insertItem(
	new uiAction(uiODProbeParentTreeItem::sAddAndSelectData()), 1 );
    if ( canAddFromWell() )
	mnu.insertItem(
	    new uiAction(uiODProbeParentTreeItem::sAddAtWellLocation()), 2 );
    mnu.insertItem(
	new uiAction(uiODProbeParentTreeItem::sAddColorBlended()), 3 );
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
	    //TODO newitm->setAtWellLocation( *wd );
	    newitem->emitPRRequest( OD::Add );
	}
    }
    handleStandardItems( mnuid );
    return true;
}


bool uiODProbeParentTreeItem::fillProbe( Probe& newprobe, Type type )
{
    if ( type == Default )
	return setDefaultAttribLayer( newprobe );
    else if ( type == Select )
	return setSelAttribProbeLayer( newprobe );
    else if ( type == RGBA )
	return setRGBProbeLayers( newprobe );
    return false;

}


bool uiODProbeParentTreeItem::setDefaultAttribLayer( Probe& probe ) const
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


bool uiODProbeParentTreeItem::setSelAttribProbeLayer( Probe& probe ) const
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


bool uiODProbeParentTreeItem::setRGBProbeLayers( Probe& probe ) const
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
