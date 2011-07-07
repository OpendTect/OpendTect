/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/



static const char* rcsID = "$Id: uiseis2dto3d.cc,v 1.5 2011-07-07 10:42:48 cvsbruno Exp $";

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seis2dto3d.h"
#include "seisselection.h"
#include "seistrctr.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseis2dto3d.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

uiSeis2DTo3D::uiSeis2DTo3D( uiParent* p )
	: uiDialog( p, Setup( "create 3D cube from to 2D LineSet",
			      "Specify process parameters",
			      mTODOHelpID) )
    	, inctio_(*mMkCtxtIOObj(SeisTrc))
    	, outctio_((*uiSeisSel::mkCtxtIOObj(Seis::Vol,false)))
	, seis2dto3d_(*new Seis2DTo3D)	 
{
    inpfld_ = new uiSeisSel( this, inctio_, uiSeisSel::Setup( Seis::Line ) );
    
    winfld_ = new uiGenInput( this,"Interpolation window (Inl/Crl)", 
							IntInpIntervalSpec() );
    winfld_->attach( alignedBelow, inpfld_ );
    winfld_->setValue( Interval<float>(150,150) );

    reusetrcsbox_ = new uiCheckBox( this, "Re-use interpolated traces" );
    reusetrcsbox_->attach( alignedBelow, winfld_ );

    velfiltfld_ = new uiGenInput( this, "Maximum velocity to pass (m/s)" );
    velfiltfld_->setValue( 2000 );
    velfiltfld_->attach( alignedBelow, reusetrcsbox_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, velfiltfld_ );

    outsubselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(Seis::Vol) );
    outsubselfld_->attachObj()->attach( alignedBelow, outfld_ );
}


uiSeis2DTo3D::~uiSeis2DTo3D()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete &seis2dto3d_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3D::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput() )
	mErrRet("Missing Output\nPlease enter a name for the output seismics")
    else if ( outctio_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	return false;

    seis2dto3d_.setInput( *inctio_.ioobj, inpfld_->attrNm() );

    CubeSampling cs(false); 
    outsubselfld_->getSampling( cs.hrg );
    outsubselfld_->getZRange( cs.zrg );

    const int wininlstep = winfld_->getIInterval().start;
    const int wincrlstep = winfld_->getIInterval().stop;
    const float maxvel = velfiltfld_->getfValue();
    const bool reusetrcs = reusetrcsbox_->isChecked();

    seis2dto3d_.setParams( wininlstep, wincrlstep, maxvel, reusetrcs );
    seis2dto3d_.setOutput( *outctio_.ioobj, cs );

    if ( seis2dto3d_.errMsg() )
	uiMSG().error( seis2dto3d_.errMsg() );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute( seis2dto3d_ ) )
	return seis2dto3d_.errMsg();

    return true;
}

