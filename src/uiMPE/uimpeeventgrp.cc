/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpeeventgrp.h"

#include "draw.h"
#include "horizonadjuster.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "separstr.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uilabel.h"
#include "uimpepreviewgrp.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "od_helpids.h"

// TODO: Move preview viewer to separate object

#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

static VSEvent::Type getEventType( int sel )
{
    if ( sel==0 )	return VSEvent::Max;
    if ( sel==1 )	return VSEvent::Min;
    if ( sel==2 )	return VSEvent::ZCPosNeg;
    if ( sel==3 )	return VSEvent::ZCNegPos;
    if ( sel==4 )	return VSEvent::Max;
    if ( sel==5 )	return VSEvent::Min;
			return VSEvent::None;
}


static int getEventIdx( VSEvent::Type tp )
{
    if ( tp == VSEvent::Max )		return 0;
    if ( tp == VSEvent::Min )		return 1;
    if ( tp == VSEvent::ZCPosNeg )	return 2;
    if ( tp == VSEvent::ZCNegPos )	return 3;
					return -1;
}


static const char* sEventNames[] =
{ "Peak", "Trough", "Zero Crossing +-", "Zero Crossing -+",
  "Local Maximum", "Local Minimum", 0 };

static bool sAllowSteps = false;

uiEventGroup::uiEventGroup( uiParent* p, bool is2d )
    : uiDlgGroup(p,tr("Event"))
    , addstepbut_(0)
    , sectiontracker_(0)
    , adjuster_(0)
    , changed_(this)
    , is2d_(is2d)
    , seedpos_(TrcKeyValue::udf())
{
    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    evfld_ = new uiGenInput( leftgrp, tr("Event type"),
			     StringListInpSpec(sEventNames) );
    evfld_->valuechanged.notify( mCB(this,uiEventGroup,selEventType) );
    leftgrp->setHAlignObj( evfld_ );

    uiStringSet strs;
    strs.add( tr("Cut-off amplitude") ); strs.add( tr("Relative difference") );
    thresholdtypefld_ = new uiGenInput( leftgrp, tr("Threshold type"),
					StringListInpSpec(strs) );
    thresholdtypefld_->setValue( 1 );
    thresholdtypefld_->valuechanged.notify(
	    mCB(this,uiEventGroup,selAmpThresholdType) );
    thresholdtypefld_->attach( alignedBelow, evfld_ );

    ampthresholdfld_ = new uiGenInput( leftgrp, tr("Allowed difference (%)"),
				       StringInpSpec() );
    ampthresholdfld_->attach( alignedBelow, thresholdtypefld_ );
    ampthresholdfld_->valuechanged.notify(
	    mCB(this,uiEventGroup,changeCB) );

    if ( !is2d && sAllowSteps )
    {
	addstepbut_ = new uiPushButton( leftgrp, tr("Steps"),
		mCB(this,uiEventGroup,addStepPushedCB), false );
	addstepbut_->attach( rightTo, ampthresholdfld_ );
    }

    uiString srchwindtxt = tr("Search window %1").arg(SI().getUiZUnitString());
    StepInterval<int> swin0( -10000, -1, -1 );
    StepInterval<int> swin1( 1, 10000, 1 );
    IntInpIntervalSpec iis; iis.setSymmetric( true );
    iis.setLimits( swin0, 0 ); iis.setLimits( swin1, 1 );
    srchgatefld_ = new uiGenInput( leftgrp, srchwindtxt, iis );
    srchgatefld_->attach( alignedBelow, ampthresholdfld_ );
    srchgatefld_->valuechanging.notify( mCB(this,uiEventGroup,changeCB) );

    uiSeparator* sep = new uiSeparator( leftgrp, "Sep" );
    sep->attach( stretchedBelow, srchgatefld_ );

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    StepInterval<int> intv( -10000, 10000, step );
    IntInpIntervalSpec diis; diis.setSymmetric( true );
    diis.setLimits( intv, 0 ); diis.setLimits( intv, 1 );

    uiString disptxt = tr("Data Display window %1")
					.arg(SI().getUiZUnitString());
    nrzfld_ = new uiGenInput( leftgrp, disptxt, diis );
    nrzfld_->attach( alignedBelow, srchgatefld_ );
    nrzfld_->attach( ensureBelow, sep );
    nrzfld_->valuechanging.notify(
		mCB(this,uiEventGroup,visibleDataChangeCB) );

    IntInpSpec tiis; tiis.setLimits( StepInterval<int>(3,99,2) );
    nrtrcsfld_ = new uiGenInput( leftgrp, tr("Nr Traces"), tiis );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiEventGroup,visibleDataChangeCB) );

    datalabel_ = new uiLabel( leftgrp, uiStrings::sEmptyString() );
    datalabel_->setStretch( 2, 1 );
    datalabel_->attach( leftAlignedBelow, nrzfld_ );
    datalabel_->attach( ensureBelow, nrtrcsfld_ );

    previewgrp_ = new uiPreviewGroup( this );
    previewgrp_->attach( rightTo, leftgrp );
    previewgrp_->windowChanged_.notify(
		mCB(this,uiEventGroup,previewChgCB) );
}


