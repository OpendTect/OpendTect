/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uiodvolproctreeitem.cc,v 1.3 2011-02-23 06:25:12 cvsnanne Exp $";

#include "uiodvolproctreeitem.h"

#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "vissurvobj.h"
#include "volprocattrib.h"
#include "volprocchain.h"
#include "volproctrans.h"


namespace VolProc
{


void uiDataTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }


uiDataTreeItem::uiDataTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selmenuitem_( "Select setup ...", true )
    , reloadmenuitem_( "Reload", true )
{}


uiDataTreeItem::~uiDataTreeItem()
{}


bool uiDataTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );
    
    if ( !select() ) return false;
    
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->getColTabSequence(displayID(),attribNr()) )
	return false;

    ODMainWin()->applMgr().updateColorTable( displayID(), attribNr() );

    return true;
}


uiODDataTreeItem* uiDataTreeItem::create( const Attrib::SelSpec& as,
					  const char* parenttype )
{
    if ( as.id()!=Attrib::SelSpec::cOtherAttrib() ||
	 strcmp( as.defString(), sKeyVolumeProcessing() ) )
	return 0;

    return new uiDataTreeItem( parenttype );
}


void uiDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::createMenuCB( cb );
    mDynamicCastGet(MenuHandler*,menu,cb);

    PtrMan<IOObj> ioobj = IOM().get( mid_ );

    mAddMenuItem( menu, &selmenuitem_, true, false );
    mAddMenuItem( menu, &reloadmenuitem_, ioobj, false );
}


void uiDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==selmenuitem_.id )
    {
	menu->setIsHandled( true );
	PtrMan<IOObj> ioobj = IOM().get( mid_ );

	const CtxtIOObj ctxt( VolProcessingTranslatorGroup::ioContext(),ioobj );
	uiIOObjSelDlg dlg( ODMainWin(), ctxt );
	if ( !dlg.go() || !dlg.nrSel() )
	    return;

	mid_ = dlg.selected( 0 );

	const BufferString def =
	    VolProc::ExternalAttribCalculator::createDefinition( mid_ );
	Attrib::SelSpec spec( "VolProc", Attrib::SelSpec::cOtherAttrib(),
			      false, 0 );
	spec.setDefString( def.buf() );
	visserv->setSelSpec( displayID(), attribNr(), spec );
	updateColumnText( uiODSceneMgr::cNameColumn() );

	RefMan<VolProc::Chain> chain = new VolProc::Chain;
	BufferString str;
	if ( chain && VolProcessingTranslator::retrieve(*chain,ioobj,str) )
	{
	    if ( !chain->areSamplesIndependent() )
	    {
		uiMSG().askGoOn("The output of this setup is not "
				"sample-independent, and the output may not be "
				"the same as when computing the entire volume");
	    }
	}
    }

    if ( mnuid==selmenuitem_.id || mnuid==reloadmenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv->calculateAttrib( displayID(), attribNr(), false );
    }
}


BufferString uiDataTreeItem::createDisplayName() const
{
    BufferString dispname = "<right-click>";
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	dispname = ioobj->name();

    return dispname;
}


void uiDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
		 	visserv->getObject(displayID()))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}

	if ( !so->hasColor() )
	    displayMiniCtab( so->getColTabSequence(attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}


};//namespace

