/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodattribtreeitem.h"

#include "attribsel.h"
#include "attribprobelayer.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioobj.h"
#include "ptrman.h"
#include "probeimpl.h"
#include "posvecdataset.h"
#include "randomlineprobe.h"
#include "survinfo.h"
#include "volumedatapackzaxistransformer.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvobj.h"
#include "vissurvscene.h"
#include "zdomain.h"
#include "seisdatapack.h"
#include "zaxistransform.h"
#include "zaxistransformutils.h"

#include "uiattribpartserv.h"
#include "uicoltabsel.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uishortcutsmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"



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


void uiODAttribTreeItem::prepareForShutdown()
{
    mDynamicCastGet( visSurvey::SurveyObject*,so,
		     applMgr()->visServer()->getObject(displayID()) );
    if ( !so )
	return;

    so->removeAttrib( attribNr() );
    uiODDataTreeItem::prepareForShutdown();
}


bool uiODAttribTreeItem::init()
{
    mDynamicCastGet( visSurvey::SurveyObject*,so,
		     applMgr()->visServer()->getObject(displayID()) );
    if ( !so )
	return false;

    AttribProbeLayer* attrlay = attribProbeLayer();
    if ( attrlay && attrlay->dispType()==AttribProbeLayer::RGB )
	so->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
    if ( parent_->nrChildren()>1 )
	so->addAttrib();//For first child attrib is automatically added

    keyPressed()->notify( mCB(this,uiODAttribTreeItem,keyPressCB) );


    return uiODDataTreeItem::init();
}


bool uiODAttribTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() )
	return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->canSetColTabSequence( displayID() ) )
	return false;

    AttribProbeLayer* attrprlayer = attribProbeLayer();
    if ( !attrprlayer )
	return false;

    coltabsel_.setSequence( attrprlayer->sequence() );
    coltabsel_.setMapper( attrprlayer->mapper() );
    coltabsel_.asParent()->display( true );
    return true;
}


void uiODAttribTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleUnpack(uiKeyDesc,kd,cb);

    if ( kd.key()==OD::KB_PageUp || kd.key()==OD::KB_PageDown )
        applMgr()->pageUpDownPressed( kd.key()==OD::KB_PageUp );
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
	if ( geomid.isValid() ) \
	    attrserv->filter2DMenuItems( *subitem, *as, geomid, true, 0 ); \
	mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
	subitem = attrserv->calcAttribMenuItem( *as, is2d, needext ); \
	if ( geomid.isValid() ) \
	    attrserv->filter2DMenuItems( *subitem, *as, geomid, false, 2 ); \
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
	subitem = attrserv->nlaAttribMenuItem( *as, is2d, needext ); \
	if ( subitem && subitem->nrItems() ) \
	{ \
	    if ( geomid.isValid() ) \
		attrserv->filter2DMenuItems(*subitem, *as, geomid, false, 0 ); \
	    mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
	} \
	subitem = attrserv->storedAttribMenuItem( *as, is2d, true ); \
	if ( geomid.isValid() ) \
	    attrserv->filter2DMenuItems( *subitem, *as, geomid, true, 1 ); \
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    } \
    mCreateDepthDomMnuItemIfNeeded( is2d, needext ); \
}

//TODO PrIMPL handle multicomponent data
void uiODAttribTreeItem::createSelMenu( MenuItem& mnu )
{
    AttribProbeLayer* attrprlayer = attribProbeLayer();
    if ( !attrprlayer )
	return;

    const Probe* parentprobe = attrprlayer->getProbe();
    Pos::GeomID geomid;
    mDynamicCastGet(const Line2DProbe*,line2dprobe,parentprobe);
    if ( line2dprobe )
	geomid = line2dprobe->geomID();

    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    if ( as && visserv->hasAttrib(displayID()) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayID()));
	if ( !so ) return;

	const OD::Pol2D3D pol2d3d = so->getAllowedDataType();
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()));

	const bool needtransform = !scene->zDomainInfo().def_.isSI();
	const bool cantransform = !needtransform || scene->getZAxisTransform();

	bool need2dlist = SI().has2D() && pol2d3d != OD::Only3D;
	bool need3dlist = SI().has3D() && pol2d3d != OD::Only2D;

	MenuItem* subitem;
	attrserv->resetMenuItems();
	if ( need3dlist )
	    mCreateItemsList( false, need2dlist );
	if ( need2dlist && pol2d3d != OD::Only3D )
	    mCreateItemsList( true, need3dlist );
    }
}


void uiODAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    bool isonly2d = false;
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(sceneID()));
    if ( so )
	isonly2d = so->getAllowedDataType() == OD::Only2D;

    if ( !istb )
    {
	selattrmnuitem_.removeItems();
	createSelMenu( selattrmnuitem_);
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
	 AttribProbeLayer* attrprlayer = attribProbeLayer();
	 if ( !attrprlayer ) return;
	 attrprlayer->saveDisplayPars();
    }
    else if ( mnuid == usecolsettingsmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->useDefColTab( displayID(), attribNr() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( handleSelMenu(mnuid) )
    {
	menu->setIsHandled(true);
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
}


bool uiODAttribTreeItem::handleSelMenu( int mnuid )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==-1 || visserv->isLocked(displayID()) )
	return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    if ( !as ) return false;

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();

    Attrib::SelSpec myas( *as );
    bool dousemulticomp = false;
    if ( attrserv->handleAttribSubMenu(mnuid,myas,dousemulticomp) )
    {
	if ( dousemulticomp )
	{
	    mDynamicCastGet( visSurvey::SurveyObject*, so,
			     visserv->getObject(displayID()));

	    if ( so && !so->canHaveMultipleTextures() )
	    {
		const Attrib::SelSpecList& selspecs =
						attrserv->getTargetSelSpecs();
		if ( selspecs.size() )
		{
		    mTIUiMsg().warning( tr("This object cannot yet display "
				"more than the first component selected") );
		    myas = selspecs[0];
		    dousemulticomp = false;
		}
	    }
	}

	if ( dousemulticomp )
	{
	    Attrib::SelSpec mtas( "Multi-Textures",
				  Attrib::SelSpec::cOtherAttribID() );
	    if ( !ODMainWin()->applMgr().calcMultipleAttribs( mtas ) )
		return false;
	}
	else
	{
	    AttribProbeLayer* attrlayer = attribProbeLayer();
	    if ( attrlayer )
		attrlayer->setSelSpec( myas );
	}
	return true;
    }

    return false;
}


//TODO PrIMPL remove visid related stuffs
uiString uiODAttribTreeItem::createDisplayName( int visid, int attrib )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    uiString dispname = uiString::empty();
    if ( as )
    {
	const int nrtextures = visserv->nrTextures( visid, attrib );
	const int curidx = visserv->selectedTexture( visid, attrib );
	if ( nrtextures > 1 )
	{
	    BufferString str;
	    str.add( curidx ).add( "/" ).add( nrtextures ).addSpace();
	    dispname.appendPlainText( str, true );
	}
	dispname.appendPlainText( as->userRef(), true );
    }

    if ( as && as->isNLA() )
    {
	dispname = toUiString(as->objectRef());
	BufferString nodenm = as->userRef();
	if ( DBKey::isValidString(nodenm) )
	    nodenm = DBKey(nodenm).name();
	dispname = toUiString("%1 (%2)").arg( as->objectRef() ).arg( nodenm );
    }

    if ( as && as->id() == Attrib::SelSpec::cAttribNotSelID() )
        dispname = uiStrings::sRightClick();
    else if ( !as )
	dispname = toUiString( visserv->getObjectName(visid) );
    else if ( as->id() == Attrib::SelSpec::cNoAttribID() )
        dispname = uiString::empty();

    return dispname;
}


uiString uiODAttribTreeItem::createDisplayName() const
{
    const AttribProbeLayer* attrlayer = attribProbeLayer();
    if ( !attrlayer )
	return uiStrings::sRightClick();

    Attrib::SelSpec as = attrlayer->selSpec();
    uiString dispname( as.id().isValid() ? toUiString(as.userRef())
					 : uiString::empty() );
    if ( as.isNLA() )
    {
	BufferString nodenm = as.userRef();
	if ( DBKey::isValidString(nodenm) )
	    nodenm = DBKey(nodenm).name();
	dispname = toUiString("%1 (%2)").arg( as.objectRef() ).arg( nodenm );
    }

    if ( as.id() == Attrib::SelSpec::cAttribNotSelID() )
	dispname = uiStrings::sRightClick();
    else if ( as.id() == Attrib::SelSpec::cNoAttribID() )
	dispname = uiString::empty();

    return dispname;
}


void uiODAttribTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	const ColTab::Sequence* seq =
	    attribProbeLayer() ? &attribProbeLayer()->sequence() : 0;
	if ( seq )
	{
	    displayMiniCtab( seq );
	    coltabsel_.setSequence( *seq );
	}
    }

    uiODDataTreeItem::updateColumnText( col );
}


void uiODAttribTreeItem::setProbeLayer( ProbeLayer* probelayer )
{
    uiODDataTreeItem::setProbeLayer( probelayer );
    AttribProbeLayer* attrlay = attribProbeLayer();
    if ( !attrlay )
	return;

    mDynamicCastGet( visSurvey::SurveyObject*, so,
		     visserv_->getObject(displayID()));
    if ( !so )
	return;

    if ( attrlay->dispType()==AttribProbeLayer::RGB )
	so->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
}