uiEventGroup::~uiEventGroup()
{
}


void uiEventGroup::updateSensitivity( bool )
{
    NotifyStopper ns( changed_ );
    selEventType( 0 );
}


void uiEventGroup::changeCB( CallBacker* )
{
    previewgrp_->setWindow( srchgatefld_->getIInterval() );
    changed_.trigger();
}


void uiEventGroup::visibleDataChangeCB( CallBacker* )
{
    previewgrp_->setWindow( srchgatefld_->getIInterval() );
    previewgrp_->setDisplaySize( nrtrcsfld_->getIntValue(),
				 nrzfld_->getIInterval() );
}


void uiEventGroup::previewChgCB(CallBacker *)
{
    const Interval<int> intv = previewgrp_->getManipWindow();
    if ( mIsUdf(intv.start) )
	srchgatefld_->setValue( intv.stop, 1 );
    if ( mIsUdf(intv.stop) )
	srchgatefld_->setValue( intv.start, 0 );
}


void uiEventGroup::selEventType( CallBacker* )
{
    const VSEvent::Type ev = getEventType( evfld_->getIntValue() );
    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld_->setSensitive( thresholdneeded );
    ampthresholdfld_->setSensitive( thresholdneeded );
    if ( addstepbut_ )
	addstepbut_->setSensitive( thresholdneeded );

    changed_.trigger();
}


void uiEventGroup::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld_->getIntValue() == 0;
    ampthresholdfld_->setTitleText(absthreshold ?tr("Amplitude value")
						:tr("Allowed difference (%)"));
    if ( absthreshold )
    {
	const TypeSet<float>& thresholds = adjuster_->getAmplitudeThresholds();
	if ( is2d_ || thresholds.isEmpty() )
	    ampthresholdfld_->setValue( adjuster_->amplitudeThreshold() );
	else
	{
	    BufferString bs;
	    bs += thresholds[0];
	    if ( sAllowSteps )
	    {
		for (int idx=1;idx<thresholds.size();idx++)
		{ bs += ","; bs += thresholds[idx]; }
	    }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
    else
    {
	const TypeSet<float>& variances = adjuster_->getAllowedVariances();
	if ( is2d_ || variances.isEmpty() )
	    ampthresholdfld_->setValue( adjuster_->allowedVariance()*100 );
	else
	{
	    BufferString bs;
	    bs += variances[0]*100;
	    if ( sAllowSteps )
	    {
		for ( int idx=1; idx<variances.size(); idx++)
		{ bs += ","; bs += variances[idx]*100; }
	    }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }

    changed_.trigger();
}



class uiStepDialog : public uiDialog
{ mODTextTranslationClass(uiStepDialog)
public:

uiStepDialog( uiParent* p, const char* valstr )
    : uiDialog(p,Setup(tr("Stepwise tracking"),uiString::emptyString(),
			mODHelpKey(mTrackingWizardHelpID)))
{
    steptable_ = new uiTable( this, uiTable::Setup(5,1).rowdesc("Step")
						       .rowgrow(true)
						       .defrowlbl(true)
						       .selmode(uiTable::Multi)
						       .defrowlbl(""),
			      "Stepwise tracking table" );
    steptable_->setColumnLabel( 0, uiStrings::sValue() );

    SeparString ss( valstr, ',' );
    if ( ss.size() > 3 )
	steptable_->setNrRows( ss.size() + 2 );

    for ( int idx=0; idx<ss.size(); idx++ )
	steptable_->setText( RowCol(idx,0), ss[idx] );
}


void getValueString( BufferString& valstr )
{
    SeparString ss( 0, ',' );
    for ( int idx=0; idx<steptable_->nrRows(); idx++ )
    {
	const char* valtxt = steptable_->text( RowCol(idx,0) );
	if ( !valtxt || !*valtxt ) continue;
	ss.add( valtxt );
    }

    valstr = ss.buf();
}

    uiTable*	steptable_;
};



void uiEventGroup::addStepPushedCB( CallBacker* )
{
    uiStepDialog dlg( this, ampthresholdfld_->text() );
    if ( !dlg.go() ) return;

    BufferString valstr;
    dlg.getValueString( valstr );
    ampthresholdfld_->setText( valstr );
}


void uiEventGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,st ? st->adjuster() : 0)
    adjuster_ = horadj;
    if ( !adjuster_ ) return;

    init();
}


