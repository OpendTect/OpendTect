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
#include "uiselobjothersurv.h"
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
                                     mODHelpKey(mSeisWvltManHelpID) )
				     .nrstatusflds(1).modal(false),
		   WaveletTranslatorGroup::ioContext() )
    , extrdlg_(0)
    , propdlg_(0)
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "impfromothsurv", tr("Get from other survey"),
			mCB(this,uiSeisWvltMan,getFromOtherSurvey) );
    disppropbut_ = manipgrp->addButton( "info", mJoinUiStrs(sDisplay(),
				sProperties()), mCB(this,uiSeisWvltMan,
				dispProperties) );

    revpolbut_ = manipgrp->addButton( "revpol", tr("Reverse polarity"),
				mCB(this,uiSeisWvltMan,reversePolarity) );
    rotatephbut_  = manipgrp->addButton( "phase", tr("Rotate phase"),
				mCB(this,uiSeisWvltMan,rotatePhase) );
    taperbut_ = manipgrp->addButton( "wavelet_taper", tr("Taper"),
				     mCB(this,uiSeisWvltMan,taper) );
    addButtons();
    uiGroup* wvltdispgrp = new uiGroup( listgrp_,"Wavelet Display" );
    wvltdispgrp->attach( rightOf, selgrp_ );

    uiFunctionDisplay::Setup fdsu;
    fdsu.noy2axis(true).noy2gridline(true);

    waveletdisplay_ = new uiFunctionDisplay( wvltdispgrp, fdsu );
    const uiString ztxt = toUiString("%1 %2").arg(SI().zIsTime() ?
	 uiStrings::sTime() : uiStrings::sDepth()).arg(SI().zUnitString());
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
    uiPushButton* impbut = new uiPushButton( grp, uiStrings::sImport(), false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );

    uiPushButton* crbut = new uiPushButton( grp, tr("Generate"), false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );

    uiPushButton* mergebut = new uiPushButton( grp, tr("Stack"), false );
    mergebut->activated.notify( mCB(this,uiSeisWvltMan,mrgPush) );
    mergebut->attach( rightOf, crbut );

    uiPushButton* extractbut = new uiPushButton( grp, tr("Extract"), false );
    extractbut->activated.notify( mCB(this,uiSeisWvltMan,extractPush) );
    extractbut->attach( rightOf, mergebut );

    uiPushButton* matchbut = new uiPushButton( grp, tr("Match"), false );
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
    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
	if ( res == -1 )
	    return;
	else
	    is2d = res == 1;
    }

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
    if ( !but->sensitive() ) \
	but->setToolTip( deftt ); \
    else \
	but->setToolTip( str1 ); \

