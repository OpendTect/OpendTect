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
#include "uimsg.h"
#include "uioddatatreeitem.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribprobelayer.h"
#include "attribdescset.h"
#include "coltabseqmgr.h"
#include "probemanager.h"
#include "zdomain.h"


uiODSceneProbeParentTreeItem::AddType
		uiODSceneProbeParentTreeItem::getAddType( int mnuid ) const
{
    return mnuid < Empty ? (AddType)mnuid : Empty;
}


uiString uiODSceneProbeParentTreeItem::sAddEmptyPlane()
{
    return tr("Add Empty Plane");
}

uiString uiODSceneProbeParentTreeItem::sAddAndSelectData()
{
    return m3Dots(tr("Add and Select Data"));
}

uiString uiODSceneProbeParentTreeItem::sAddDefaultData()
{
    return tr("Add Default Data");
}

uiString uiODSceneProbeParentTreeItem::sAddDefaultAttrib()
{
    return tr("Add Default Attribute");
}

uiString uiODSceneProbeParentTreeItem::sAddColorBlended()
{
    return m3Dots(uiStrings::sAddColBlend());
}

uiODSceneProbeParentTreeItem::uiODSceneProbeParentTreeItem( const uiString& nm )
    : uiODSceneParentTreeItem( nm )
    , menu_( 0 )
{
}


const char* uiODSceneProbeParentTreeItem::childObjTypeKey() const
{
    return ProbePresentationInfo::sFactoryKey();
}


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
    menu_->insertAction( new uiAction(sAddDefaultData(),"attribtype_stored"),
			 cAddDefaultDataMenuID() );
    if ( Attrib::DescSet::global(is2D()).hasTrueAttribute() )
	menu_->insertAction(
		new uiAction(sAddDefaultAttrib(),"attribtype_attrib"),
		cAddDefaultAttribMenuID() );
    menu_->insertAction(
		new uiAction(sAddAndSelectData(),"selectfromlist"),
		cAddAndSelectDataMenuID() );
    menu_->insertAction(
		new uiAction(sAddColorBlended(),"colorblending"),
		cAddColorBlendedMenuID() );
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
    uiPresManagedTreeItem* newitem = addChildItem( probeprinfo );

    newitem->emitPrRequest( Presentation::Add );
    return true;
}


bool uiODSceneProbeParentTreeItem::fillProbe( Probe& newprobe )
{
    if ( typetobeadded_ == DefaultData )
	return setDefaultAttribLayer( newprobe, true );
    if ( typetobeadded_ == DefaultAttrib )
	return setDefaultAttribLayer( newprobe, false );
    else if ( typetobeadded_ == Select )
	return setSelAttribProbeLayer( newprobe );
    else if ( typetobeadded_ == RGBA )
	return setRGBProbeLayers( newprobe );
    return true;

}


bool uiODSceneProbeParentTreeItem::setDefaultAttribLayer( Probe& probe,
							  bool stored ) const
{
    if ( !applMgr() )
	return false;

    return addDefaultAttribLayer( *applMgr(), probe, stored );
}

bool uiODSceneProbeParentTreeItem::addDefaultAttribLayer( uiODApplMgr& applmgr,
			      Probe& probe, bool stored )
{
    Attrib::DescID descid;
    if ( !applmgr.getDefaultDescID(descid,probe.is2D(),stored) )
	return false;

    const Attrib::DescSet& ads = Attrib::DescSet::global( probe.is2D() );
    const Attrib::Desc* desc = ads.getDesc( descid );
    if ( !desc )
	return false;

    Attrib::SelSpec as( 0, descid, false, "" );
    as.set( *desc );

    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    attriblayer->setSelSpec( as );
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
    selattr.set2D( probe.is2D() );
    return getSelAttribSelSpec( probe, selattr, *applMgr(), sceneID(),
			      tr("Select attribute to display") );
}


bool uiODSceneProbeParentTreeItem::setSelAttribProbeLayer( Probe& probe ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    AttribProbeLayer* attriblayer = new AttribProbeLayer;
    Attrib::SelSpec attrlayselspec = attriblayer->selSpec();
    if ( !getSelAttrSelSpec(probe,attrlayselspec) )
    {
	delete attriblayer;
	return false;
    }

    attriblayer->setSelSpec( attrlayselspec );
    if ( attriblayer->selSpec().isStored(0)
      && !attriblayer->haveSavedDispPars() )
    {
	/* TODO add 'never show this message again' facility */
	uiMSG().message( tr("No saved color settings found for the selected"
	      " cube. Default settings will be loaded. For changing "
	    "these settings, click on \"Save Color Settings\" option in tree."),
	    uiString::emptyString(), uiString::emptyString(), true );
	attriblayer->saveDisplayPars();
    }

    probe.addLayer( attriblayer );
    return true;
}


bool uiODSceneProbeParentTreeItem::getSelRGBAttrSelSpecs( Probe& probe,
				    TypeSet<Attrib::SelSpec>& rgbaspecs ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    const Pos::GeomID geomid = probe.position().hsamp_.getGeomID();
    const ZDomain::Info* zdinf = applMgr()->visServer()->zDomainInfo(sceneID());

    return applMgr()->attrServer()->selectRGBAttribs( rgbaspecs,zdinf,geomid );
}


bool uiODSceneProbeParentTreeItem::setRGBProbeLayers( Probe& probe ) const
{
    TypeSet<Attrib::SelSpec> rgbaspecs;
    if ( !getSelRGBAttrSelSpecs(probe,rgbaspecs) )
	return false;

    for ( int idx=0; idx<rgbaspecs.size(); idx++ )
    {
	AttribProbeLayer* attriblayer =
	    new AttribProbeLayer( AttribProbeLayer::RGB );
	attriblayer->setSelSpec( rgbaspecs[idx] );
	attriblayer->setSequence( ColTab::SeqMGR().getRGBBlendColSeq(idx) );
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

    return probe->displayName();
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
    Attrib::SelSpec attrlayselspec = as ? *as : attriblayer->selSpec();
    attrlayselspec.set2D( parentprobe->is2D() );
    if ( !getSelAttribSelSpec(*parentprobe,attrlayselspec,*applmgr,sceneID(),
			      tr("Select attribute to display")) )
    {
	delete attriblayer;
	return 0;
    }

    attriblayer->setSelSpec( attrlayselspec );
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
    mDynamicCastGet(const Probe*,probe,dataObj().ptr());
    return probe;
}

Probe* uiODSceneProbeTreeItem::getProbe()
{
    mDynamicCastGet(Probe*,probe,dataObj().ptr());
    return probe;
}


Presentation::ObjInfo* uiODSceneProbeTreeItem::getObjPrInfo() const
{
    const Probe* probe = getProbe();
    if ( !probe )
	return 0;

    ProbePresentationInfo* prinfo =
	new ProbePresentationInfo( ProbeMGR().getID(*probe) );
    return prinfo;
}