void uiEventGroup::init()
{
    if ( !adjuster_ ) return;

    VSEvent::Type ev = adjuster_->trackEvent();
    int fldidx = getEventIdx( ev );
    if ( adjuster_->isAmplitudeSignChangeAllowed() )
	fldidx += 4;
    evfld_->setValue( fldidx );

    NotifyStopper ns1( srchgatefld_->valuechanging );
    Interval<float> intvf( adjuster_->searchWindow() );
    intvf.scale( mCast(float,SI().zDomain().userFactor()) );
    Interval<int> srchintv; srchintv.setFrom( intvf );
    srchgatefld_->setValue( srchintv );

    thresholdtypefld_->setValue( adjuster_->useAbsThreshold() ? 0 : 1 );

    const int sample = 2*mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const Interval<int> dataintv = srchintv + Interval<int>(-sample,sample);
    nrzfld_->setValue( dataintv );

    nrtrcsfld_->setValue( 5 );

    selAmpThresholdType( 0 );
}


void uiEventGroup::setSeedPos( const TrcKeyValue& tkv )
{
    seedpos_ = tkv;
    previewgrp_->setSeedPos( tkv );

    if ( adjuster_ && adjuster_->getAttributeSel(0) )
    {
	const Attrib::SelSpec* as = adjuster_->getAttributeSel(0);
	datalabel_->setText( tr("Picked on: %1").arg(as->userRef()) );
    }
}


bool uiEventGroup::commitToTracker( bool& fieldchange ) const
{
    if ( !adjuster_ ) return false;

    fieldchange = false;

    VSEvent::Type evtyp = getEventType( evfld_->getIntValue() );
    if ( adjuster_->trackEvent() != evtyp )
    {
	fieldchange = true;
	adjuster_->setTrackEvent( evtyp );
    }

    const bool allowsignchg = evfld_->getIntValue() > 3;
    if ( adjuster_->isAmplitudeSignChangeAllowed() != allowsignchg )
    {
	fieldchange = true;
	adjuster_->allowAmplitudeSignChange( allowsignchg );
    }

    Interval<float> intv = srchgatefld_->getFInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( tr("Search window should be minus to positive, ex. -20, 20"));
    Interval<float> relintv( (float)intv.start/SI().zDomain().userFactor(),
			     (float)intv.stop/SI().zDomain().userFactor() );
    if ( adjuster_->searchWindow() != relintv )
    {
	fieldchange = true;
	adjuster_->setSearchWindow( relintv );
    }

    const bool useabs = thresholdtypefld_->getIntValue() == 0;
    if ( adjuster_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	adjuster_->setUseAbsThreshold( useabs );
    }

    SeparString ss( ampthresholdfld_->text(), ',' );
    if ( useabs )
    {
	const float thrval = ss.getFValue(0);
	if ( mIsUdf(thrval) )
	    mErrRet( tr("Value threshold not set") );

	if ( adjuster_->amplitudeThreshold() != thrval )
	{
	    fieldchange = true;
	    adjuster_->setAmplitudeThreshold( thrval );
	}
    }
    else
    {
	const float var = ss.getFValue(0) / 100;
	if ( var<=0.0 || var>=1.0 )
	    mErrRet( tr("Allowed difference must be between 0-100") );

	if ( adjuster_->allowedVariance() != var )
	{
	    fieldchange = true;
	    adjuster_->setAllowedVariance( var );
	}
    }

    if ( sAllowSteps )
    { pErrMsg("Multiple steps not handled"); }

    return true;
}

} // namespace MPE