void uiSeisWvltMan::ownSelChg()
{
    uiString tt,curwvlt;
    if ( curioobj_ )
	curwvlt = curioobj_->uiName();

    revpolbut_->setSensitive( curioobj_ );
    rotatephbut_->setSensitive( curioobj_ );
    taperbut_->setSensitive( curioobj_ );
    disppropbut_->setSensitive( curioobj_ );

    mSetButToolTip(revpolbut_,tr("Reverse %1 polarity").arg(curwvlt),
						       tr("Reverse polarity"));
    mSetButToolTip(rotatephbut_,tr("Rotate %1 phase").arg(curwvlt),
							   tr("Rotate phase"));
    mSetButToolTip(taperbut_,tr("Taper %1").arg(curwvlt), tr("Taper") );
    mSetButToolTip(disppropbut_,toUiString("%1 %2 %3")
		   .arg(uiStrings::sDisplay()).arg(curwvlt)
		   .arg(uiStrings::sProperties().toLower()),
		   mJoinUiStrs(sDisplay(),sProperties().toLower()));
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;
    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( curioobj_->key() );
    dispWavelet( wvlt );
    if ( wvlt )
    {
	const float zfac = mCast( float, SI().zDomain().userFactor() );
	WaveletAttrib wvltattrib( *wvlt );

	BufferString msg;
	msg.add( "Number of samples: " ).add( wvlt->size() ).addNewLine();
	msg.add( "Sample interval " )
	   .add( SI().zUnitString(true).getFullString() )
	   .add( ": " ).add( wvlt->sampleRate() * zfac ).addNewLine();
	Interval<float> extremevals;
	wvlt->getExtrValues( extremevals );
	msg.add( "Min/Max amplitude: " ).add( extremevals.start )
	   .add( "/" ).add( extremevals.stop ).addNewLine();
	float avgphase = wvltattrib.getAvgPhase( true );
	if ( mIsZero(avgphase,1e-3f) ) avgphase = 0.f;
	msg.add( "Average phase (deg): ").add( avgphase, 2 ).addNewLine();
	txt.add( msg );
	wvlt = 0;

	DBKey orgid; DBKey horid; DBKey seisid; BufferString lvlnm;
	if ( WaveletMGR().getScalingInfo(curioobj_->key(),
			    orgid,horid,seisid,lvlnm) )
	{
	    msg = "Scaled: ";
	    if ( orgid.isInvalid() )
		msg.add( "Outside OpendTect" );
	    else
	    {
		msg.add( "'").add( DBM().nameOf(orgid) ).add( "'" );
		msg.add( " scaled to '").add( DBM().nameOf(seisid) ).add( "'" );
		msg.add( "\n\t(along '").add( DBM().nameOf(horid) ).add( "'" );
		msg.add( " at '").add( lvlnm ).add( "')" );
	    }
	    txt.add( msg ).addNewLine();
	}
    }

    txt += getFileInfo();
    setInfo( txt );
}


void uiSeisWvltMan::dispProperties( CallBacker* )
{
    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( curioobj_->key() );

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
    uiSelObjFromOtherSurvey objsel( this, ctxt );
    if ( !objsel.go() )
	return;

    Wavelet* wvlt = 0;
    WaveletLoader loader( objsel.ioObj() );
    uiRetVal rv = loader.read( wvlt );
    if ( rv.isError() )
	uiMSG().error( rv );

    CtxtIOObj ctio( ctxt );
    if ( !wvlt )
	mRet(uiStrings::sEmptyString())

    ctio.setName( wvlt->name() );
    DBM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	mRet(uiStrings::phrCannotCreate(tr("new entry in Object Management")))
    else if ( !loader.addToMGR(wvlt,ctio.ioobj_->key()) )
	mRet(uiStrings::phrCannotWrite(tr("wavelet to disk")))

    selgrp_->fullUpdate( ctio.ioobj_->key() );
    mRet( uiStrings::sEmptyString() )
}


#define mPrepWvltChg() \
    const DBKey ky( curioobj_->key() ); \
    RefMan<Wavelet> wvlt = WaveletMGR().fetchForEdit( ky ); \
    if ( !wvlt ) \
	return

#define mStoreWvltChg() \
    uiRetVal rv = WaveletMGR().store( *wvlt, ky ); \
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
    mkFileInfo();
}


void uiSeisWvltMan::taper( CallBacker* )
{
    mPrepWvltChg();

    uiSeisWvltTaperDlg dlg( this, *wvlt );
    uiString title = tr("Taper '%1'").arg(curioobj_->uiName());
    dlg.setCaption( title );
    if ( dlg.go() )
	{ mStoreWvltChg(); }
}


#define mErr() mErrRet( (uiStrings::phrJoinStrings(uiStrings::sCannot(),  \
				uiStrings::sDraw(), uiStrings::sWavelet())) );

void uiSeisWvltMan::rotUpdateCB( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltRotDlg*,dlg,cb);
    if ( !dlg )
	mErr();

    const Wavelet* wvlt = dlg->getWavelet();
    if ( !wvlt )
	mErr();

    dispWavelet( wvlt );
}


void uiSeisWvltMan::dispWavelet( const Wavelet* wvlt )
{
    wvnamdisp_->setText( curioobj_->uiName() );
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
