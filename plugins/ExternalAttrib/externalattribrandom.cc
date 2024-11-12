/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "externalattribrandom.h"

#include "arraynd.h"
#include "attribdesc.h"
#include "attribdatacubes.h"
#include "attribsel.h"
#include "seisdatapack.h"
#include "statrand.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "vissurvobj.h"


namespace ExternalAttrib
{

void Random::initClass()
{ Attrib::ExtAttrFact().addCreator( create, 0 ); }


Attrib::ExtAttribCalc* Random::create( const Attrib::SelSpec& as )
{
    Random* res = new Random;
    if ( res->setTargetSelSpec(as) )
	return res;

    delete res;
    return nullptr;
}


Random::Random()
{}


Random::~Random()
{ }


bool Random::setTargetSelSpec( const Attrib::SelSpec& ss )
{
    const char* definition = ss.defString();

    BufferString attribname;
    if ( !Attrib::Desc::getAttribName( definition, attribname ) ||
	 strcmp(attribname.buf(), sAttribName()) )
	return false;

    return true;
}


ConstRefMan<RegularSeisDataPack>
Random::createAttrib( const TrcKeyZSampling& tkzs,
		      const RegularSeisDataPack* cachedp, TaskRunner* tr)
{
    const Attrib::DataCubes* dc = nullptr;
    const Attrib::DataCubes* output = createAttrib( tkzs, dc );
    if ( !output || !output->nrCubes() )
	return nullptr;

    RefMan<RegularSeisDataPack> regsdp = new RegularSeisDataPack(
	    VolumeDataPack::categoryStr(tkzs.isFlat() && tkzs.nrZ()!=1,false) );
    regsdp->setSampling( tkzs );
    regsdp->addComponent( uiStrings::sEmptyString() );

    for ( int lineidx=0; lineidx<tkzs.nrLines(); lineidx++ )
    {
	for ( int trcidx=0; trcidx<tkzs.nrTrcs(); trcidx++ )
	{
	    for ( int zidx=0; zidx<tkzs.nrZ(); zidx++ )
	    {
		regsdp->data(0).set( lineidx, trcidx, zidx,
				     Stats::RandGen::get() );
	    }
	}
    }

    return regsdp;
}


// RandomManager

RandomManager::RandomManager()
    : addrandomattribmnuitem_( "Add random attribute" )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    visserv->getMenuHandler()->createnotifier.notify(
	    mCB(this,RandomManager,createMenuCB) );
    visserv->getMenuHandler()->handlenotifier.notify(
	    mCB(this,RandomManager,handleMenuCB) );
}


RandomManager::~RandomManager()
{
    ODMainWin()->applMgr().visServer()->getMenuHandler()->createnotifier.remove(
	    mCB(this,RandomManager,createMenuCB) );
    ODMainWin()->applMgr().visServer()->getMenuHandler()->handlenotifier.remove(
	    mCB(this,RandomManager,handleMenuCB) );
}


void RandomManager::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    const int displayid = menu->menuID();

    visBase::DataObject* dataobj =
	ODMainWin()->applMgr().visServer()->getObject( displayid );

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const bool islocked = visserv->isLocked( displayid );

    mDynamicCastGet(visSurvey::SurveyObject*,so,dataobj);
    if ( !so || so->getAttributeFormat()!=visSurvey::SurveyObject::Cube
	     || !so->canHaveMultipleAttribs() )
    {
	mResetMenuItem( &addrandomattribmnuitem_ );
	return;
    }

    mAddMenuItem( menu, &addrandomattribmnuitem_,
		  !islocked && so->canAddAttrib( 1 ), false );
}


void RandomManager::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiTreeItem* parent = ODMainWin()->sceneMgr().findItem( menu->menuID() );
    if ( !parent || mnuid != addrandomattribmnuitem_.id )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    menu->setIsHandled( true );

    const int attrib = visserv->addAttrib( menu->menuID() );
    Attrib::SelSpec spec( 0, Attrib::SelSpec::cOtherAttrib(), false, 0 );
    spec.setDefString( Random::sAttribName() );
    visserv->setSelSpec( menu->menuID(), attrib, spec );
    visserv->calculateAttrib( menu->menuID(), attrib, false );
    visserv->enableAttrib( menu->menuID(), attrib, true );

    auto* newitem = new uiRandomTreeItem( typeid(*parent).name() );
    parent->addChild( newitem, false );
}


void uiRandomTreeItem::initClass()
{
    uiODDataTreeItem::factory().addCreator( create, 0 );
}


uiRandomTreeItem::uiRandomTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
{}


BufferString uiRandomTreeItem::createDisplayName() const
{
    return BufferString( sKeyDefinition() );
}


uiODDataTreeItem* uiRandomTreeItem::create( const Attrib::SelSpec& as,
					    const char* parenttype )
{
    if ( as.id()!=Attrib::SelSpec::cOtherAttrib() ||
	 strcmp( as.defString(), sKeyDefinition() ) )
	return nullptr;

    return new uiRandomTreeItem( parenttype );
}


bool uiRandomTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( !visserv->getColTabSequence(displayID(),attribNr()) )
	return false;

    ODMainWin()->applMgr().updateColorTable( displayID(), attribNr() );

    return true;
}


void uiRandomTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet( visSurvey::SurveyObject*, so,
			 visserv->getObject( displayID() ) )
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}

	if ( !so->hasColor() )
	    displayMiniCtab(so->getColTabSequence(attribNr()));
    }

    uiODDataTreeItem::updateColumnText( col );
}


void uiRandomTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDataTreeItem::createMenu( menu, istb );
}



void uiRandomTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
}


} // namespace ExternalAttrib
