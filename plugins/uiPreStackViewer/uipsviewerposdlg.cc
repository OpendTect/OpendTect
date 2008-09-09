/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          Aug 2008
 RCS:           $Id: uipsviewerposdlg.cc,v 1.2 2008-09-09 10:52:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uipsviewerposdlg.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "visprestackviewer.h"
#include "visseis2ddisplay.h"


namespace PreStackView
{


uiPSViewerPositionDlg::uiPSViewerPositionDlg( uiParent* p, PreStackViewer& vwr )
    : uiDialog( p, Setup(vwr.getObjectName(),"PreStack viewer position",
			 mTODOHelpID) )
    , viewer_( vwr )  
{
    const bool is3d = viewer_.is3DSeis();
    const bool isinl = viewer_.isOrientationInline();
    posfld_ = new uiGenInput( this, is3d ? (isinl ? "Crossline" : "Inline") 
	    				 : "Trace Nr" );
    posfld_->setValue( !is3d ? viewer_.traceNr() : 
	    (isinl ? viewer_.getPosition().crl : viewer_.getPosition().inl) ) ;
    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->activated.notify( mCB(this,uiPSViewerPositionDlg,applyCB) );
    applybut_->attach( centeredBelow, posfld_ );
}


void uiPSViewerPositionDlg::applyCB( CallBacker* )
{
    if ( viewer_.is3DSeis() )
    {
	BinID newpos = viewer_.getPosition();
	const int inlcrl = posfld_->getIntValue();
	
	if ( viewer_.isOrientationInline() )
	{
	    const StepInterval<int> crlrg = SI().crlRange( true );
	    if ( crlrg.includes( inlcrl ) )
	    	newpos.crl = posfld_->getIntValue();
	    else
	    {
		BufferString msg = "The crossline should be between ";
		msg += crlrg.start;
		msg += " and ";
		msg += crlrg.stop;
		uiMSG().error( msg );
		return;
	    }
	}
	else
	{
	    const StepInterval<int> inlrg = SI().inlRange( true );
	    if ( inlrg.includes( inlcrl ) )
	    	newpos.inl = posfld_->getIntValue();
	    else
	    {
		BufferString msg = "The inline should be between ";
		msg += inlrg.start;
		msg += " and ";
		msg += inlrg.stop;
		uiMSG().error( msg );
		return;
	    }
	}

	viewer_.setPosition( newpos );
    }
    else
    {
	const int tracenr = posfld_->getIntValue();
	if ( !viewer_.getSeis2DDisplay() )
	    return;

	const Interval<int> trcrg = 
	    viewer_.getSeis2DDisplay()->getTraceNrRange();
	if ( !trcrg.includes(tracenr) )
	{
	    BufferString msg = "The trace number should be between ";
	    msg += trcrg.start;
	    msg += " and ";
	    msg += trcrg.stop;
	    uiMSG().error( msg );
	    return;
	}

	viewer_.setTraceNr( tracenr );
    }
}


bool uiPSViewerPositionDlg::acceptOK( CallBacker* )
{
    if ( viewer_.is3DSeis() )
    {
	BinID newpos = viewer_.getPosition();
	const int inlcrl = posfld_->getIntValue();
	
	if ( viewer_.isOrientationInline() )
	{
	    const StepInterval<int> crlrg = SI().crlRange( true );
	    if ( crlrg.includes( inlcrl ) )
	    	newpos.crl = posfld_->getIntValue();
	    else
	    {
		BufferString msg = "The crossline should be between ";
		msg += crlrg.start;
		msg += " and ";
		msg += crlrg.stop;
		uiMSG().error( msg );
		return false;
	    }
	}
	else
	{
	    const StepInterval<int> inlrg = SI().inlRange( true );
	    if ( inlrg.includes( inlcrl ) )
	    	newpos.inl = posfld_->getIntValue();
	    else
	    {
		BufferString msg = "The inline should be between ";
		msg += inlrg.start;
		msg += " and ";
		msg += inlrg.stop;
		uiMSG().error( msg );
		return false;
	    }
	}

	viewer_.setPosition( newpos );
    }
    else
    {
	const int tracenr = posfld_->getIntValue();
	if ( !viewer_.getSeis2DDisplay() )
	    return false;

	const Interval<int> trcrg = 
	    viewer_.getSeis2DDisplay()->getTraceNrRange();
	if ( !trcrg.includes(tracenr) )
	{
	    BufferString msg = "The trace number should be between ";
	    msg += trcrg.start;
	    msg += " and ";
	    msg += trcrg.stop;
	    uiMSG().error( msg );
	    return false;
	}

	viewer_.setTraceNr( tracenr );
    }

    return true;
}


}; //namespace

