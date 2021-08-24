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
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"
#include "veldesc.h"
#include "unitofmeasure.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
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
	     .addItem( tr("Upward") );
    optsfld_->setChecked( 0, true ).setChecked( 1, true );

    uiObject* attobj = optsfld_->attachObj();
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( grp, VelocityDesc::getVelVolumeLabel(),
		FloatInpSpec(zinft_?10000.f:3000.f) );
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
	vel = velfld_->getFValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
	if ( zinft_ )
	    vel *= mFromFeetFactorF;
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
}


void uiCalcPolyHorVol::horSel( CallBacker* cb )
{
    if ( hor_ )
	{ hor_->unRef(); hor_ = 0; }

    horsel_->commitInput();
    const IOObj* ioobj = horsel_->ioobj( true );
    if ( !ioobj ) return;

    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key(),
							  &taskrunner );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( hor )
    {
	hor->ref();
	hor_ = hor;
    }

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
{
    if ( hor_.nrSections() < 1 )
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
    delete ps_;
}


void uiCalcHorPolyVol::psSel( CallBacker* cb )
{
    if ( ps_ ) delete ps_;
    ps_ = 0;

    const IOObj* ioobj = pssel_->ioobj( true );
    if ( !ioobj ) return;

    ps_ = new Pick::Set; BufferString msg;
    if ( !PickSetTranslator::retrieve(*ps_,ioobj,false,msg) )
    {
	uiMSG().error( mToUiStringTodo(msg) );
	delete ps_; ps_ = 0;
    }

    haveChg( cb );
}


const Pick::Set* uiCalcHorPolyVol::getPickSet()
{
    if ( !ps_ )
	psSel( 0 );
    return ps_;
}
