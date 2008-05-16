/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.cc,v 1.1 2008-05-16 11:38:10 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiseisrandto2dline.h"

#include "ctxtioobj.h"
#include "linekey.h"
#include "seisrandlineto2d.h"
#include "seistrctr.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uitaskrunner.h"

uiSeisRandTo2DLineDlg::uiSeisRandTo2DLineDlg( uiParent* p,
					      const Geometry::RandomLine& rln )
    : uiDialog(p,uiDialog::Setup("Save 2D Line","Specify parameters",""))
    , inctio_(*mMkCtxtIOObj(SeisTrc))
    , outctio_(*mMkCtxtIOObj(SeisTrc))
    , randln_(rln)
{
    inctio_.ctxt.forread = true;
    inpfld_ = new uiSeisSel( this, inctio_, uiSeisSel::Setup(Seis::Vol) );

    outctio_.ctxt.forread = false;
    outpfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Line) );
    outpfld_->attach( alignedBelow, inpfld_ );

    linenmfld_ = new uiGenInput( this, "Line Name", StringInpSpec() );
    linenmfld_->attach( alignedBelow, outpfld_ );

    trcnrfld_ = new uiGenInput( this, "Trace Nr (Start/Step)",
	    			IntInpIntervalSpec() );
    trcnrfld_->attach( alignedBelow, linenmfld_ );
}


uiSeisRandTo2DLineDlg::~uiSeisRandTo2DLineDlg()
{ delete &inctio_; delete &outctio_; }


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisRandTo2DLineDlg::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outpfld_->commitInput(true) )
	mErrRet("Missing Output\nPlease select a lineset for output")

    BufferString attrnm = outpfld_->attrNm();
    if ( attrnm.isEmpty() )
	mErrRet("Missing Attribute name")

    BufferString linenm = linenmfld_->text();
    if ( linenm.isEmpty() )
	mErrRet("Missing Line Name\nPlease enter a Line Name")

    Interval<int> trcinp = trcnrfld_->getIInterval();
    if ( mIsUdf(trcinp.start) || trcinp.start <= 0
	 || mIsUdf(trcinp.stop) || trcinp.stop <= 0 )
	mErrRet("Please specify how you want the traces to be numbered")

    LineKey lk( linenm, attrnm );
    SeisRandLineTo2D exec( inctio_.ioobj, outctio_.ioobj, lk, trcinp, randln_ );
    uiTaskRunner dlg( this );
    return dlg.execute( exec );
}

