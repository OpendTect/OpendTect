/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/


#include "uiseiswvltman.h"

#include "dbman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "waveletmanager.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uiaxishandler.h"
#include "uitoolbutton.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseiswvltattr.h"
#include "uiseiswvltgen.h"
#include "uiseiswvltimpexp.h"
#include "uisurvioobjseldlg.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uiwaveletextraction.h"
#include "uiwaveletmatchdlg.h"
#include "od_helpids.h"
#include "uilabel.h"


#define mErrRet(s) { uiMSG().error(s); return; }

mDefineInstanceCreatedNotifierAccess(uiSeisWvltMan)


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
		uiStrings::phrManage(uiStrings::sWavelet(mPlural)),mNoDlgTitle,
		mODHelpKey(mSeisWvltManHelpID) ).nrstatusflds(1).modal(false),
		   WaveletTranslatorGroup::ioContext() )
    , extrdlg_(0)
    , propdlg_(0)
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    copybut_ = manipgrp->addButton( "copyobj",
		uiStrings::phrCopy(uiStrings::sWavelet()),
		mCB(this,uiSeisWvltMan,copyPush) );
    manipgrp->nextButtonOnNewRowCol();
    manipgrp->addButton( "impfromothsurv", tr("Get from other survey"),
			mCB(this,uiSeisWvltMan,getFromOtherSurvey) );
    disppropbut_ = manipgrp->addButton( "info", uiStrings::sDisplayProperties(),
				mCB(this,uiSeisWvltMan, dispProperties) );

    revpolbut_ = manipgrp->addButton( "revpol", tr("Reverse polarity"),
				mCB(this,uiSeisWvltMan,reversePolarity) );
    rotatephbut_  = manipgrp->addButton( "phase", tr("Rotate phase"),
				mCB(this,uiSeisWvltMan,rotatePhase) );
    taperbut_ = manipgrp->addButton( "wavelet_taper", uiStrings::sTaper(),
				     mCB(this,uiSeisWvltMan,taper) );
    addButtons();
    uiGroup* wvltdispgrp = new uiGroup( listgrp_,"Wavelet Display" );
    wvltdispgrp->attach( rightOf, selgrp_ );

    uiFunctionDisplay::Setup fdsu;
    fdsu.noy2axis(true).noy2gridline(true);

    waveletdisplay_ = new uiFunctionDisplay( wvltdispgrp, fdsu );
    const uiString ztxt = (SI().zIsTime() ? uiStrings::sTime()
					  : uiStrings::sDepth())
			    .withSurvZUnit();
    waveletdisplay_->xAxis()->setCaption( ztxt );
    waveletdisplay_->yAxis(false)->setCaption( uiStrings::sAmplitude() );

    wvnamdisp_ = new uiLabel( wvltdispgrp, uiStrings::sWavelet() );
    wvnamdisp_->attach(centeredAbove, waveletdisplay_);
    wvnamdisp_->setAlignment( OD::Alignment::HCenter );

    selChg( this );
    mTriggerInstanceCreatedNotifier();
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    detachAllNotifiers();
    delete extrdlg_;
    delete propdlg_;
}


void uiSeisWvltMan::addButtons()
{
    uiButtonGroup* grp = extraButtonGroup();
    uiButton* impbut = uiButton::getStd( grp, OD::Import,
				     mCB(this,uiSeisWvltMan,impPush), false );

    uiPushButton* crbut = new uiPushButton( grp, uiStrings::sGenerate(), false);
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );

    uiPushButton* mergebut = new uiPushButton( grp, uiStrings::sStack(), false);
    mergebut->activated.notify( mCB(this,uiSeisWvltMan,mrgPush) );
    mergebut->attach( rightOf, crbut );

    uiPushButton* extractbut = new uiPushButton( grp, uiStrings::sExtract(),
						 false );
    extractbut->activated.notify( mCB(this,uiSeisWvltMan,extractPush) );
    extractbut->attach( rightOf, mergebut );

    uiPushButton* matchbut = new uiPushButton( grp, uiStrings::sMatch(), false);
    matchbut->activated.notify( mCB(this,uiSeisWvltMan,matchPush) );
    matchbut->attach( rightOf, extractbut );
}


void uiSeisWvltMan::impPush( CallBacker* )
{
    uiSeisWvltImp dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.selKey() );
}


void uiSeisWvltMan::crPush( CallBacker* )
{
    uiSeisWvltGen dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.storeKey() );
}


void uiSeisWvltMan::mrgPush( CallBacker* )
{
    if ( selgrp_->getListField()->size()<2 )
	mErrRet( tr("At least two wavelets are needed to merge wavelets") );

    uiSeisWvltMerge dlg( this, curioobj_ ? curioobj_->name().str() : 0 );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.storeKey() );
}


