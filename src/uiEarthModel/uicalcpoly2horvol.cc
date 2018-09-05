/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/


#include "uicalcpoly2horvol.h"
#include "poly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "picksetmanager.h"
#include "survinfo.h"
#include "veldesc.h"
#include "unitofmeasure.h"

#include "uiconstvel.h"
#include "uiioobjsel.h"
#include "uipicksetsel.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunnerprovider.h"
#include "uilabel.h"
#include "uimsg.h"
#include "od_helpids.h"

#include <math.h>


uiCalcHorVol::uiCalcHorVol( uiParent* p,const uiString& dlgtxt )
	: uiDialog(p,Setup(tr("Calculate volume"),dlgtxt,
                    mODHelpKey(mCalcPoly2HorVolHelpID) ))
	, zinft_(SI().depthsInFeet())
	, velfld_(0)
	, valfld_(0)
{
    setCtrlStyle( CloseOnly );
}


uiGroup* uiCalcHorVol::mkStdGrp()
{
    const CallBack calccb( mCB(this,uiCalcHorVol,calcReq) );

    uiGroup* grp = new uiGroup( this, "uiCalcHorVol group" );

    optsfld_ = new uiCheckList( grp );
    optsfld_->addItem( tr("Ignore negative thicknesses") )
	     .addItem( uiStrings::sUpward() );
    optsfld_->setChecked( 0, true ).setChecked( 1, true );

    uiObject* attobj = optsfld_->attachObj();
    if ( SI().zIsTime() )
    {
	velfld_ = new uiConstantVel( grp, Vel::getGUIDefaultVelocity()*1.25f );
	velfld_->attach( alignedBelow, optsfld_ );
	velfld_->valuechanged.notify( calccb );
	attobj = velfld_->attachObj();
    }

    uiSeparator* sep = new uiSeparator( grp, "Hor sep" );
    sep->attach( stretchedBelow, attobj );

    uiPushButton* calcbut = new uiPushButton( grp,
				tr("Estimate volume"), calccb, true);
    calcbut->attach( alignedBelow, attobj );
    calcbut->attach( ensureBelow, sep );

    uiGenInput* areafld = 0;
    const Pick::Set* ps = getPickSet();
    if ( ps )
    {
	const float area = ps->getXYArea();
	if ( !mIsUdf(area) )
	{
	    areafld = new uiGenInput( grp, tr("==> Area") );
	    areafld->attach( alignedBelow, calcbut );
	    areafld->setReadOnly( true );
	    areafld->setText( getAreaString(area,SI().xyInFeet(),2,true) );
	}
    }

    valfld_ = new uiGenInput( grp, tr("==> Volume") );
    valfld_->setElemSzPol( uiObject::WideMax );
    if ( areafld )
	valfld_->attach( alignedBelow, areafld );
    else
	valfld_->attach( alignedBelow, calcbut );

    valfld_->setReadOnly( true );

    grp->setHAlignObj( attobj );
    return grp;
}


void uiCalcHorVol::haveChg( CallBacker* )
{
    if ( valfld_ ) valfld_->clear();
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
	vel = velfld_->getInternalVelocity();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
    }

    Poly2HorVol ph2v( ps, const_cast<EM::Horizon3D*>(hor) );
    const float m3 = ph2v.getM3( vel, optsfld_->isChecked(0),
				 !optsfld_->isChecked(1) );
    valfld_->setText( ph2v.dispText(m3,SI().xyInFeet()) );
}


uiCalcPolyHorVol::uiCalcPolyHorVol( uiParent* p, const Pick::Set& ps )
	: uiCalcHorVol(p, tr("Volume estimation: polygon to horizon") )
	, ps_(ps)
	, hor_(0)
{
    ps_.ref();
    if ( ps_.size() < 3 )
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
    if ( hor_ )
	hor_->unRef();
    ps_.unRef();
}


void uiCalcPolyHorVol::horSel( CallBacker* cb )
{
    if ( hor_ )
	{ hor_->unRef(); hor_ = 0; }

    horsel_->commitInput();
    const IOObj* ioobj = horsel_->ioobj( true );
    if ( !ioobj ) return;

    uiTaskRunnerProvider trprov( this );
    EM::Object* emobj =
		EM::Hor3DMan().loadIfNotFullyLoaded( ioobj->key(), trprov );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( hor )
	{ hor->ref(); hor_ = hor; }

    haveChg( cb );
}


const EM::Horizon3D* uiCalcPolyHorVol::getHorizon()
{
    if ( !hor_ )
	horSel( 0 );
    return hor_;
}


uiCalcHorPolyVol::uiCalcHorPolyVol( uiParent* p, const EM::Horizon3D& h )
	: uiCalcHorVol(p,tr("Volume estimation from horizon part") )
	, ps_(0)
	, hor_(h)
	, pssel_(0)
{
    hor_.ref();

    uiIOObjSel::Setup su( uiStrings::phrCalculateFrom(uiStrings::sPolygon()) );
    pssel_ = new uiPickSetIOObjSel( this, su,
				    true, uiPickSetIOObjSel::PolygonOnly );
    pssel_->selectionDone.notify( mCB(this,uiCalcHorPolyVol,psSel) );

    mkStdGrp()->attach( alignedBelow, pssel_ );
}


uiCalcHorPolyVol::~uiCalcHorPolyVol()
{
    hor_.unRef();
}


void uiCalcHorPolyVol::psSel( CallBacker* cb )
{
    ps_ = pssel_->getPickSet( true );
    haveChg( cb );
}


const Pick::Set* uiCalcHorPolyVol::getPickSet()
{
    if ( !ps_ )
	psSel( 0 );
    return ps_;
}
