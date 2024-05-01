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
#include "uistrings.h"


ImageSubItem::ImageSubItem( Pick::Set& pck, const VisID& displayid )
    : uiODAnnotSubItem( pck, displayid )
    , filemnuitem_( m3Dots(tr("Select image")) )
{
    defscale_ = mCast(float,set_->disp_.pixsize_);
}


ImageSubItem::~ImageSubItem()
{
    detachAllNotifiers();
    removeStuff();
    visserv_->removeObject( displayid_, sceneID() );
}


bool ImageSubItem::init()
{
    if (  !displayid_.isValid() )
    {
	RefMan<visSurvey::ImageDisplay> id = visSurvey::ImageDisplay::create();
	visserv_->addObject( id.ptr(), sceneID(), true );
	displayid_ = id->id();
	id->setUiName( name_ );
    }

    RefMan<visSurvey::ImageDisplay> id =
	dCast(visSurvey::ImageDisplay*,visserv_->getObject(displayid_) );
    if ( !id )
	return false;

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

    imagedisp_ = id;
    return uiODAnnotSubItem::init();
}


ConstRefMan<visSurvey::ImageDisplay> ImageSubItem::getDisplay() const
{
    return imagedisp_.get();
}


RefMan<visSurvey::ImageDisplay> ImageSubItem::getDisplay()
{
    return imagedisp_.get();
}


const char* ImageSubItem::parentType() const
{ return typeid(ImageParentItem).name(); }


void ImageSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    ConstRefMan<visSurvey::ImageDisplay> id = getDisplay();
    if ( !id )
	return;

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


void ImageSubItem::selectFileName()
{
    RefMan<visSurvey::ImageDisplay> id = getDisplay();
    if ( !id )
	return;

    BufferString filename = id->getFileName();
    BufferString filter = "JPEG (*.jpg *.jpeg);;PNG (*.png)";
    uiFileDialog dlg( getUiParent(), true, filename, filter );
    if ( !dlg.go() )
	return;

    filename = dlg.fileName();

    id->setFileName(filename);
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}
