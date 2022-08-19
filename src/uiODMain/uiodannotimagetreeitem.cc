/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodannottreeitem.h"

#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "uivispartserv.h"
#include "uifiledlg.h"
#include "visimagedisplay.h"
#include "uistrings.h"


ImageSubItem::ImageSubItem( Pick::Set& pck, VisID displayid )
    : uiODAnnotSubItem( pck, displayid )
    , filemnuitem_( m3Dots(tr("Select image")) )
{
    defscale_ = mCast(float,set_->disp_.pixsize_);
}



bool ImageSubItem::init()
{
    visSurvey::ImageDisplay* id = 0;
    if (  !displayid_.isValid() )
    {
	id = visSurvey::ImageDisplay::create();
	visserv_->addObject( id, sceneID(), true );
	displayid_ = id->id();
	id->setUiName( name_ );
    }
    else
    {
	mDynamicCast(visSurvey::ImageDisplay*,id,
			visserv_->getObject(displayid_))
    }

    if ( !id ) return false;

    id->needFileName.notifyIfNotNotified(
			mCB(this,ImageSubItem,retrieveFileName) );

    BufferString filename;
    set_->pars_.get( sKey::FileName(), filename );
    if ( filename.isEmpty() )
    {
	Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	const int setidx = mgr.indexOf( *set_ );
	PtrMan<IOObj> ioobj = IOM().get( mgr.id(setidx) );
	if ( ioobj )
	    ioobj->pars().get(sKey::FileName(), filename );
    }

    if ( !filename.isEmpty() )
	id->setFileName( filename.buf() );

    return uiODAnnotSubItem::init();
}



const char* ImageSubItem::parentType() const
{ return typeid(ImageParentItem).name(); }


void ImageSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    mDynamicCastGet(visSurvey::ImageDisplay*,id,visserv_->getObject(displayid_))
    par.set( sKey::FileName(), id->getFileName() );
}


void ImageSubItem::createMenu( MenuHandler* menu, bool istb )
{
    if ( !menu || menu->menuID()!=displayID().asInt() )
	return;

    uiODAnnotSubItem::createMenu( menu, istb );
    mAddMenuOrTBItem(istb,0,menu,&filemnuitem_,true,false);
}


void ImageSubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID().asInt() )
	return;

    uiODAnnotSubItem::handleMenuCB( cb );
    if ( mnuid==filemnuitem_.id )
    {
	menu->setIsHandled(true);
	selectFileName();
    }
}


void ImageSubItem::retrieveFileName( CallBacker* )
{
    selectFileName();
}


void ImageSubItem::updateColumnText(int col)
{
    if ( col!=1 )
	uiODDisplayTreeItem::updateColumnText(col);
}


void ImageSubItem::selectFileName() const
{
    mDynamicCastGet(visSurvey::ImageDisplay*,id,
		    visserv_->getObject(displayid_))
    if ( !id ) return;

    BufferString filename = id->getFileName();
    BufferString filter = "JPEG (*.jpg *.jpeg);;PNG (*.png)";
    uiFileDialog dlg( getUiParent(), true, filename, filter );
    if ( !dlg.go() ) return;

    filename = dlg.fileName();

    id->setFileName(filename);
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}
