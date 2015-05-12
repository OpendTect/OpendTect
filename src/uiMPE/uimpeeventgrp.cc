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
#include "mpeengine.h"
#include "sectiontracker.h"
#include "separstr.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimsg.h"
#include "uitable.h"
#include "od_helpids.h"


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
    , seedpos_(Coord3::udf())
{
    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    evfld_ = new uiGenInput( leftgrp, tr("Event type"),
			     StringListInpSpec(sEventNames) );
    evfld_->valuechanged.notify( mCB(this,uiEventGroup,selEventType) );
    evfld_->valuechanged.notify( mCB(this,uiEventGroup,changeCB) );

    thresholdtypefld_ = new uiGenInput( leftgrp, tr("Threshold type"),
		BoolInpSpec(true,tr("Cut-off amplitude"),
				 tr("Relative difference")) );
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

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    StepInterval<int> intv( -10000, 10000, step );
    IntInpSpec iis; iis.setLimits( intv );

    BufferString disptxt( "Data Display window ", SI().getZUnitString() );
    nrzfld_ = new uiGenInput( leftgrp, disptxt, iis, iis );
    nrzfld_->attach( alignedBelow, ampthresholdfld_ );
    nrzfld_->valuechanging.notify(
		mCB(this,uiEventGroup,visibleDataChangeCB) );

    IntInpSpec tiis; tiis.setLimits( StepInterval<int>(3,99,2) );
    nrtrcsfld_ = new uiGenInput( leftgrp, "Nr Traces", tiis );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiEventGroup,visibleDataChangeCB) );

    BufferString srchwindtxt( "Search window ", SI().getZUnitString() );
    intv.step = 1;
    iis.setLimits( intv );
    srchgatefld_ = new uiGenInput( leftgrp, srchwindtxt, iis, iis );
    srchgatefld_->attach( alignedBelow, nrtrcsfld_ );
    srchgatefld_->valuechanged.notify( mCB(this,uiEventGroup,changeCB) );

    previewvwr_ = new uiFlatViewer( this );
    previewvwr_->attach( rightOf, leftgrp );
    previewvwr_->setPrefWidth( 150 );
    previewvwr_->setPrefHeight( 200 );
    previewvwr_->setStretch( 0, 0 );
    previewvwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);
    previewvwr_->appearance().setGeoDefaults( true );

    minitm_ = previewvwr_->createAuxData( "Min line" );
    minitm_->cursor_.shape_ = MouseCursor::SizeVer;
    minitm_->linestyle_.color_ = Color(0,255,0);
    minitm_->poly_ += FlatView::Point(0,0);
    minitm_->poly_ += FlatView::Point(0,0);
    previewvwr_->addAuxData( minitm_ );

    maxitm_ = previewvwr_->createAuxData( "Max line" );
    maxitm_->cursor_.shape_ = MouseCursor::SizeVer;
    maxitm_->linestyle_.color_ = Color(0,255,0);
    maxitm_->poly_ += FlatView::Point(0,0);
    maxitm_->poly_ += FlatView::Point(0,0);
    previewvwr_->addAuxData( maxitm_ );

    seeditm_ = previewvwr_->createAuxData( "Seed" );
    seeditm_->poly_ += FlatView::Point(0,0);
    seeditm_->markerstyles_ += MarkerStyle2D();
    seeditm_->markerstyles_[0].color_ = Color(0,255,0);
    previewvwr_->addAuxData( seeditm_ );

    setHAlignObj( evfld_ );
}


uiEventGroup::~uiEventGroup()
{
}


void uiEventGroup::changeCB( CallBacker* )
{
    changed_.trigger();
}


void uiEventGroup::visibleDataChangeCB( CallBacker* )
{
    updateViewer();
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
    const bool absthreshold = thresholdtypefld_->getBoolValue();
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
	    for ( int idx=1; idx<adjuster_->getAllowedVariances().size(); idx++ )
	    { bs += ","; bs += adjuster_->getAllowedVariances()[idx]*100; }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
}



class uiStepDialog : public uiDialog
{
public:

uiStepDialog( uiParent* p, const char* valstr )
    : uiDialog(p,Setup("Stepwise tracking",uiStrings::sEmptyString(),
			mODHelpKey(mTrackingWizardHelpID)))
{
    steptable_ = new uiTable( this, uiTable::Setup(5,1).rowdesc("Step")
						       .rowgrow(true)
						       .defrowlbl(true)
						       .selmode(uiTable::Multi)
						       .defrowlbl(""),
			      "Stepwise tracking table" );
    steptable_->setColumnLabel( 0, "Value" );

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

    Interval<float> intvf( adjuster_->permittedZRange() );
    intvf.scale( mCast(float,SI().zDomain().userFactor()) );
    Interval<int> srchintv; srchintv.setFrom( intvf );
    srchgatefld_->setValue( srchintv );

    thresholdtypefld_->setValue( adjuster_->useAbsThreshold() );

    const int sample = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const Interval<int> dataintv = srchintv + Interval<int>(-sample,sample);
    nrzfld_->setValue( dataintv );

    nrtrcsfld_->setValue( 5 );
}


void uiEventGroup::setSeedPos( const Coord3& crd )
{
    seedpos_ = crd;
    updateViewer();
}


void uiEventGroup::updateViewer()
{
    if ( seedpos_.isUdf() )
	return;

    const BinID& bid = SI().transform( seedpos_.coord() );
    const float z = (float)seedpos_.z;
    const int nrtrcs = nrtrcsfld_->getIntValue();

    StepInterval<float> zintv; zintv.setFrom( nrzfld_->getIInterval() );
    zintv.scale( 1.f/SI().zDomain().userFactor() );
    zintv.step = SI().zStep();
    const DataPack::ID dpid =
	MPE::engine().getSeedPosDataPack( bid, z, nrtrcs, zintv );

    previewvwr_->setPack( true, dpid );
    previewvwr_->setViewToBoundingBox();

    FlatView::Point& pt = seeditm_->poly_[0];
    pt = FlatView::Point( bid.crl(), z );

    minitm_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs/2, z+zintv.start );
    minitm_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs/2, z+zintv.start );
    maxitm_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs/2, z+zintv.stop );
    maxitm_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs/2, z+zintv.stop );
    previewvwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
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
    if ( adjuster_->permittedZRange() != relintv )
    {
	fieldchange = true;
	adjuster_->setPermittedZRange( relintv );
    }

    const bool useabs = thresholdtypefld_->getBoolValue();
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
		mErrRet( tr("Allowed variance must be between 0-100") );
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
		    mErrRet( tr("Allowed variance must be between 0-100") );

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
