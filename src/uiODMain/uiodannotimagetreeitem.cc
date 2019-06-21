/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
________________________________________________________________________

-*/



#include "uiodannottreeitem.h"

#include "ioobj.h"
#include "keystrs.h"
#include "picksetmanager.h"
#include "uivispartserv.h"
#include "uifileselector.h"
#include "visimagedisplay.h"
#include "uistrings.h"
#include "uipixmap.h"


ImageSubItem::ImageSubItem( Pick::Set& pck, int displayid )
    : uiODAnnotSubItem( pck, displayid )
    , filemnuitem_( m3Dots(tr("Select image")) )
{
    defscale_ = mCast(float,set_.dispSize());
}



bool ImageSubItem::init()
{
    if ( !uiODAnnotSubItem::init() )
	return false;

    visSurvey::ImageDisplay* id = 0;
    if ( displayid_==-1 )
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

    if ( !id )
	{ pErrMsg("Is this normal?"); return false; }

    id->needFileName.notifyIfNotNotified(
			mCB(this,ImageSubItem,retrieveFileName) );

    BufferString filename;
    set_.pars().get( sKey::FileName(), filename );
    if ( filename.isEmpty() )
	Pick::SetMGR().getIOObjPars( getSetID() )
			    .get( sKey::FileName(), filename );
    if ( !filename.isEmpty() )
	id->setFileName( filename.buf() );

    return true;
}



const char* ImageSubItem::parentType() const
{
    return typeid(ImageParentItem).name();
}


void ImageSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    mDynamicCastGet(visSurvey::ImageDisplay*,id,visserv_->getObject(displayid_))
    par.set( sKey::FileName(), id->getFileName() );
}


void ImageSubItem::createMenu( MenuHandler* menu, bool istb )
{
    if ( !menu || menu->menuID()!=displayID() )
	return;

    uiODAnnotSubItem::createMenu( menu, istb );
    mAddMenuOrTBItem(istb,0,menu,&filemnuitem_,true,false);
}


void ImageSubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    uiODAnnotSubItem::handleMenuCB( cb );
    if ( mnuid==filemnuitem_.id )
    {
	menu->setIsHandled(true);
	selectFileName();
    }
}


void ImageSubItem::retrieveFileName( CallBacker* cb )
{
    selectFileName();
}


void ImageSubItem::updateColumnText( int col )
{
    if ( col!=1 )
	uiODDisplayTreeItem::updateColumnText(col);
}


void ImageSubItem::selectFileName() const
{
    mDynamicCastGet(visSurvey::ImageDisplay*,id,
		    visserv_->getObject(displayid_))
    if ( !id )
	return;

    uiFileSelector::Setup fssu( id->getFileName() );
    OD::GetSupportedImageFormats( fssu.formats_, true );
    uiFileSelector uifs( getUiParent(), fssu );
    if ( uifs.go() )
	id->setFileName( uifs.fileName() );
}
