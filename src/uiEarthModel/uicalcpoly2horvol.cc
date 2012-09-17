/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicalcpoly2horvol.cc,v 1.12 2011/09/06 15:31:42 cvsbert Exp $";

#include "uicalcpoly2horvol.h"
#include "poly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uimsg.h"

#include <math.h>


uiCalcHorVol::uiCalcHorVol( uiParent* p, const char* dlgtxt )
	: uiDialog(p,Setup("Calculate volume",dlgtxt,"104.4.5"))
	, zinft_(SI().depthsInFeetByDefault())
	, velfld_(0)
	, valfld_(0)
{
    setCtrlStyle( LeaveOnly );
}


uiGroup* uiCalcHorVol::mkStdGrp()
{
    const CallBack chgcb( mCB(this,uiCalcHorVol,haveChg) );
    const CallBack calccb( mCB(this,uiCalcHorVol,calcReq) );

    uiGroup* grp = new uiGroup( this, "uiCalcHorVol group" );

    optsfld_ = new uiCheckList( grp, "Ignore negative thicknesses", "Upward" );
    optsfld_->setChecked( 0, true ); optsfld_->setChecked( 1, true );

    uiObject* attobj = optsfld_->attachObj();
    if ( SI().zIsTime() )
    {
	const char* txt = zinft_ ? "Velocity (ft/s)" : "Velocity (m/s)";
	velfld_ = new uiGenInput( grp, txt, FloatInpSpec(zinft_?10000:3000) );
	velfld_->attach( alignedBelow, optsfld_ );
	velfld_->valuechanged.notify( calccb );
	attobj = velfld_->attachObj();
    }

    uiSeparator* sep = new uiSeparator( grp, "Hor sep" );
    sep->attach( stretchedBelow, attobj );

    uiPushButton* calcbut = new uiPushButton( grp,
	    			"&Estimate volume", calccb, true);
    calcbut->attach( alignedBelow, attobj );
    calcbut->attach( ensureBelow, sep );

    uiGenInput* areafld = 0;
    const Pick::Set* ps = getPickSet();
    if ( ps )
    {
	const float area = ps->getXYArea();
	if ( !mIsUdf(area) )
	{
	    areafld = new uiGenInput( grp, "==> Area" );
	    areafld->attach( alignedBelow, calcbut );
	    areafld->setReadOnly( true );

	    areafld->setText( getAreaString( area, true, 0 ) );
	}
    }

    valfld_ = new uiGenInput( grp, "==> Volume" );
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
    const Pick::Set* ps = getPickSet(); if ( !ps ) return;
    const EM::Horizon3D* hor = getHorizon(); if ( !hor ) return;

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getfValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet("Please provide the velocity")
	if ( zinft_ )
	    vel *= mFromFeetFactor;
    }

    Poly2HorVol ph2v( ps, const_cast<EM::Horizon3D*>(hor) );
    float m3 = ph2v.getM3( vel, optsfld_->isChecked(0),
	    			!optsfld_->isChecked(1));
    valfld_->setText( ph2v.dispText(m3,zinft_) );
}


uiCalcPolyHorVol::uiCalcPolyHorVol( uiParent* p, const Pick::Set& ps )
	: uiCalcHorVol(p,"Volume estimation: polygon to horizon")
	, ps_(ps)
	, hor_(0)
{
    if ( ps_.size() < 3 )
	{ new uiLabel( this, "Invalid polygon" ); return; }

    horsel_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D),
	    			"Calculate to" );
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
    const IOObj* ioobj = horsel_->ioobj();
    if ( !ioobj ) return;

    uiTaskRunner tr( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key(),
							  &tr );
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
	: uiCalcHorVol(p,"Volume estimation from horizon part")
	, ps_(0)
	, hor_(h)
{
    if ( hor_.nrSections() < 1 )
	{ new uiLabel( this, "Invalid horizon" ); return; }

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.toselect.require_.set( sKey::Type, sKey::Polygon );
    pssel_ = new uiIOObjSel( this, ctxt, "Calculate from polygon" );
    pssel_->selectionDone.notify( mCB(this,uiCalcHorPolyVol,psSel) );

    mkStdGrp()->attach( alignedBelow, pssel_ );
}


uiCalcHorPolyVol::~uiCalcHorPolyVol()
{
    delete ps_;
}


void uiCalcHorPolyVol::psSel( CallBacker* cb )
{
    bool havenew = ps_;
    if ( ps_ ) delete ps_;
    ps_ = 0;

    const IOObj* ioobj = pssel_->ioobj();
    if ( !ioobj ) return;

    ps_ = new Pick::Set; BufferString msg; 
    if ( !PickSetTranslator::retrieve(*ps_,ioobj,false,msg) )
    {
	uiMSG().error( msg );
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
