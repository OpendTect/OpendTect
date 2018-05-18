/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
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
#include "uicombobox.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimpepreviewgrp.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistrings.h"
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
	{ "Peak", "Trough", "Zero Crossing +-", "Zero Crossing -+", 0 };

static bool sAllowSteps = false;

class uiAmplThresholdGrp : public uiGroup
{ mODTextTranslationClass(uiAmplThresholdGrp)
public:
uiAmplThresholdGrp( uiParent* p, const uiString& lbl )
    : uiGroup(p,"Ampl Threshold")
{
    cbfld_ = new uiCheckBox( this, lbl, mCB(this,uiAmplThresholdGrp,checkCB) );

    uiStringSet list;
    list.add( uiStrings::sRelative() ).add( uiStrings::sAbsolute() );
    typefld_ = new uiComboBox( this, list, "Type" );
    typefld_->attach( rightTo, cbfld_ );
    typefld_->selectionChanged.notify( mCB(this,uiAmplThresholdGrp,typeCB) );

    percfld_ = new uiSpinBox( this, 0, "Percentage" );
    percfld_->setSuffix( toUiString("%") );
    percfld_->attach( rightTo, typefld_ );

    amplfld_ = new uiLineEdit( this, "Amplitude" );
    amplfld_->attach( rightTo, typefld_ );

    setHAlignObj( typefld_ );
}


void checkCB( CallBacker* )
{
    const bool ison = hasThreshold();
    typefld_->setSensitive( ison );
    percfld_->setSensitive( ison );
    amplfld_->setSensitive( ison );
}

void typeCB( CallBacker* )
{
    const bool isrel = hasRelThreshold();
    percfld_->display( isrel );
    amplfld_->display( !isrel );
}

bool hasThreshold() const
{ return cbfld_->isChecked(); }

bool hasRelThreshold() const
{ return typefld_->currentItem()==0; }

int getRelValue() const
{ return percfld_->getIntValue(); }
void setRelValue( int val )
{ percfld_->setValue( val ); }

float getAbsValue() const
{ return amplfld_->getFValue(); }
void setAbsValue( float val )
{ amplfld_->setValue( val ); }

    uiCheckBox*		cbfld_;
    uiComboBox*		typefld_;
    uiSpinBox*		percfld_;
    uiLineEdit*		amplfld_;
};


uiEventGroup::uiEventGroup( uiParent* p, bool is2d )
    : uiDlgGroup(p,uiStrings::sEvent())
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
    allowsignchgfld_ = new uiCheckBox( leftgrp, tr("Positive only"),
		mCB(this,uiEventGroup,changeCB) );
    allowsignchgfld_->attach( rightTo, evfld_ );

    ulimitfld_ = new uiAmplThresholdGrp( leftgrp, tr("Lower limit") );
    ulimitfld_->attach( alignedBelow, evfld_ );
    llimitfld_ = new uiAmplThresholdGrp( leftgrp, tr("Upper limit") );
    llimitfld_->attach( alignedBelow, ulimitfld_ );

    uiString srchwindtxt = tr("Search window").withSurvZUnit();
    StepInterval<int> swin0( -10000, -1, -1 );
    StepInterval<int> swin1( 1, 10000, 1 );
    IntInpIntervalSpec iis; iis.setSymmetric( true );
    iis.setLimits( swin0, 0 ); iis.setLimits( swin1, 1 );
    srchgatefld_ = new uiGenInput( leftgrp, srchwindtxt, iis );
    srchgatefld_->attach( alignedBelow, llimitfld_ );
    srchgatefld_->valuechanging.notify( mCB(this,uiEventGroup,changeCB) );

    uiSeparator* sep = new uiSeparator( leftgrp, "Sep" );
    sep->attach( stretchedBelow, srchgatefld_ );

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    StepInterval<int> intv( -10000, 10000, step );
    IntInpIntervalSpec diis; diis.setSymmetric( true );
    diis.setLimits( intv, 0 ); diis.setLimits( intv, 1 );

    uiString disptxt = tr("Data Display window").withSurvZUnit();
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

    datalabel_ = new uiLabel( leftgrp, uiString::empty() );
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
    const bool minormax = ev==VSEvent::Min || ev==VSEvent::Max;
    llimitfld_->setSensitive( minormax );
    ulimitfld_->setSensitive( minormax );
    allowsignchgfld_->display( minormax );

    if ( ev == VSEvent::Max )
	allowsignchgfld_->setText( tr("Positive only") );
    else if ( ev == VSEvent::Min )
	allowsignchgfld_->setText( tr("Negative only") );
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
    const int fldidx = getEventIdx( ev );
    evfld_->setValue( fldidx );
    allowsignchgfld_->setChecked( !adjuster_->isAmplitudeSignChangeAllowed() );

    Interval<float> intvf( adjuster_->searchWindow() );
    intvf.scale( mCast(float,SI().zDomain().userFactor()) );
    Interval<int> srchintv; srchintv.setFrom( intvf );
    srchgatefld_->setValue( srchintv );

    llimitfld_->setRelValue( mNINT32(100*adjuster_->allowedVariance()) );
    llimitfld_->setAbsValue( adjuster_->amplitudeThreshold() );
    ulimitfld_->setRelValue( mNINT32(100*adjuster_->allowedVariance()) );
    ulimitfld_->setAbsValue( adjuster_->amplitudeThreshold() );

    const int sample = 2*mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const Interval<int> dataintv = srchintv + Interval<int>(-sample,sample);
    nrzfld_->setValue( dataintv );

    nrtrcsfld_->setValue( 5 );
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

    const bool allowsignchg = !allowsignchgfld_->isChecked();
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

    const bool useabs = !llimitfld_->hasRelThreshold();
    if ( adjuster_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	adjuster_->setUseAbsThreshold( useabs );
    }

    if ( useabs )
    {
	const float thrval = llimitfld_->getAbsValue();
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
	const float var = (float)llimitfld_->getRelValue() / 100.f;
	if ( var<0.0 || var>1.0 )
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