void uiSeisWvltMan::extractPush( CallBacker* )
{
    const int res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
    if ( res == -1 )
	return;

    const bool is2d = res == 1;

    if ( extrdlg_ && extrdlg_->is2D() != is2d )
	{ delete extrdlg_; extrdlg_ = 0; }

    if ( !extrdlg_ )
    {
	extrdlg_ = new uiWaveletExtraction( this, is2d );
	mAttachCB( extrdlg_->extractionDone, uiSeisWvltMan::wvltExtractedCB );
	mAttachCB( extrdlg_->windowClosed, uiSeisWvltMan::extrDlgCloseCB );
    }
    extrdlg_->show();
}


void uiSeisWvltMan::matchPush( CallBacker* )
{
    uiWaveletMatchDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getDBKey() );
}


void uiSeisWvltMan::copyPush( CallBacker* )
{
    uiSeisWvltCopy copydlg( this, curioobj_ );
    if ( copydlg.go() )
	selgrp_->fullUpdate( copydlg.getDBKey() );
}


void uiSeisWvltMan::extrDlgCloseCB( CallBacker* )
{
    extrdlg_ = 0;
}


void uiSeisWvltMan::propDlgCloseCB( CallBacker* )
{
    propdlg_ = 0;
}


void uiSeisWvltMan::wvltExtractedCB( CallBacker* )
{
    selgrp_->fullUpdate( extrdlg_->storeKey() );
}


#define mSetButToolTip(but,str1,deftt) \
    if ( !but->isSensitive() ) \
	but->setToolTip( deftt ); \
    else \
	but->setToolTip( str1 ); \

void uiSeisWvltMan::ownSelChg()
{
    BufferString wvltname;
    ConstRefMan<Wavelet> wvlt;
    if ( curioobj_ )
    {
	wvltname = curioobj_->name();
	wvlt = WaveletMGR().fetch( curioobj_->key() );
    }
    dispWavelet( wvlt );

    revpolbut_->setSensitive( wvlt );
    rotatephbut_->setSensitive( wvlt );
    taperbut_->setSensitive( wvlt );
    disppropbut_->setSensitive( wvlt );

    mSetButToolTip(revpolbut_,tr("Reverse %1 polarity").arg(wvltname),
						       tr("Reverse polarity"));
    mSetButToolTip(rotatephbut_,tr("Rotate %1 phase").arg(wvltname),
							   tr("Rotate phase"));
    mSetButToolTip(taperbut_, toUiString("%1 %2")
	    .arg(uiStrings::sTaper()).arg(wvltname), uiStrings::sTaper() );
    mSetButToolTip(disppropbut_,toUiString("%1 %2 %3")
		   .arg(uiStrings::sDisplay()).arg(wvltname)
		   .arg(uiStrings::sProperties().toLower()),
		   uiStrings::sDisplayProperties().toLower());
}


bool uiSeisWvltMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( ioobj.key() );
    if ( !wvlt )
    { inf.add( uiStrings::sNoInfoAvailable() ); return false; }

    uiString msg;
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    WaveletAttrib wvltattrib( *wvlt );

    addObjInfo( inf, uiStrings::sNrSamples(), wvlt->size() );

    addObjInfo( inf, uiStrings::sSampleIntrvl().withSurvZUnit(),
		     wvlt->sampleRate() * zfac );

    Interval<float> extremevals;
    wvlt->getExtrValues( extremevals );
    uiString extremevalrng = toUiString("%1 - %2").arg(extremevals.start)
							.arg(extremevals.stop);

    addObjInfo( inf, tr("Min/Max amplitude"), extremevalrng );

    float avgphase = wvltattrib.getAvgPhase( true );
    if ( mIsZero(avgphase,1e-3f) ) avgphase = 0.f;

    addObjInfo( inf, tr("Average phase (deg)"), avgphase );

    wvlt = 0;
    DBKey orgid; DBKey horid; DBKey seisid; BufferString lvlnm;
    if ( WaveletMGR().getScalingInfo(ioobj.key(),
			orgid,horid,seisid,lvlnm) )
    {
	if ( orgid.isInvalid() )
	    msg = tr("Outside OpendTect");
	else
	{
	    msg = tr("'%1' scaled to '%2' (along '%3' [%4])")
	       .arg(orgid.name()).arg(seisid.name())
	       .arg(horid.name()).arg(lvlnm);
	}

	addObjInfo( inf, tr("Scaled"), msg );
    }

    return true;
}


