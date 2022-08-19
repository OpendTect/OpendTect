/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewerposdlg.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "visprestackdisplay.h"
#include "visseis2ddisplay.h"
#include "visplanedatadisplay.h"
#include "od_helpids.h"


namespace PreStackView
{


uiViewer3DPositionDlg::uiViewer3DPositionDlg( uiParent* p,
					      visSurvey::PreStackDisplay& vwr )
    : uiDialog( p, Setup(toUiString(vwr.getObjectName()),mNoDlgTitle,
                         mODHelpKey(mViewer3DPositionsDlgHelpID) )
			.modal(false) )
    , viewer_(vwr)
    , applybox_(0)
    , applybut_(0)
{
    ootxt_ = is3D() ? (isInl() ? uiStrings::sCrossline() : uiStrings::sInline())
                               : uiStrings::sTraceNumber();
    setCtrlStyle( CloseOnly );

    oobox_ = new uiCheckBox( this, ootxt_ );
    oobox_->setChecked( true );
    oobox_->activated.notify( mCB(this,uiViewer3DPositionDlg,ooBoxSel) );

    posfld_ = new uiSpinBox( this, 0, "Position" );
    posfld_->attach( rightOf, oobox_ );
    StepInterval<int> posspos = vwr.getTraceRange( vwr.getPosition() );
    if ( posspos.isUdf() ) posspos = StepInterval<int>( 1, mUdf(int), 1 );
    posfld_->setInterval( posspos );

    stepfld_ = new uiLabeledSpinBox(this,uiStrings::sStep(),0,"Step");
    stepfld_->attach( rightOf, posfld_ );
    stepfld_->box()->setInterval( StepInterval<int>(posspos.step,
		posspos.stop-posspos.start,posspos.step) );
    stepfld_->box()->valueChanged.notify(
			mCB(this,uiViewer3DPositionDlg,stepCB) );

    applybox_ = new uiCheckBox( this, tr("Immediate update"),
				mCB(this,uiViewer3DPositionDlg,applBoxSel) );
    applybox_->attach( alignedBelow, oobox_ );
    applybut_ = new uiPushButton( this, tr("Update Now"), true );
    applybut_->attach( rightTo, applybox_ );
    applybut_->activated.notify( mCB(this,uiViewer3DPositionDlg,applyCB) );

    postFinalize().notify( mCB(this,uiViewer3DPositionDlg,atStart) );
    if ( viewer_.getSectionDisplay() )
    {
	mAttachCB( viewer_.getSectionDisplay()->getMovementNotifier(),
		   uiViewer3DPositionDlg::sectionChangedCB );
    }

    mAttachCB( viewer_.draggermoving, uiViewer3DPositionDlg::renewFld );
}


uiViewer3DPositionDlg::~uiViewer3DPositionDlg()
{
    detachAllNotifiers();
}


bool uiViewer3DPositionDlg::is3D() const
{ return viewer_.is3DSeis(); }


bool uiViewer3DPositionDlg::isInl() const
{ return viewer_.isOrientationInline(); }


void uiViewer3DPositionDlg::sectionChangedCB( CallBacker* )
{
    updateFieldDisplay();
}


void uiViewer3DPositionDlg::updateFieldDisplay()
{
    const StepInterval<int> psdatarg =
	viewer_.getTraceRange( viewer_.getBinID() );
    const bool haspsdata = psdatarg!=Interval<int>::udf();
    posfld_->setSensitive( haspsdata );
    stepfld_->setSensitive( haspsdata );
    oobox_->setSensitive( haspsdata );
    applybox_->setSensitive( haspsdata );
    applybut_->setSensitive( haspsdata );
}


void uiViewer3DPositionDlg::renewFld( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() :
	    (isInl() ? viewer_.draggerPosition().crl()
		     : viewer_.draggerPosition().inl()) );
}


void uiViewer3DPositionDlg::stepCB( CallBacker* )
{ posfld_->setStep( stepfld_->box()->getIntValue() ); }


void uiViewer3DPositionDlg::atStart( CallBacker* )
{
    posfld_->setValue( !is3D() ? viewer_.traceNr() :
	(isInl() ? viewer_.getPosition().crl() : viewer_.getPosition().inl()) );

    applybox_->setChecked( true );
    applybut_->display( false );

    applybox_->setChecked( true );
    posfld_->valueChanging.notify( mCB(this,uiViewer3DPositionDlg,posChg) );
}


void uiViewer3DPositionDlg::ooBoxSel( CallBacker* c )
{
    const bool dodisp = oobox_->isChecked();
    oobox_->setText( dodisp ? ootxt_ : uiStrings::sDisplay() );
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


void uiViewer3DPositionDlg::applyCB( CallBacker* )
{
    const int location = posfld_->getIntValue();
    if ( is3D() )
    {
	BinID newpos = viewer_.getPosition();

	if ( isInl() )
	{
	    if ( newpos.crl()==location )
		return;

	    const StepInterval<int> crlrg = SI().crlRange( true );
	    if ( crlrg.includes( location, false ) )
		newpos.crl() = location;
	    else
	    {
		uiString msg = tr("The cross-line should be between %1 and %2")
			     .arg(crlrg.start).arg(crlrg.stop);
		uiMSG().error( msg );
		return;
	    }
	}
	else
	{
	    if ( newpos.inl()==location )
		return;

	    const StepInterval<int> inlrg = SI().inlRange( true );
	    if ( inlrg.includes( location, false ) )
		newpos.inl() = location;
	    else
	    {
		uiString msg = tr("The in-line should be between %1 and %2")
			     .arg(inlrg.start).arg(inlrg.stop);
		uiMSG().error( msg );
		return;
	    }
	}

	viewer_.setPosition( TrcKey(newpos) );

	const BinID bid = viewer_.getPosition();
	posfld_->setValue( isInl() ? bid.crl() : bid.inl() );
    }
    else
    {
	if ( !viewer_.getSeis2DDisplay() || location==viewer_.traceNr() )
	    return;

	const Interval<int> trcrg =
	    viewer_.getSeis2DDisplay()->getTraceNrRange();
	if ( !trcrg.includes(location, true) )
	{
	    uiString msg = tr("The trace number should be between %1 and %2")
			 .arg(trcrg.start).arg(trcrg.stop);
	    uiMSG().error( msg );
	    return;
	}

	viewer_.setTraceNr( location );
	posfld_->setValue( viewer_.traceNr() );
    }
}


bool uiViewer3DPositionDlg::rejectOK( CallBacker* )
{
    oobox_->setChecked( true );
    return true;
}

} // namespace PreStack
