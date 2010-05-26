/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uiodvolproctreeitem.cc,v 1.1 2010-05-26 20:41:55 cvskris Exp $";

#include "uiodvolproctreeitem.h"

#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "vissurvobj.h"
#include "volproctrans.h"
#include "volprocattrib.h"


namespace VolProc
{


void uiDataTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }    


uiDataTreeItem::uiDataTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selmenuitem_( "Select setup", true )
    , reloadmenuitem_( "Reload", true )
{}


uiDataTreeItem::~uiDataTreeItem()
{ }


bool uiDataTreeItem::anyButtonClick( uiListViewItem* item )
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


uiODDataTreeItem* uiDataTreeItem::create( const Attrib::SelSpec& as,
					  const char* parenttype )
{
    if ( as.id()!=Attrib::SelSpec::cOtherAttrib() ||
	 strcmp( as.defString(), sKeyVolumeProcessing() ) )
	return 0;

    return new uiDataTreeItem( parenttype );
}


void  uiDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::createMenuCB( cb );
    mDynamicCastGet(MenuHandler*,menu,cb);

    PtrMan<IOObj> ioobj = IOM().get( mid_ );

    mAddMenuItem( menu, &selmenuitem_, true, false );
    mAddMenuItem( menu, &reloadmenuitem_, ioobj, false );
}

void  uiDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() )
       return;

    if ( mnuid==selmenuitem_.id )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid_ );

	const CtxtIOObj ctxt( VolProcessingTranslatorGroup::ioContext(),ioobj );
	uiIOObjSelDlg dlg( ODMainWin(), ctxt );
	if ( !dlg.go() || !dlg.nrSel() )
	    return;

	mid_ = dlg.selected( 0 );
    }

    if ( mnuid==selmenuitem_.id || mnuid==reloadmenuitem_.id )
    {
	menu->setIsHandled( true );
	const BufferString def =
	    VolProc::ExternalAttribCalculator::createDefinition( mid_ );

	Attrib::SelSpec spec( "Velocity", Attrib::SelSpec::cOtherAttrib(),
			      false, 0 );

	spec.setDefString( def.buf() );

	ODMainWin()->applMgr().visServer()->setSelSpec( displayID(), 
							attribNr(), spec );
	ODMainWin()->applMgr().visServer()->calculateAttrib( displayID(),
							     attribNr(), false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
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


};//namespace

