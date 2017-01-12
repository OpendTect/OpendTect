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

#include "attribdescsetsholder.h"
#include "attribprobelayer.h"
#include "attribdescset.h"
#include "coltabsequence.h"
#include "probemanager.h"
#include "zdomain.h"


uiODSceneProbeParentTreeItem::Type
		uiODSceneProbeParentTreeItem::getType( int mnuid ) const
{
    switch ( mnuid )
    {
	case 0: return uiODSceneProbeParentTreeItem::Default; break;
	case 1: return uiODSceneProbeParentTreeItem::Select; break;
	case 2: return uiODSceneProbeParentTreeItem::RGBA; break;
	default: return uiODSceneProbeParentTreeItem::Empty;
    }
}


uiString uiODSceneProbeParentTreeItem::sAddEmptyPlane()
{ return tr("Add Empty Plane"); }

uiString uiODSceneProbeParentTreeItem::sAddAndSelectData()
{ return m3Dots(tr("Add and Select Data")); }

uiString uiODSceneProbeParentTreeItem::sAddDefaultData()
{ return tr("Add Default Data"); }

uiString uiODSceneProbeParentTreeItem::sAddColorBlended()
{ return m3Dots(uiStrings::sAddColBlend()); }

uiODSceneProbeParentTreeItem::uiODSceneProbeParentTreeItem( const uiString& nm )
    : uiODSceneParentTreeItem( nm )
    , menu_( 0 )
{}


const char* uiODSceneProbeParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


bool uiODSceneProbeParentTreeItem::showSubMenu()
{
    if ( !canShowSubMenu() )
	return false;

    if ( !menu_ )
	menu_ = new uiMenu( getUiParent(), uiStrings::sAction() );

    menu_->clear();
    addMenuItems();
    addStandardItems( *menu_ );
    const int mnuid = menu_->exec();
    return handleSubMenu( mnuid );
}

void uiODSceneProbeParentTreeItem::addMenuItems()
{
    menu_->insertItem(
	new uiAction( uiODSceneProbeParentTreeItem::sAddDefaultData()),
		      sAddDefaultDataMenuID() );
    menu_->insertItem(
	new uiAction( uiODSceneProbeParentTreeItem::sAddAndSelectData()),
		      sAddAndSelectDataMenuID() );
    menu_->insertItem(
	new uiAction( uiODSceneProbeParentTreeItem::sAddColorBlended()),
		      sAddColorBlendedMenuID() );
}


bool uiODSceneProbeParentTreeItem::handleSubMenu( int mnuid )
{
    if ( setProbeToBeAddedParams(mnuid) )
	return addChildProbe();

    handleStandardItems( mnuid );
    return true;
}


bool uiODSceneProbeParentTreeItem::addChildProbe()
{
    RefMan<Probe> newprobe = createNewProbe();
    if ( !newprobe )
	return false;

    if ( !fillProbe(*newprobe)	||
	 !ProbeMGR().store(*newprobe).isOK() )
	return false;

    ProbePresentationInfo probeprinfo( ProbeMGR().getID(*newprobe) );
    uiODPrManagedTreeItem* newitem = addChildItem( probeprinfo );
    newitem->emitPRRequest( OD::Add );
    return true;
}


bool uiODSceneProbeParentTreeItem::fillProbe( Probe& newprobe )
{
    if ( typetobeadded_ == Default )
	return setDefaultAttribLayer( newprobe );
    else if ( typetobeadded_ == Select )
	return setSelAttribProbeLayer( newprobe );
    else if ( typetobeadded_ == RGBA )
	return setRGBProbeLayers( newprobe );
    return true;

}


bool uiODSceneProbeParentTreeItem::setDefaultAttribLayer( Probe& probe ) const
{
    if ( !applMgr() )
	return false;

    return addDefaultAttribLayer( *applMgr(), probe );
}

bool uiODSceneProbeParentTreeItem::addDefaultAttribLayer( uiODApplMgr& applmgr,
							  Probe& probe )
{
    Attrib::DescID descid;
    if ( !applmgr.getDefaultDescID(descid,probe.is2D()) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( probe.is2D(), true );
    const Attrib::Desc* desc = ads->getDesc( descid );
    if ( !desc )
	return false;

    Attrib::SelSpec as( 0, descid, false, "" );
    as.set( *desc );

    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    attriblayer->setSelSpec( as );
    attriblayer->useStoredColTabPars();
    TrcKeyZSampling probepos = probe.position();
    probe.addLayer( attriblayer );
    return true;
}


static bool getSelAttribSelSpec( Probe& probe, Attrib::SelSpec& selattr,
			       uiODApplMgr& applmgr, int scnid,
			       uiString seltxt )
{
    const Pos::GeomID probegeomid = probe.position().hsamp_.getGeomID();
    const ZDomain::Info* zdinf = applmgr.visServer()->zDomainInfo( scnid );
    const bool issi = !zdinf || zdinf->def_.isSI();
    return applmgr.attrServer()->selectAttrib(
	    selattr, issi ? 0 : zdinf, probegeomid, seltxt );
}


bool uiODSceneProbeParentTreeItem::getSelAttrSelSpec(
	Probe& probe, Attrib::SelSpec& selattr ) const
{
    selattr.set2DFlag( probe.is2D() );
    return getSelAttribSelSpec( probe, selattr, *applMgr(), sceneID(),
			      tr("Select attribute to display") );
}


bool uiODSceneProbeParentTreeItem::setSelAttribProbeLayer( Probe& probe ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    Attrib::SelSpec attrlayselspec = attriblayer->getSelSpec();
    if ( !getSelAttrSelSpec(probe,attrlayselspec) )
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
	attriblayer->setColTab( RGBBlend::getColTab(idx) );
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
	uiODDataTreeItem* item = createProbeLayerItem( *probelay );
	if ( item )
	{
	    addChild( item, false );
	    item->updateDisplay();
	    item->setChecked( visserv_->isAttribEnabled(displayid_,idx));
	}
    }

    return true;
}


uiString uiODSceneProbeTreeItem::createDisplayName() const
{
    const Probe* probe = getProbe();
    if ( !probe )
	return uiString::emptyString();

    return toUiString( probe->name() );
}


void uiODSceneProbeTreeItem::handleAddAttrib()
{
    uiODDataTreeItem* newitem = createAttribItem( 0 );
    if ( newitem )
    {
	addChild( newitem, false );
	newitem->updateDisplay();
    }
}


uiODDataTreeItem* uiODSceneProbeTreeItem::createAttribItem(
	const Attrib::SelSpec* as ) const
{
    Probe* parentprobe = const_cast<Probe*> (getProbe());
    uiODApplMgr* applmgr = const_cast<uiODSceneProbeTreeItem*>(this)->applMgr();
    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    Attrib::SelSpec attrlayselspec = as ? *as : attriblayer->getSelSpec();
    attrlayselspec.set2DFlag( parentprobe->is2D() );
    if ( !getSelAttribSelSpec(*parentprobe,attrlayselspec,*applmgr,sceneID(),
			      tr("Select attribute to display")) )
    {
	delete attriblayer;
	return 0;
    }

    attriblayer->setSelSpec( attrlayselspec );
    attriblayer->useStoredColTabPars();
    parentprobe->addLayer( attriblayer );

    return createProbeLayerItem( *attriblayer );
}


uiODDataTreeItem* uiODSceneProbeTreeItem::createProbeLayerItem(
	ProbeLayer& probelayer ) const
{
    return uiODDataTreeItem::fac().create( probelayer );
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
