/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/



static const char* rcsID = "$Id: uiseis2dto3d.cc,v 1.2 2011-05-04 13:16:21 cvsbruno Exp $";

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seis2dto3d.h"
#include "seisselection.h"
#include "seistrctr.h"

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
    
    iterfld_ = new uiGenInput( this, "Iteration number" );
    iterfld_->attach( alignedBelow, inpfld_ );

    winfld_ = new uiGenInput( this,"Window (Inl / Crl" );
    winfld_->attach( alignedBelow, iterfld_ );
    winfld_->setValue( 50 );

    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, winfld_ );
   
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

    seis2dto3d_.setInput( *inctio_.ioobj, "Seis" );

    CubeSampling cs(false); 
    outsubselfld_->getSampling( cs.hrg );
    outsubselfld_->getZRange( cs.zrg );
    seis2dto3d_.setWin( winfld_->getIntValue() );
    seis2dto3d_.setOutput( *outctio_.ioobj, cs );
    seis2dto3d_.setNrIter( iterfld_->getIntValue() );

    if ( seis2dto3d_.errMsg() )
	uiMSG().error( seis2dto3d_.errMsg() );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute( seis2dto3d_ ) )
	return seis2dto3d_.errMsg();

    return true;
}

