/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
________________________________________________________________________

-*/



#include "uiodannottreeitem.h"

#include "draw.h"
#include "ioobj.h"
#include "iopar.h"
#include "picksetmanager.h"
#include "survinfo.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uivispartserv.h"
#include "visscalebardisplay.h"
#include "od_helpids.h"


struct ScaleBarPars
{
    ScaleBarPars()
	: oninlcrl_(true), orientation_(0), length_(1000)   {}

    bool		oninlcrl_;
    int			orientation_;
    double		length_;
    OD::LineStyle	ls_;
};


class uiScaleBarDialog : public uiDialog
{ mODTextTranslationClass(uiScaleBarDialog);
public:
uiScaleBarDialog( uiParent* p, const ZDomain::Info& zinf )
    : uiDialog(p,Setup(tr("Scale Bar Properties")
		,mNoDlgTitle, mODHelpKey(mArrowDialogHelpID) ))
    , propertyChange(this)
    , zinf_(zinf)
{
    objectfld_ = new uiGenInput( this, tr("Picked on"),
	BoolInpSpec(true,tr("Inl/Crl"),uiStrings::sZSlice()) );
    objectfld_->setSensitive( false );

    const uiString orientstr( uiStrings::sOrientation() );
    const BoolInpSpec horverspec( true, uiStrings::sHorizontal(),
                                  uiStrings::sVertical() );
    horverfld_ = new uiGenInput( this, orientstr, horverspec );
    horverfld_->attach( alignedBelow, objectfld_ );
    horverfld_->valuechanged.notify( mCB(this,uiScaleBarDialog,changeCB) );

    // TODO: Support X, Y, Inl, Crl orientations
    const BoolInpSpec xyspec( true, uiStrings::sX(), uiStrings::sY() );
    inlcrlfld_= new uiGenInput( this, orientstr, xyspec );
    inlcrlfld_->attach( alignedBelow, objectfld_ );
    inlcrlfld_->valuechanged.notify( mCB(this,uiScaleBarDialog,changeCB) );
    inlcrlfld_->display( false );

    lengthfld_ = new uiGenInput( this, uiStrings::sLength(), DoubleInpSpec() );
    lengthfld_->attach( alignedBelow, horverfld_ );
    lengthfld_->updateRequested.notify( mCB(this,uiScaleBarDialog,changeCB) );
    unitlbl_ = new uiLabel( this, uiString::empty() );
    unitlbl_->setPrefWidthInChar( 6 );
    unitlbl_->attach( rightTo, lengthfld_ );

    OD::LineStyle ls; uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    linestylefld_ = new uiSelLineStyle( this, ls, lssu );
    linestylefld_->changed.notify( mCB(this,uiScaleBarDialog,changeCB) );
    linestylefld_->attach( alignedBelow, lengthfld_ );
}

void setPars( const ScaleBarPars& pars )
{
    pars_ = pars;
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

bool acceptOK()
{
    changeCB(0);
    return true;
}

bool rejectOK()
{
    setPars( pars_ );
    propertyChange.trigger();
    return true;
}


void changeCB( CallBacker* )
{
    const bool isxy = !objectfld_->getBoolValue() || horverfld_->getBoolValue();
    uiString unstr( isxy ? SI().xyUnitString() : zinf_.def_.unitStr() );
    unitlbl_->setText( unstr.parenthesize() );
    propertyChange.trigger();
}

    uiGenInput*			objectfld_;
    uiGenInput*			horverfld_;
    uiGenInput*			inlcrlfld_;
    uiGenInput*			lengthfld_;
    uiLabel*			unitlbl_;
    uiSelLineStyle*		linestylefld_;
    const ZDomain::Info&	zinf_;
    ScaleBarPars		pars_;
};


// ScaleBarSubItem
const char* ScaleBarSubItem::parentType() const
{ return typeid(ScaleBarParentItem).name(); }


ScaleBarSubItem::ScaleBarSubItem( Pick::Set& pck, int displayid )
    : uiODAnnotSubItem(pck,displayid)
    , propmnuitem_(m3Dots(uiStrings::sProperties()))
{
    propmnuitem_.iconfnm = "disppars";
}


bool ScaleBarSubItem::init()
{
    if ( !uiODAnnotSubItem::init() )
	return false;

    if (  displayid_==-1 )
    {
	visSurvey::ScaleBarDisplay* ad = visSurvey::ScaleBarDisplay::create();
	visserv_->addObject( ad, sceneID(), true );
	visserv_->setViewMode( false );
	displayid_ = ad->id();
	ad->setUiName( name_ );
    }

    mDynamicCastGet(visSurvey::ScaleBarDisplay*,ad,
						visserv_->getObject(displayid_))
    if ( !ad )
	return false;

    ad->fromPar( Pick::SetMGR().getIOObjPars(getSetID()) );
    return true;
}


void ScaleBarSubItem::fillStoragePar( IOPar& par ) const
{
    uiODAnnotSubItem::fillStoragePar( par );
    mDynamicCastGet(visSurvey::ScaleBarDisplay*,ad,
		    visserv_->getObject(displayid_))
    ad->toPar( par );
}


void ScaleBarSubItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAnnotSubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem(istb,menu,menu,&propmnuitem_,true,false );
}


void ScaleBarSubItem::handleMenuCB( CallBacker* cb )
{
    uiODAnnotSubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	mDynamicCastGet(visSurvey::ScaleBarDisplay*,ad,
					       visserv_->getObject(displayid_));
	ScaleBarPars pars;
	pars.oninlcrl_ = ad->isOnInlCrl();
	pars.orientation_ = ad->getOrientation();
	pars.length_ = ad->getLength();
	pars.ls_ = OD::LineStyle(OD::LineStyle::Solid,ad->getLineWidth(),
			     set_.dispColor());
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

    mDynamicCastGet(visSurvey::ScaleBarDisplay*,ad,
		    visserv_->getObject(displayid_));
    ad->setOnInlCrl( pars.oninlcrl_ );
    ad->setOrientation( pars.orientation_ );
    ad->setLineWidth( pars.ls_.width_ );
    ad->setLength( pars.length_ );
}
