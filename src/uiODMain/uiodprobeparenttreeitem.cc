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
#include "survinfo.h"


uiODSceneProbeParentTreeItem::AddType uiODSceneProbeParentTreeItem::getAddType(
					int mnuid )
{
    return mnuid <= RGBA ? (AddType)mnuid : DefaultData;
}

int uiODSceneProbeParentTreeItem::getMenuID( AddType addtyp )
{
    return addtyp;
}


uiString uiODSceneProbeParentTreeItem::sTxt4AddMnu( AddType addtyp )
{
    switch ( addtyp )
    {
	case DefaultData:	return tr("Add Default Data");
	case DefaultAttrib:	return tr("Add Default Attribute");
	case Select:		return m3Dots(tr("Add and Select Data"));
	case RGBA:		return m3Dots(tr("Add Color Blended"));
    }
    pFreeFnErrMsg("non-enum value passed");
    return uiStrings::sAdd();
}


const char* uiODSceneProbeParentTreeItem::iconID4AddMnu( AddType addtyp )
{
    switch ( addtyp )
    {
	case DefaultData:	return "attribtype_stored";
	case DefaultAttrib:	return "attribtype_attrib";
	case Select:		return "selectfromlist";
	case RGBA:		return "colorblending";
    }
    pFreeFnErrMsg("non-enum value passed");
    return "create";
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
    return mnuid < 0 ? false : handleSubMenu( mnuid );
}

void uiODSceneProbeParentTreeItem::addMenuItems()
{
#   define mAddAddMnuItem(at) \
    menu_->insertAction( \
	    new uiAction(sTxt4AddMnu(at),iconID4AddMnu(at)), getMenuID(at) )

    mAddAddMnuItem( DefaultData );
    if ( Attrib::DescSet::global(is2D()).hasTrueAttribute() )
	mAddAddMnuItem( DefaultAttrib );
    mAddAddMnuItem( Select );
    mAddAddMnuItem( RGBA );
}


bool uiODSceneProbeParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid < 0 )
	return true;

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

    SilentTaskRunnerProvider trprov;
    if ( !fillProbe(*newprobe)	||
	 !ProbeMGR().store(*newprobe,trprov).isOK() )
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
	mTIUiMsg().message( tr("No saved color settings found for the selected"
	      " cube. Default settings will be loaded. For changing "
	    "these settings, click on \"Save Color Settings\" option in tree."),
	    uiString::empty(), uiString::empty(), true );
	attriblayer->saveDisplayPars();
    }

    probe.addLayer( attriblayer );
    return true;
}


bool uiODSceneProbeParentTreeItem::getSelRGBAttrSelSpecs( Probe& probe,
				    Attrib::SelSpecList& rgbaspecs ) const
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false;

    const Pos::GeomID geomid = probe.position().hsamp_.getGeomID();
    const ZDomain::Info* zdinf = applMgr()->visServer()->zDomainInfo(sceneID());

    return applMgr()->attrServer()->selectRGBAttribs( rgbaspecs,zdinf,geomid );
}


bool uiODSceneProbeParentTreeItem::setRGBProbeLayers( Probe& probe ) const
{
    Attrib::SelSpecList rgbaspecs;
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


void uiODSceneProbeParentTreeItem::getDefZRange(
				StepInterval<float>& zrg ) const
{
    zrg = SI().zRange( OD::UsrWork );
    Presentation::ManagedViewer* vwr = OD::PrMan().getViewer( viewerID() );
    const ZAxisTransform* ztransf = vwr ? vwr->getZAxisTransform() : 0;
    if ( ztransf )
	zrg = ztransf->getZInterval( true );
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
	return uiString::empty();

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
