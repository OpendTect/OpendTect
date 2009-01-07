/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          Aug 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerposdlg.cc,v 1.10 2009-01-07 16:06:19 cvsyuancheng Exp $";

#include "uipsviewerposdlg.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "visprestackviewer.h"
#include "visseis2ddisplay.h"


namespace PreStackView
{


uiViewer3DPositionDlg::uiViewer3DPositionDlg( uiParent* p,
					      PreStackView::Viewer3D& vwr )
    : uiDialog( p, Setup(vwr.getObjectName(),"Viewer position",
			 mTODOHelpID).modal(false) )
    , viewer_(vwr)  
    , applybox_(0)
    , applybut_(0)
{
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
		    is3D() ? (isInl() ? "Crossline" : "Inline") : "Trace Nr",
		    0 , "Position" );
    posfld_ = lsb->box();
    StepInterval<int> posspos = vwr.getTraceRange();
    if ( posspos.isUdf() ) posspos = StepInterval<int>( 1, mUdf(int), 1 );
    posfld_->setInterval( posspos );

    uiLabeledSpinBox* steplsb = new uiLabeledSpinBox(this,"Step",0,"Step");
    steplsb->attach( rightOf, lsb );
    stepfld_ = steplsb->box();
    stepfld_->setInterval( StepInterval<int>(posspos.step, 
		posspos.stop-posspos.start,posspos.step) );
    stepfld_->valueChanged.notify( mCB(this,uiViewer3DPositionDlg,stepCB) );

    applybox_ = new uiCheckBox( this, "Apply immediately",
	    			mCB(this,uiViewer3DPositionDlg,boxSel) );
    applybox_->attach( alignedBelow, lsb );
    applybut_ = new uiPushButton( this, "&Apply now", true );
    applybut_->attach( leftOf, applybox_ );
    applybut_->activated.notify( mCB(this,uiViewer3DPositionDlg,applyCB) );

    finaliseDone.notify( mCB(this,uiViewer3DPositionDlg,atStart) );
    viewer_.draggermoving.notify( mCB(this,uiViewer3DPositionDlg,renewFld) );
}


bool uiViewer3DPositionDlg::is3D() const
{ return viewer_.is3DSeis(); }


bool uiViewer3DPositionDlg::isInl() const
{ return viewer_.isOrientationInline(); }


void uiViewer3DPositionDlg::renewFld( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() :
	    (isInl() ? viewer_.draggerPosition().crl 
	     	     : viewer_.draggerPosition().inl) );
}


void uiViewer3DPositionDlg::stepCB( CallBacker* )
{ posfld_->setStep( stepfld_->getValue() ); }


void uiViewer3DPositionDlg::atStart( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() : 
	(isInl() ? viewer_.getPosition().crl : viewer_.getPosition().inl) ) ;

    applybox_->setChecked( true );
    applybut_->display( false );

    applybox_->setChecked( true );
    posfld_->valueChanging.notify( mCB(this,uiViewer3DPositionDlg,posChg) );
}


void uiViewer3DPositionDlg::boxSel( CallBacker* c )
{
    if ( applybut_ && applybox_ )
	applybut_->display( !applybox_->isChecked() );
}


void uiViewer3DPositionDlg::posChg( CallBacker* c )
{
    if ( applybox_->isChecked() && 
	 viewer_.draggerPosition()==viewer_.getPosition() )
    {
	applyCB( c ); //This will adjust to the nearest position.
    }
}


bool uiViewer3DPositionDlg::applyCB( CallBacker* )
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


bool uiViewer3DPositionDlg::acceptOK( CallBacker* c )
{
    return applyCB( c );
}


}; //namespace