void uiSeisWvltMan::dispProperties( CallBacker* )
{
    ConstRefMan<Wavelet> wvlt;
    if ( curioobj_ )
	wvlt = WaveletMGR().fetch( curioobj_->key() );
    if ( !wvlt )
	return;

    if ( propdlg_ )
	delete propdlg_;

    propdlg_ = new uiWaveletDispPropDlg( this, *wvlt );
    propdlg_->setCaption( tr("Wavelet '%1' Properties").arg(wvlt->name()) );
    mAttachCB( propdlg_->windowClosed, uiSeisWvltMan::propDlgCloseCB );
    propdlg_->go();
}


#define mRet(s) \
	{ ctio.setObj(0); if ( !s.isEmpty() ) uiMSG().error(s); return; }

void uiSeisWvltMan::getFromOtherSurvey( CallBacker* )
{
    const IOObjContext ctxt( mIOObjContext(Wavelet) );
    uiSurvIOObjSelDlg objsel( this, ctxt );
    objsel.excludeCurrentSurvey();
    objsel.setHelpKey( mODHelpKey(mSeisWvltMangetFromOtherSurveyHelpID) );
    if ( !objsel.go() )
	return;

    WaveletLoader loader( objsel.ioObj() );
    Wavelet* rdwvlt = 0;
    uiRetVal rv = loader.read( rdwvlt );
    if ( rv.isError() )
	uiMSG().error( rv );
    RefMan<Wavelet> wvlt = rdwvlt;

    CtxtIOObj ctio( ctxt );
    ctio.setName( wvlt->name() );
    DBM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	mRet(uiStrings::phrCannotCreate(tr("new entry in Object Management")))
    else if ( !loader.addToMGR(wvlt,ctio.ioobj_->key()) )
	mRet(tr("Cannot add Wavelet to Manager"))
    SilentTaskRunnerProvider trprov;
    uiRetVal uirv = WaveletMGR().save( *wvlt, trprov );
    if ( uirv.isError() )
	mRet(uirv)

    selgrp_->fullUpdate( ctio.ioobj_->key() );
    mRet( uiString::empty() )
}


#define mPrepWvltChg() \
    if ( !curioobj_ ) \
	return; \
    const DBKey ky( curioobj_->key() ); \
    RefMan<Wavelet> wvlt = WaveletMGR().fetchForEdit( ky ); \
    if ( !wvlt ) \
	return

#define mStoreWvltChg() \
    SilentTaskRunnerProvider trprov; \
    uiRetVal rv = WaveletMGR().store( *wvlt, ky, trprov ); \
    if ( rv.isError() ) \
	uiMSG().error( rv ); \
    else \
	selgrp_->fullUpdate( ky )

void uiSeisWvltMan::reversePolarity( CallBacker* )
{
    mPrepWvltChg();

    TypeSet<float> samps; wvlt->getSamples( samps );
    for ( int idx=0; idx<wvlt->size(); idx++ )
	samps[idx] *= -1;
    wvlt->setSamples( samps );

    mStoreWvltChg();
}


void uiSeisWvltMan::rotatePhase( CallBacker* )
{
    mPrepWvltChg();

    uiSeisWvltRotDlg dlg( this, *wvlt );
    dlg.acting.notify( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    if ( dlg.go() )
	{ mStoreWvltChg(); }

    dlg.acting.remove( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    updateFromSelected();
}


void uiSeisWvltMan::taper( CallBacker* )
{
    mPrepWvltChg();

    uiSeisWvltTaperDlg dlg( this, *wvlt );
    uiString title = tr("Taper '%1'").arg(curioobj_->name());
    dlg.setCaption( title );
    if ( dlg.go() )
	{ mStoreWvltChg(); }
}

#define mErr() mErrRet( uiStrings::phrCannotCreate(uiStrings::sWavelet()) );

void uiSeisWvltMan::rotUpdateCB( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltRotDlg*,dlg,cb);
    if ( !dlg )
	return;

    const Wavelet* wvlt = dlg->getWavelet();
    if ( !wvlt )
	return;

    dispWavelet( wvlt );
}


void uiSeisWvltMan::dispWavelet( const Wavelet* wvlt )
{
    if ( !curioobj_ )
	return;

    wvnamdisp_->setText( toUiString(curioobj_->name()) );
    wvnamdisp_->setPrefWidthInChar( 60 );
    TypeSet<float> samps;
    if ( wvlt )
	wvlt->getSamples( samps );
    if ( samps.isEmpty() )
	{ waveletdisplay_->setEmpty(); return; }

    const int wvltsz = wvlt->size();
    StepInterval<float> intxval;
    intxval.setFrom( wvlt->samplePositions() );
    const float zfac = mCast(float,SI().zDomain().userFactor());
    intxval.scale( zfac );
    waveletdisplay_->setVals( intxval, samps.arr(), wvltsz );
}
