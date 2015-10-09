/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

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
    if ( sel==0 )	return VSEvent::Min;
    if ( sel==1 )	return VSEvent::Max;
    if ( sel==2 )	return VSEvent::ZCPosNeg;
    if ( sel==3 )	return VSEvent::ZCNegPos;
			return VSEvent::None;
}


static int getEventIdx( VSEvent::Type tp )
{
    if ( tp == VSEvent::Min )		return 0;
    if ( tp == VSEvent::Max )		return 1;
    if ( tp == VSEvent::ZCPosNeg )	return 2;
    if ( tp == VSEvent::ZCNegPos )	return 3;
					return -1;
}


static const char* sEventNames[] = { "Min", "Max", "0+-", "0-+", 0 };


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
    evfld_->valuechanged.notify( mCB(this,uiEventGroup,changeCB) );
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

    if ( !is2d )
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

    uiString disptxt = tr("Data Display Window %1")
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

    previewgrp_ = new uiPreviewGroup( this );
    previewgrp_->attach( rightTo, leftgrp );
    previewgrp_->windowChanged_.notify(
		mCB(this,uiEventGroup,previewChgCB) );
}


uiEventGroup::~uiEventGroup()
{
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
}


void uiEventGroup::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld_->getIntValue() == 0;
    ampthresholdfld_->setTitleText(absthreshold ?tr("Amplitude value")
						:tr("Allowed difference (%)"));
    if ( absthreshold )
    {
	if ( is2d_ || adjuster_->getAmplitudeThresholds().isEmpty() )
	    ampthresholdfld_->setValue( adjuster_->amplitudeThreshold() );
	else
	{
	    BufferString bs;
	    bs += adjuster_->getAmplitudeThresholds()[0];
	    for (int idx=1;idx<adjuster_->getAmplitudeThresholds().size();idx++)
	    { bs += ","; bs += adjuster_->getAmplitudeThresholds()[idx]; }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
    else
    {
	if ( is2d_ || adjuster_->getAllowedVariances().isEmpty() )
	    ampthresholdfld_->setValue( adjuster_->allowedVariance()*100 );
	else
	{
	    BufferString bs;
	    bs += adjuster_->getAllowedVariances()[0]*100;
	    for ( int idx=1; idx<adjuster_->getAllowedVariances().size(); idx++)
	    { bs += ","; bs += adjuster_->getAllowedVariances()[idx]*100; }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
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
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    adjuster_ = horadj;
    if ( !adjuster_ ) return;

    init();
}


void uiEventGroup::init()
{
    VSEvent::Type ev = adjuster_->trackEvent();
    const int fldidx = getEventIdx( ev );
    evfld_->setValue( fldidx );

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
}


bool uiEventGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;

    VSEvent::Type evtyp = getEventType( evfld_->getIntValue() );
    if ( adjuster_->trackEvent() != evtyp )
    {
	fieldchange = true;
	adjuster_->setTrackEvent( evtyp );
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

    const bool useabs = thresholdtypefld_->getBoolValue() == 0;
    if ( adjuster_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	adjuster_->setUseAbsThreshold( useabs );
    }

    if ( useabs )
    {
	SeparString ss( ampthresholdfld_->text(), ',' );
	int idx = 0;
	if ( ss.size() < 2 )
	{
	    float vgate = ss.getFValue(0);
	    if ( Values::isUdf(vgate) )
		mErrRet( tr("Value threshold not set") );
	    if ( adjuster_->amplitudeThreshold() != vgate )
	    {
		fieldchange = true;
		adjuster_->setAmplitudeThreshold( vgate );
	    }
	}
	else
	{
	    TypeSet<float> vars;
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx);
		if ( Values::isUdf(varvalue) )
		    mErrRet( tr("Value threshold not set properly") );

		if ( adjuster_->getAmplitudeThresholds().size() < idx+1 )
		{
		    fieldchange = true;
		    adjuster_->getAmplitudeThresholds() += varvalue;
		    if ( idx == 0 )
			adjuster_->setAmplitudeThreshold( varvalue );
		}
		else if ( adjuster_->getAmplitudeThresholds().size() >= idx+1 )
		    if ( adjuster_->getAmplitudeThresholds()[idx] != varvalue )
		    {
			fieldchange = true;
			adjuster_->getAmplitudeThresholds() += varvalue;
			if ( idx == 0 )
			    adjuster_->setAmplitudeThreshold( varvalue );
		    }
	    }
	}

	if ( idx==0 && adjuster_->getAmplitudeThresholds().size() > 0 )
	{
	    adjuster_->getAmplitudeThresholds()[idx] =
				adjuster_->amplitudeThreshold();
	    idx++;
	}

	if ( adjuster_->getAmplitudeThresholds().size() > idx )
	{
	    int size = adjuster_->getAmplitudeThresholds().size();
	    fieldchange = true;
	    adjuster_->getAmplitudeThresholds().removeRange( idx, size-1 );
	}
    }
    else
    {
	SeparString ss( ampthresholdfld_->text(), ',' );
	int idx = 0;
	TypeSet<float>& vars = adjuster_->getAllowedVariances();
	if ( ss.size() < 2 )
	{
	    float var = ss.getFValue(0) / 100;
	    if ( var<=0.0 || var>=1.0 )
		mErrRet( tr("Allowed difference must be between 0-100") );
	    if ( adjuster_->allowedVariance() != var )
	    {
		fieldchange = true;
		adjuster_->setAllowedVariance( var );
	    }
	}
	else
	{
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx) / 100;
		if ( varvalue <=0.0 || varvalue>=1.0 )
		    mErrRet( tr("Allowed difference must be between 0-100") );

		if ( vars.size() < idx+1 )
		{
		    fieldchange = true;
		    vars += varvalue;
		    if ( idx == 0 )
			adjuster_->setAllowedVariance( varvalue );
		}
		else if ( vars.size() >= idx+1 )
		    if ( vars[idx] != varvalue )
		    {
			fieldchange = true;
			vars[idx] = varvalue;
			if ( idx == 0 )
			    adjuster_->setAllowedVariance( varvalue );
		    }
	    }
	}

	if ( idx==0 && vars.size()>0 )
	{
	    vars[idx] = adjuster_->allowedVariance();
	    idx++;
	}

	if (  vars.size() > idx )
	{
	    const int size = vars.size();
	    fieldchange = true;
	    vars.removeRange( idx, size-1 );
	}
    }

    return true;
}

} //namespace MPE
