/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
________________________________________________________________________

-*/



#include "uiodannottreeitem.h"

#include "ioobj.h"
#include "picksetmanager.h"
#include "trigonometry.h"
#include "visarrowdisplay.h"
#include "uiarrowdlg.h"
#include "uivispartserv.h"

const char* ArrowSubItem::parentType() const
{ return typeid(ArrowParentItem).name(); }


ArrowSubItem::ArrowSubItem( Pick::Set& pck, int displayid )
    : uiODAnnotSubItem( pck, displayid )
    , propmnuitem_( m3Dots(uiStrings::sProperties()) )
    , arrowtype_( 2 )
{
    defscale_ = mCast(float,set_.dispSize());
    propmnuitem_.iconfnm = "disppars";
}


bool ArrowSubItem::init()
{
    if ( !uiODAnnotSubItem::init() )
	return false;

    if ( displayid_==-1 )
    {
	visSurvey::ArrowDisplay* ad = visSurvey::ArrowDisplay::create();
	visserv_->addObject( ad, sceneID(), true );
	displayid_ = ad->id();
	ad->setUiName( name_ );
    }

    mDynamicCastGet(visSurvey::ArrowDisplay*,ad,visserv_->getObject(displayid_))
    if ( !ad )
	return false;

    const DBKey setid = getSetID();
    const IOPar psiop( set_.pars() );
    const IOPar ioobjiop( Pick::SetMGR().getIOObjPars(setid) );
    if ( !ioobjiop.get(sKeyArrowType(),arrowtype_) )
	psiop.get( sKeyArrowType(), arrowtype_ );
    ad->setType( (visSurvey::ArrowDisplay::Type)arrowtype_ );

    int linewidth = 2;
    if ( !ioobjiop.get(sKeyLineWidth(),linewidth) )
	psiop.get( sKeyLineWidth(), linewidth );
    ad->setLineWidth( linewidth );

    //Read Old format orientation
    RefMan<Pick::Set> workps = new Pick::Set( set_ );
    BufferString orientation;
    bool anychg = false;
    Pick::SetIter4Edit psiter( *workps );
    while ( psiter.next() )
    {
	Pick::Location ploc = psiter.get();
	bool havekyedtxt = ploc.getKeyedText("O",orientation);
	if ( havekyedtxt )
	{
	    Sphere dir = ploc.dir();
	    if ( orientation[0] == '2' )
	    {
		dir.phi_ = (float) (-M_PI_2-dir.phi_);
		dir.theta_ = M_PI_2;
	    }
	    else
	    {
		dir.phi_ = (float) (M_PI_2-dir.phi_);
		dir.theta_ -= M_PI_2;
	    }
	    ploc.setDir( dir );
	}
	if ( havekyedtxt || !ploc.text().isEmpty() )
	{
	    ploc.setText( 0 );
	    psiter.get() = ploc;
	    anychg = true;
	}
    }
    psiter.retire();

    if ( anychg ) // kept track to avoid unnecessary updates everywhere
	set_ = *workps;

    return true;
}


void ArrowSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    mDynamicCastGet(visSurvey::ArrowDisplay*,ad,visserv_->getObject(displayid_))
    par.set( sKeyArrowType(), (int)ad->getType() );
    par.set( sKeyLineWidth(), ad->getLineWidth() );
}


void ArrowSubItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAnnotSubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem(istb,menu,menu,&propmnuitem_,true,false );
}


void ArrowSubItem::handleMenuCB( CallBacker* cb )
{
    uiODAnnotSubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	uiArrowDialog dlg( getUiParent() );
	dlg.setColor( set_.dispColor() );
	dlg.setArrowType( arrowtype_ );
	mDynamicCastGet(visSurvey::ArrowDisplay*,
			ad,visserv_->getObject(displayid_));
	dlg.setLineWidth( ad->getLineWidth() );
	dlg.propertyChange.notify( mCB(this,ArrowSubItem,propertyChange) );
	dlg.setScale( mCast(float,(set_.dispSize()/(defscale_))) );
	dlg.go();

	set_.setDispColor( dlg.getColor() );
	arrowtype_ = dlg.getArrowType();
	updateColumnText( 1 );
    }
}


void ArrowSubItem::propertyChange( CallBacker* cb )
{
    mDynamicCastGet(uiArrowDialog*,dlg,cb)
    if ( !dlg ) return;

    const int arrowtype = dlg->getArrowType();
    setScale( defscale_*dlg->getScale() );
    setColor( dlg->getColor() );

    mDynamicCastGet(visSurvey::ArrowDisplay*,
	ad,visserv_->getObject(displayid_));
    ad->setType( (visSurvey::ArrowDisplay::Type) arrowtype );
    ad->setLineWidth( dlg->getLineWidth() );
}