const AttribProbeLayer* uiODAttribTreeItem::attribProbeLayer() const
{
    mDynamicCastGet(const AttribProbeLayer*,attrlayer,probelayer_.ptr());
    return attrlayer;
}


AttribProbeLayer* uiODAttribTreeItem::attribProbeLayer()
{
    mDynamicCastGet(AttribProbeLayer*,attrlayer,probelayer_.ptr());
    return attrlayer;
}


//TODO PrIMPL Different DPM for Horizon Attribute
DataPackMgr& uiODAttribTreeItem::getDPM()
{
    return DPM(DataPackMgr::SeisID());
}


ConstRefMan<DataPack> uiODAttribTreeItem::calculateAttribute()
{
    ConstRefMan<DataPack> attrdp( 0 );
    AttribProbeLayer* attrprlayer = attribProbeLayer();
    if ( !attrprlayer )
	return attrdp;
    const Attrib::SelSpec attrselspec = attrprlayer->selSpec();
    if ( attrselspec.id().isInvalid() )
	return attrdp;

    const Probe* parentprobe = attrprlayer->getProbe();
    if ( !parentprobe ) //TODO: Bring all display items under the probe system
    {
	const int visid = displayID();
	const int attrib = attribNr();
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visserv->setSelSpec( visid, attrib, attrselspec );
	if ( !visserv->calcManipulatedAttribs(visid) )
	    visserv->calculateAttrib( visid, attrib, false );

	return 0;
    }

    const TrcKeyZSampling probepos = parentprobe->position();
    ZAxisTransform* ztransform = visserv_->getZAxisTransform( sceneID() );
    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
    attrserv->setTargetSelSpec( attrselspec );

    mDynamicCastGet(const RandomLineProbe*,rdlprobe,parentprobe);
    mDynamicCastGet(const ZSliceProbe*,zprobe,parentprobe);
    DataPack::ID attrdpid;
    if ( zprobe && ztransform && !attrselspec.isZTransformed() )
    {
	RefMan<DataPointSet> dps = new DataPointSet( false, true );
	DPM(DataPackMgr::PointID()).add( dps );

	ZAxisTransformPointGenerator generator( *ztransform );
	generator.setInput( probepos, SilentTaskRunnerProvider() );
	generator.setOutputDPS( *dps );
	generator.execute();

	const int firstcol = dps->nrCols();
	BufferStringSet userrefs; userrefs.add( attrselspec.userRef() );
	dps->dataSet().add( new DataColDef(userrefs.get(0)) );
	if ( !attrserv->createOutput(*dps,firstcol) )
	    return attrdp;

	attrdpid =
	    RegularSeisDataPack::createDataPackForZSlice( &dps->bivSet(),
			    probepos, ztransform->toZDomainInfo(), &userrefs );
	return getDPM().getDP( attrdpid );
    }


    if ( rdlprobe )
	attrdpid = attrserv->createRdmTrcsOutput( probepos.zsamp_,
					      rdlprobe->randomeLineID() );
    else
	attrdpid = attrserv->createOutput( probepos, DataPack::cNoID() );

    attrdp = getDPM().getDP( attrdpid );
    if ( !attrdp )
	return attrdp;

    mDynamicCastGet(const VolumeDataPack*,voldp,attrdp.ptr());
    const FixedString zdomainkey( voldp ? voldp->zDomain().key() : "" );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( ztransform && !alreadytransformed )
    {
	VolumeDataPackZAxisTransformer transformer( *ztransform );
	transformer.setInput( voldp );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( transformer.getOutput() )
	    attrdp = transformer.getOutput();
    }

    return attrdp;
}


void uiODAttribTreeItem::updateDisplay()
{
    AttribProbeLayer* attrprlayer = attribProbeLayer();
    if ( !attrprlayer )
	return;

    if ( attrprlayer->dataPackID().isInvalid() )
    {
	ConstRefMan<DataPack> attrdp = calculateAttribute();
	if ( attrdp )
	{
	    NotifyStopper ns( attrprlayer->objectChanged(), this );
	    attrprlayer->setDataPackID( attrdp->id() );
	}
    }

    visserv_->setSelSpec( displayID(), attribNr(), attrprlayer->selSpec() );
    visserv_->setColTabMapper( displayID(), attribNr(), attrprlayer->mapper() );
    visserv_->setColTabSequence( displayID(), attribNr(),
				    attrprlayer->sequence() );
    visserv_->setDataPackID( displayID(), attribNr(),
			     attrprlayer->dataPackID() );
}


void uiODAttribTreeItem::colSeqChg( const ColTab::Sequence& seq )
{
    AttribProbeLayer* attrprlayer = attribProbeLayer();
    if ( !attrprlayer )
	return;

    attrprlayer->setSequence( seq );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}
