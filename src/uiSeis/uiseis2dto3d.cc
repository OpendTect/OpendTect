/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seis2dto3d.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseis2dto3d.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

uiSeis2DTo3D::uiSeis2DTo3D( uiParent* p )
	: uiDialog( p, Setup( tr("create 3D cube from to 2D LineSet"),
			      mNoDlgTitle,
			      mODHelpKey(mSeis2DTo3DHelpID) ) )
	, seis2dto3d_(*new Seis2DTo3D)
{
    const IOObjContext inctxt( uiSeisSel::ioContext( Seis::Line, false ) );
    inpfld_ = new uiSeisSel( this, inctxt, uiSeisSel::Setup( Seis::Line ) );
    interpoltypefld_ = new uiGenInput( this, tr("Type of interpolation"),
			               BoolInpSpec(true,tr("Nearest trace"),
                                       tr("FFT based")) );
    interpoltypefld_->attach( alignedBelow, inpfld_ );
    interpoltypefld_->valuechanged.notify(mCB(this,uiSeis2DTo3D,typeChg));

    winfld_ = new uiGenInput( this,tr("Interpolation window (Inl/Crl)"),
							IntInpIntervalSpec() );
    winfld_->attach( alignedBelow, interpoltypefld_ );
    winfld_->setValue( Interval<float>(150,150) );

    reusetrcsbox_ = new uiCheckBox( this, tr("Re-use interpolated traces") );
    reusetrcsbox_->attach( alignedBelow, winfld_ );

    velfiltfld_ = new uiGenInput( this, tr("Maximum velocity to pass (m/s)") );
    velfiltfld_->setValue( 2000 );
    velfiltfld_->attach( alignedBelow, reusetrcsbox_ );

    const IOObjContext outctxt( uiSeisSel::ioContext( Seis::Vol, false ) );
    outfld_ = new uiSeisSel( this, outctxt, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, velfiltfld_ );

    outsubselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(Seis::Vol) );
    outsubselfld_->attachObj()->attach( alignedBelow, outfld_ );

    typeChg( 0 );
}


uiSeis2DTo3D::~uiSeis2DTo3D()
{
    delete &seis2dto3d_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3D::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    const IOObj* outioobj = outfld_->ioobj();
    if ( !inioobj || !outioobj )
	return false;

    seis2dto3d_.setInput( *inioobj );

    CubeSampling cs(false);
    outsubselfld_->getSampling( cs.hrg );
    outsubselfld_->getZRange( cs.zrg );

    const int wininlstep = winfld_->getIInterval().start;
    const int wincrlstep = winfld_->getIInterval().stop;
    const float maxvel = velfiltfld_->getfValue();
    const bool reusetrcs = reusetrcsbox_->isChecked();

    seis2dto3d_.setParams( wininlstep, wincrlstep, maxvel, reusetrcs );
    seis2dto3d_.setOutput( const_cast<IOObj&>(*outioobj), cs );
    seis2dto3d_.setIsNearestTrace( interpoltypefld_->getBoolValue() );

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, seis2dto3d_ ) )
	return false;

    if ( !SI().has3D() )
	uiMSG().warning( tr("3D cube created successfully. "
			 "You need to change survey type to 'Both 2D and 3D' "
			 "in survey setup to display/use the cube") );
    return true;
}



void uiSeis2DTo3D::typeChg( CallBacker* )
{
    bool isfft = !interpoltypefld_->getBoolValue();
    winfld_->display( isfft );
    reusetrcsbox_->display( isfft );
    velfiltfld_->display( isfft );
}
