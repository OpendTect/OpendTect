/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodannottreeitem.h"

#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "uiarrowdlg.h"
#include "uivispartserv.h"
#include "uiodapplmgr.h"

#define mRemoveImple(type) \
void type##ParentItem::setRemovedCB( CallBacker* cb ) \
    { \
	mDynamicCastGet(Pick::Set*,ps,cb) \
	if ( !ps ) return; \
	for ( int idx=0; idx<nrChildren(); idx++ )\
	{ \
	    mDynamicCastGet(type##SubItem*,itm,getChild(idx)) \
	    if ( !itm ) continue; \
	    if ( itm->getSet().ptr() == ps ) \
	    { \
		applMgr()->visServer()->removeObject( itm->displayID(),\
							sceneID() ); \
		uiTreeItem::removeChild( itm ); \
		return; \
	    } \
	} \
    } \


mRemoveImple(Arrow)
mRemoveImple(Image)
mRemoveImple(ScaleBar)



const char* ArrowSubItem::parentType() const
{ return typeid(ArrowParentItem).name(); }


ArrowSubItem::ArrowSubItem( Pick::Set& pck, const VisID& displayid )
    : uiODAnnotSubItem( pck, displayid )
    , propmnuitem_( m3Dots(uiStrings::sProperties()) )
    , arrowtype_( 2 )
{
    defscale_ = mCast(float,set_->disp_.pixsize_);
    propmnuitem_.iconfnm = "disppars";
}


ArrowSubItem::~ArrowSubItem()
{
    detachAllNotifiers();
    removeStuff();
    visserv_->removeObject( displayid_, sceneID() );
}


bool ArrowSubItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::ArrowDisplay> ad = visSurvey::ArrowDisplay::create();
	visserv_->addObject( ad.ptr(), sceneID(), true);
	displayid_ = ad->id();
	ad->setUiName( name_ );
    }

    RefMan<visSurvey::ArrowDisplay> ad =
	dCast(visSurvey::ArrowDisplay*,visserv_->getObject(displayid_) );
    if ( !ad )
	return false;

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    PtrMan<IOObj> ioobj = IOM().get( mgr.id(setidx) );
    if ( !ioobj )
	return false;

    if ( !ioobj->pars().get(sKeyArrowType(),arrowtype_) )
	set_->pars_.get( sKeyArrowType(), arrowtype_ );

    ad->setType( (visSurvey::ArrowDisplay::Type)arrowtype_ );

    int linewidth = 2;
    if ( !ioobj->pars().get(sKeyLineWidth(),linewidth) )
	set_->pars_.get( sKeyLineWidth(), linewidth );
    ad->setLineWidth( linewidth );

    //Read Old format orientation
    for ( int idx=set_->size()-1; idx>=0; idx-- )
    {
	const Pick::Location& loc = set_->get( idx );
	if ( !loc.hasText() )
	    continue;

	Pick::Location newloc = loc;
	BufferString orientation;
	if ( loc.getKeyedText("O",orientation) )
	{
	    Sphere dir = loc.dir();
	    if ( orientation[0] == '2' )
	    {
		dir.phi = (float) (-M_PI_2-dir.phi);
		dir.theta = M_PI_2;
	    }
	    else
	    {
		dir.phi = (float) (M_PI_2-dir.phi);
		dir.theta -= M_PI_2;
	    }

	    newloc.setDir( dir );
	    newloc.removeTextKey( "0" );
	}

	set_->set( idx, newloc );
    }

    arrowdisp_ = ad;
    return uiODAnnotSubItem::init();
}


ConstRefMan<visSurvey::ArrowDisplay> ArrowSubItem::getDisplay() const
{
    return arrowdisp_.get();
}


RefMan<visSurvey::ArrowDisplay> ArrowSubItem::getDisplay()
{
    return arrowdisp_.get();
}


void ArrowSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    ConstRefMan<visSurvey::ArrowDisplay> ad = getDisplay();
    if ( !ad )
	return;

    par.set( sKeyArrowType(), (int) ad->getType() );
    par.set( sKeyLineWidth(), ad->getLineWidth() );
}


void ArrowSubItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAnnotSubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID().asInt() )
	return;

    mAddMenuOrTBItem(istb,menu,menu,&propmnuitem_,true,false );
}


void ArrowSubItem::handleMenuCB( CallBacker* cb )
{
    uiODAnnotSubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID().asInt() )
	return;

    RefMan<visSurvey::ArrowDisplay> ad = getDisplay();
    if ( !ad )
	return;

    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	uiArrowDialog dlg( getUiParent() );
	dlg.setColor( set_->disp_.color_ );
	dlg.setArrowType( arrowtype_ );
	dlg.setLineWidth( ad->getLineWidth() );
	dlg.propertyChange.notify( mCB(this,ArrowSubItem,propertyChange) );
	dlg.setScale( mCast(float,set_->disp_.pixsize_/defscale_) );
	dlg.go();
	if ( set_->disp_.color_!=dlg.getColor() )
	{
	    set_->disp_.color_ = dlg.getColor();
	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.reportDispChange( this, *set_ );
	}

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

    RefMan<visSurvey::ArrowDisplay> ad = getDisplay();
    if ( !ad )
	return;

    ad->setType( (visSurvey::ArrowDisplay::Type) arrowtype );
    ad->setLineWidth( dlg->getLineWidth() );

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}
