/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
________________________________________________________________________

-*/



#include "treeitem.h"

#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "survinfo.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uivispartserv.h"
#include "visscalebar.h"

namespace Annotations
{

struct ScaleBarPars
{
ScaleBarPars()
    : oninlcrl_(true), orientation_(0), length_(1000)
{}

bool		oninlcrl_;
int		orientation_;
double		length_;
LineStyle	ls_;
};


class uiScaleBarDialog : public uiDialog
{
public:
uiScaleBarDialog( uiParent* p, const ZDomain::Info& zinf )
    : uiDialog(p,Setup("ScaleBar properties",mNoDlgTitle,"50.0.14"))
    , propertyChange(this)
    , zinf_(zinf)
{
    setCtrlStyle( LeaveOnly );

    objectfld_ = new uiGenInput( this, "Picked on",
		BoolInpSpec(true,"Inl/Crl","Z-slice") );
    objectfld_->setSensitive( false );

    const BoolInpSpec horverspec( true, "Horizontal", "Vertical" );
    horverfld_ = new uiGenInput( this, "Orientation", horverspec );
    horverfld_->attach( alignedBelow, objectfld_ );
    horverfld_->valuechanged.notify( mCB(this,uiScaleBarDialog,changeCB) );

    // TODO: Support X, Y, Inl, Crl orientations
    const BoolInpSpec xyspec( true, "X", "Y" );
    inlcrlfld_= new uiGenInput( this, "Orientation", xyspec );
    inlcrlfld_->attach( alignedBelow, objectfld_ );
    inlcrlfld_->valuechanged.notify( mCB(this,uiScaleBarDialog,changeCB) );
    inlcrlfld_->display( false );

    lengthfld_ = new uiGenInput( this, "Length", DoubleInpSpec() );
    lengthfld_->attach( alignedBelow, horverfld_ );
    lengthfld_->updateRequested.notify( mCB(this,uiScaleBarDialog,changeCB) );
    unitlbl_ = new uiLabel( this, "" );
    unitlbl_->attach( rightTo, lengthfld_ );

    LineStyle ls; uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    linestylefld_ = new uiSelLineStyle( this, ls, lssu );
    linestylefld_->changed.notify( mCB(this,uiScaleBarDialog,changeCB) );
    linestylefld_->attach( alignedBelow, lengthfld_ );
}

void setPars( const ScaleBarPars& pars )
{
    objectfld_->setValue( pars.oninlcrl_ );
    if ( pars.oninlcrl_ )
	horverfld_->setValue( !pars.orientation_ );
    else
	inlcrlfld_->setValue( !pars.orientation_ );
    updateOrientationFld();

    double length = pars.length_;
    if ( pars.oninlcrl_ && pars.orientation_==1 ) length *= zinf_.userFactor();
    lengthfld_->setValue( length );
    linestylefld_->setStyle( pars.ls_ );
}

void fillPars( ScaleBarPars& pars ) const
{
    pars.oninlcrl_ = objectfld_->getBoolValue();
    pars.orientation_ = pars.oninlcrl_ ? !horverfld_->getBoolValue()
				       : !inlcrlfld_->getBoolValue();
    double length = lengthfld_->getDValue();
    if ( pars.oninlcrl_ && pars.orientation_==1 ) length /= zinf_.userFactor();
    pars.length_ = length;
    pars.ls_ = linestylefld_->getStyle();
}

Notifier<uiScaleBarDialog>	propertyChange;

protected:

void updateOrientationFld()
{
    horverfld_->display( objectfld_->getBoolValue() );
    inlcrlfld_->display( !objectfld_->getBoolValue() );
}

void changeCB( CallBacker* )
{
    unitlbl_->setText( !objectfld_->getBoolValue() ||
		       horverfld_->getBoolValue()
		? SI().getXYUnitString(true) : zinf_.unitStr(true) );
    propertyChange.trigger();
}

    uiGenInput*			objectfld_;
    uiGenInput*			horverfld_;
    uiGenInput*			inlcrlfld_;
    uiGenInput*			lengthfld_;
    uiLabel*			unitlbl_;
    uiSelLineStyle*		linestylefld_;
    const ZDomain::Info&	zinf_;
};


// ScaleBarSubItem
const char* ScaleBarSubItem::parentType() const
{ return typeid(ScaleBarParentItem).name(); }


ScaleBarSubItem::ScaleBarSubItem( Pick::Set& pck, int displayid )
    : SubItem(pck,displayid)
    , propmnuitem_("Properties ...")
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.reportDispChange( this, *set_ );

    propmnuitem_.iconfnm = "disppars";
}


bool ScaleBarSubItem::init()
{
    if (  displayid_==-1 )
    {
	ScaleBarDisplay* ad = ScaleBarDisplay::create();
	visserv_->addObject( ad, sceneID(), true );
	displayid_ = ad->id();
	ad->setName( name_ );
    }

    mDynamicCastGet(ScaleBarDisplay*,ad,visserv_->getObject(displayid_))
    if ( !ad ) return false;

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    PtrMan<IOObj> ioobj = IOM().get( mgr.id(setidx) );
    if ( !ioobj ) return false;

    ad->fromPar( set_->pars_ );
    return SubItem::init();
}


void ScaleBarSubItem::fillStoragePar( IOPar& par ) const
{
    SubItem::fillStoragePar( par );
    mDynamicCastGet(ScaleBarDisplay*,ad,visserv_->getObject(displayid_))
    ad->toPar( par );
}


void ScaleBarSubItem::createMenu( MenuHandler* menu, bool istb )
{
    SubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem(istb,menu,menu,&propmnuitem_,true,false );
}


void ScaleBarSubItem::handleMenuCB( CallBacker* cb )
{
    SubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	mDynamicCastGet(ScaleBarDisplay*,ad,visserv_->getObject(displayid_));
	ScaleBarPars pars;
	pars.oninlcrl_ = ad->isOnInlCrl();
	pars.orientation_ = ad->getOrientation();
	pars.length_ = ad->getLength();
	pars.ls_ = LineStyle(LineStyle::Solid,ad->getLineWidth(),
			     set_->disp_.color_);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	uiScaleBarDialog dlg( getUiParent(), scene->zDomainInfo() );
	dlg.setPars( pars );
	dlg.propertyChange.notify( mCB(this,ScaleBarSubItem,propertyChange) );
	dlg.go();
	updateColumnText( 1 );
    }
}


void ScaleBarSubItem::propertyChange( CallBacker* cb )
{
    mDynamicCastGet(uiScaleBarDialog*,dlg,cb)
    if ( !dlg ) return;

    ScaleBarPars pars;
    dlg->fillPars( pars );
    setColor( pars.ls_.color_ );

    mDynamicCastGet(ScaleBarDisplay*,ad,visserv_->getObject(displayid_));
    ad->setOnInlCrl( pars.oninlcrl_ );
    ad->setOrientation( pars.orientation_ );
    ad->setLineWidth( pars.ls_.width_ );
    ad->setLength( pars.length_ );

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}

} // namespace Annotations
