/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisrandto2dline.cc,v 1.7 2009-03-24 12:33:51 cvsbert Exp $";

#include "uiseisrandto2dline.h"

#include "ctxtioobj.h"
#include "linekey.h"
#include "randomlinegeom.h"
#include "seisrandlineto2d.h"
#include "seistrctr.h"
#include "survinfo.h"
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
    outpfld_->setConfirmOverwrite( false );
    outpfld_->attach( alignedBelow, inpfld_ );

    linenmfld_ = new uiGenInput( this, "Line Name", StringInpSpec(rln.name()) );
    linenmfld_->attach( alignedBelow, outpfld_ );

    trcnrfld_ = new uiGenInput( this, "First Trace Nr", IntInpSpec(1) );
    trcnrfld_->attach( alignedBelow, linenmfld_ );
}


uiSeisRandTo2DLineDlg::~uiSeisRandTo2DLineDlg()
{ delete &inctio_; delete &outctio_; }


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisRandTo2DLineDlg::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outpfld_->commitInput() )
	mErrRet("Missing Output\nPlease select a lineset for output")

    BufferString attrnm = outpfld_->attrNm();
    if ( attrnm.isEmpty() )
	mErrRet("Missing Attribute name")

    BufferString linenm = linenmfld_->text();
    if ( linenm.isEmpty() )
	mErrRet("Missing Line Name\nPlease enter a Line Name")

    const int trcnrstart = trcnrfld_->getIntValue();
    if ( mIsUdf(trcnrstart) || trcnrstart <= 0 )
	mErrRet("Please specify a valid starting trace number")

    LineKey lk( linenm, attrnm );
    SeisRandLineTo2D exec( inctio_.ioobj, outctio_.ioobj, lk, trcnrstart,
	    		   randln_ );
    uiTaskRunner dlg( this );
    if ( !dlg.execute(exec) )
	return false;
    
    if ( !SI().has2D() )
	uiMSG().warning( "You need to change survey type to 'Both 2D and 3D'"
	       		 " to display the 2D line" );

    return true;
}

