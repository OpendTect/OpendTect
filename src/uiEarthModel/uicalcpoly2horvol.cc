/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicalcpoly2horvol.h"
#include "poly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "od_helpids.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uiconstvel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"

#include <math.h>


uiCalcHorVol::uiCalcHorVol( uiParent* p,const uiString& dlgtxt )
	: uiDialog(p,Setup(tr("Calculate Volume"),dlgtxt,
			   mODHelpKey(mCalcPoly2HorVolHelpID)))
	, zinft_(SI().depthsInFeet())
{
    setCtrlStyle( CloseOnly );
}


uiCalcHorVol::~uiCalcHorVol()
{
    detachAllNotifiers();
}


uiGroup* uiCalcHorVol::mkStdGrp()
{
    uiGroup* grp = new uiGroup( this, "uiCalcHorVol group" );

    optsfld_ = new uiCheckList( grp );
    optsfld_->addItem( tr("Ignore negative thicknesses") )
	     .addItem( tr("Upward") );
    optsfld_->setChecked( 0, true ).setChecked( 1, true );

    uiObject* attachobj = optsfld_->attachObj();
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( grp, VelocityDesc::getVelVolumeLabel(),
		FloatInpSpec(Vel::getGUIDefaultVelocity()) );
	velfld_->attach( alignedBelow, optsfld_ );
	mAttachCB( velfld_->valueChanged, uiCalcHorVol::calcReq );
	attachobj = velfld_->attachObj();
    }

    const CallBack calccb( mCB(this,uiCalcHorVol,calcReq) );
    auto* calcbut = new uiPushButton( grp, uiStrings::sCalculate(),
				      calccb, true );
    calcbut->setIcon( "downarrow" );
    calcbut->attach( alignedBelow, attachobj );

    auto* sep = new uiSeparator( grp, "Hor sep" );
    sep->attach( stretchedBelow, calcbut );

    areafld_ = new uiGenInput( grp, tr("Area") );
    areafld_->attach( alignedBelow, calcbut );
    areafld_->attach( ensureBelow, sep );
    areafld_->setReadOnly( true );

    areaunitfld_ = new uiUnitSel( grp, Mnemonic::Area );
    areaunitfld_->setUnit( UoMR().get(SI().depthsInFeet() ? "mi2" : "km2"));
    mAttachCB( areaunitfld_->selChange, uiCalcHorVol::unitChgCB );
    areaunitfld_->attach( rightOf, areafld_ );

    volumefld_ = new uiGenInput( grp, tr("Volume") );
    volumefld_->setReadOnly( true );
    volumefld_->attach( alignedBelow, areafld_ );

    volumeunitfld_ = new uiUnitSel( grp, Mnemonic::Vol );
    volumeunitfld_->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( volumeunitfld_->selChange, uiCalcHorVol::unitChgCB );
    volumeunitfld_->attach( rightOf, volumefld_ );

    grp->setHAlignObj( volumefld_ );
    return grp;
}


void uiCalcHorVol::unitChgCB( CallBacker* cb )
{
    CallBacker* caller = cb ? cb->trueCaller() : nullptr;
    BufferString txt;
    if ( caller && caller==volumeunitfld_ )
    {
	const UnitOfMeasure* uom = volumeunitfld_->getUnit();
	if ( uom && !mIsUdf(volumeinm3_) )
	{
	    const float newval = uom->getUserValueFromSI( volumeinm3_ );
	    txt.set( newval, 4 );
	}

	volumefld_->setText( txt );
    }
    else if ( caller && caller==areaunitfld_ )
    {
	const UnitOfMeasure* uom = areaunitfld_->getUnit();
	if ( uom && !mIsUdf(areainm2_) )
	{
	    const float newval = uom->getUserValueFromSI( areainm2_ );
	    txt.set( newval, 4 );
	}

	areafld_->setText( txt );
    }
}


void uiCalcHorVol::haveChg( CallBacker* )
{
    if ( volumefld_ )
	volumefld_->clear();
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiCalcHorVol::calcReq( CallBacker* )
{
    const Pick::Set* ps = getPickSet();
    if ( !ps ) mErrRet( tr("No Polygon selected") );

    const EM::Horizon3D* hor = getHorizon();
    if ( !hor ) mErrRet( tr("No Horizon selected") );

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getFValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
	if ( zinft_ )
	    vel *= mFromFeetFactorF;
    }

    const bool allownegativevalues = !optsfld_->isChecked( 0 );
    const bool upward = optsfld_->isChecked( 1 );
    Poly2HorVol ph2v( ps, const_cast<EM::Horizon3D*>(hor) );
    volumeinm3_ = ph2v.getM3( vel, upward, allownegativevalues );
    unitChgCB( volumeunitfld_ );

    areainm2_ = ps->getXYArea(); // This area is in XYUnits^2
    if ( SI().xyInFeet() )
	areainm2_ *= mFromFeetFactorF * mFromFeetFactorF;

    unitChgCB( areaunitfld_ );
}



// uiCalcPolyHorVol
uiCalcPolyHorVol::uiCalcPolyHorVol( uiParent* p, const Pick::Set& ps )
	: uiCalcHorVol(p,tr("Polygon: %1").arg(ps.name()))
	, ps_(&ps)
{
    if ( ps_->size() < 3 )
    {
	new uiLabel( this, uiStrings::phrInvalid(uiStrings::sPolygon()) );
	return;
    }

    horsel_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D),
				tr("Calculate to") );
    horsel_->selectionDone.notify( mCB(this,uiCalcPolyHorVol,horSel) );

    mkStdGrp()->attach( alignedBelow, horsel_ );
}


uiCalcPolyHorVol::~uiCalcPolyHorVol()
{
}


const EM::Horizon3D* uiCalcPolyHorVol::getHorizon() const
{
    if ( !hor_ )
	cCast(uiCalcPolyHorVol*,this)->horSel( nullptr);

    return hor_;
}


void uiCalcPolyHorVol::horSel( CallBacker* cb )
{
    const IOObj* ioobj = horsel_->ioobj( true );
    if ( !ioobj )
	return;

    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj =
		EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &taskrunner );
    hor_ = sCast(EM::Horizon3D*,emobj);
    haveChg( cb );
}



// uiCalcHorPolyVol
uiCalcHorPolyVol::uiCalcHorPolyVol( uiParent* p, const EM::Horizon3D& h )
    : uiCalcHorVol(p,tr("Horizon: %1").arg(h.name()))
    , hor_(&h)
{
    if ( hor_->nrSections() < 1 )
    {
	new uiLabel( this, uiStrings::phrInvalid(uiStrings::sHorizon(1)));
	return;
    }

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    pssel_ = new uiIOObjSel( this, ctxt, uiStrings::phrCalculateFrom(
			     uiStrings::sPolygon()));
    pssel_->selectionDone.notify( mCB(this,uiCalcHorPolyVol,psSel) );

    mkStdGrp()->attach( alignedBelow, pssel_ );
}


uiCalcHorPolyVol::~uiCalcHorPolyVol()
{
}


const Pick::Set* uiCalcHorPolyVol::getPickSet() const
{
    if ( !ps_ )
	cCast(uiCalcHorPolyVol*,this)->psSel( nullptr);

    return ps_;
}


void uiCalcHorPolyVol::psSel( CallBacker* cb )
{
    const IOObj* ioobj = pssel_->ioobj( true );
    if ( !ioobj )
	return;

    BufferString msg;
    ps_ = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps_,ioobj,false,msg) )
	uiMSG().error( mToUiStringTodo(msg) );

    haveChg( cb );
}
