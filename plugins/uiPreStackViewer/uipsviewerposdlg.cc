/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          Aug 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerposdlg.cc,v 1.6 2008-12-18 15:30:20 cvsyuancheng Exp $";

#include "uipsviewerposdlg.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "visprestackviewer.h"
#include "visseis2ddisplay.h"


namespace PreStackView
{


uiPSViewerPositionDlg::uiPSViewerPositionDlg( uiParent* p, PreStackViewer& vwr )
    : uiDialog( p, Setup(vwr.getObjectName(),"PreStack viewer position",
			 mTODOHelpID) )
    , viewer_(vwr)  
    , applybox_(0)
    , applybut_(0)
{
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
		    is3D() ? (isInl() ? "Crossline" : "Inline") : "Trace Nr",
		    0 , "Position" );
    posfld_ = lsb->box();
    StepInterval<int> posspos( 1, mUdf(int), 1 );
    //TODO get the possible positions and step here
    posfld_->setInterval( posspos );

    applybox_ = new uiCheckBox( this, "Apply immediately",
	    			mCB(this,uiPSViewerPositionDlg,boxSel) );
    applybox_->attach( alignedBelow, lsb );
    applybut_ = new uiPushButton( this, "&Apply now", true );
    applybut_->attach( leftOf, applybox_ );
    applybut_->activated.notify( mCB(this,uiPSViewerPositionDlg,applyCB) );

    finaliseDone.notify( mCB(this,uiPSViewerPositionDlg,atStart) );
}


bool uiPSViewerPositionDlg::is3D() const
{ return viewer_.is3DSeis(); }


bool uiPSViewerPositionDlg::isInl() const
{ return viewer_.isOrientationInline(); }


void uiPSViewerPositionDlg::atStart( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() : 
	(isInl() ? viewer_.getPosition().crl : viewer_.getPosition().inl) ) ;

    applybox_->setChecked( true );
    applybut_->display( false );

    applybox_->setChecked( true );
    posfld_->valueChanging.notify( mCB(this,uiPSViewerPositionDlg,posChg) );
}


void uiPSViewerPositionDlg::boxSel( CallBacker* c )
{
    if ( applybut_ && applybox_ )
	applybut_->display( !applybox_->isChecked() );
}


void uiPSViewerPositionDlg::posChg( CallBacker* c )
{
    //TODO : correct to nearest existing position

    if ( applybox_->isChecked() )
	applyCB( c );
}


bool uiPSViewerPositionDlg::applyCB( CallBacker* )
{
    const int location = posfld_->getValue();
    if ( is3D() )
    {
	BinID newpos = viewer_.getPosition();
	
	if ( isInl() )
	{
	    if ( newpos.crl==location )
		return true;

	    const StepInterval<int> crlrg = SI().crlRange( true );
	    if ( crlrg.includes( location ) )
	    	newpos.crl = location;
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
	    if ( newpos.inl==location )
		return true;

	    const StepInterval<int> inlrg = SI().inlRange( true );
	    if ( inlrg.includes( location ) )
	    	newpos.inl = location;
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

	const BinID bid = viewer_.getPosition();
	posfld_->setValue( isInl() ? bid.crl : bid.inl );
    }
    else
    {
	if ( !viewer_.getSeis2DDisplay() || location==viewer_.traceNr() )
	    return true;

	const Interval<int> trcrg = 
	    viewer_.getSeis2DDisplay()->getTraceNrRange();
	if ( !trcrg.includes(location) )
	{
	    BufferString msg = "The trace number should be between ";
	    msg += trcrg.start;
	    msg += " and ";
	    msg += trcrg.stop;
	    uiMSG().error( msg );
	    return false;
	}

	viewer_.setTraceNr( location );
	posfld_->setValue( viewer_.traceNr() );
    }

    return true;
}


bool uiPSViewerPositionDlg::acceptOK( CallBacker* c )
{
    return applyCB( c );
}


}; //namespace

