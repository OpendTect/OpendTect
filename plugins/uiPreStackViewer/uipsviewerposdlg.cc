/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          Aug 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
					      visSurvey::PreStackDisplay& vwr )
    : uiDialog( p, Setup(vwr.getObjectName(),mNoDlgTitle,"50.2.1")
	    		.modal(false) )
    , viewer_(vwr)  
    , applybox_(0)
    , applybut_(0)
{
    ootxt_ = is3D() ? (isInl() ? "Crossline" : "Inline") : "Trace Nr";
    setCtrlStyle( LeaveOnly );

    oobox_ = new uiCheckBox( this, ootxt_ );
    oobox_->setChecked( true );
    oobox_->activated.notify( mCB(this,uiViewer3DPositionDlg,ooBoxSel) );

    posfld_ = new uiSpinBox( this, 0, "Position" );
    posfld_->attach( rightOf, oobox_ );
    StepInterval<int> posspos = vwr.getTraceRange( vwr.getPosition() );
    if ( posspos.isUdf() ) posspos = StepInterval<int>( 1, mUdf(int), 1 );
    posfld_->setInterval( posspos );

    stepfld_ = new uiLabeledSpinBox(this,"Step",0,"Step");
    stepfld_->attach( rightOf, posfld_ );
    stepfld_->box()->setInterval( StepInterval<int>(posspos.step, 
		posspos.stop-posspos.start,posspos.step) );
    stepfld_->box()->valueChanged.notify(
	    		mCB(this,uiViewer3DPositionDlg,stepCB) );

    applybox_ = new uiCheckBox( this, "Immediate update",
	    			mCB(this,uiViewer3DPositionDlg,applBoxSel) );
    applybox_->attach( rightOf, stepfld_ );
    applybut_ = new uiPushButton( this, "&Update Now", true );
    applybut_->attach( rightBorder );
    applybut_->activated.notify( mCB(this,uiViewer3DPositionDlg,applyCB) );

    postFinalise().notify( mCB(this,uiViewer3DPositionDlg,atStart) );
    viewer_.draggermoving.notify( mCB(this,uiViewer3DPositionDlg,renewFld) );
}


uiViewer3DPositionDlg::~uiViewer3DPositionDlg()
{
    viewer_.draggermoving.remove( mCB(this,uiViewer3DPositionDlg,renewFld) );
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
{ posfld_->setStep( stepfld_->box()->getValue() ); }


void uiViewer3DPositionDlg::atStart( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() : 
	(isInl() ? viewer_.getPosition().crl : viewer_.getPosition().inl) ) ;

    applybox_->setChecked( true );
    applybut_->display( false );

    applybox_->setChecked( true );
    posfld_->valueChanging.notify( mCB(this,uiViewer3DPositionDlg,posChg) );
}


void uiViewer3DPositionDlg::ooBoxSel( CallBacker* c )
{
    const bool dodisp = oobox_->isChecked();
    oobox_->setText( dodisp ? ootxt_ : "Display" );
    viewer_.turnOn( dodisp );

    posfld_->display( dodisp );
    stepfld_->display( dodisp );
    applybut_->display( dodisp );
    applybox_->display( dodisp );
    if ( dodisp ) applBoxSel( c );
}


void uiViewer3DPositionDlg::applBoxSel( CallBacker* c )
{
    if ( applybut_ && applybox_ )
    {
	const bool nowon = applybox_->isChecked();
	applybut_->display( !nowon );
	if ( nowon )
	    applyCB( c );
    }
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
	    if ( crlrg.includes( location, false ) )
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
	    if ( inlrg.includes( location, false ) )
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
	if ( !trcrg.includes(location, true) )
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


bool uiViewer3DPositionDlg::rejectOK( CallBacker* )
{
    oobox_->setChecked( true );
    return true;
}


}; //namespace

