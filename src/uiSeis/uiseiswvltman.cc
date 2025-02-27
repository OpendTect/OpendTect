/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswvltman.h"

#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletattrib.h"
#include "waveletio.h"

#include "uiaxishandlerbase.h"
#include "uifuncdispbase.h"
#include "uifunctiondisplayserver.h"
#include "uilabel.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseiswvltattr.h"
#include "uiseiswvltgen.h"
#include "uiseiswvltimpexp.h"
#include "uiselobjothersurv.h"
#include "uistrings.h"
#include "uitoolbutton.h"
#include "uiwaveletextraction.h"
#include "uiwaveletmatchdlg.h"


#define mErrRet(s) { uiMSG().error(s); return; }

mDefineInstanceCreatedNotifierAccess(uiSeisWvltMan)


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
		uiStrings::phrManage(uiStrings::sWavelet(mPlural)),mNoDlgTitle,
		mODHelpKey(mSeisWvltManHelpID) ).nrstatusflds(1).modal(false),
		   WaveletTranslatorGroup::ioContext() )
    , wvltext_(0)
    , wvltpropdlg_(0)
{
    createDefaultUI();
    setPrefWidth( 50 );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    copybut_ = manipgrp->addButton( "copyobj",
		uiStrings::phrCopy(uiStrings::sWavelet()),
		mCB(this,uiSeisWvltMan,copyPush) );
    manipgrp->nextButtonOnNewRowCol();
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

    uiFuncDispBase::Setup fdsu;
    fdsu.noy2axis(true).noy2gridline(true);

    waveletdisplay_ = GetFunctionDisplayServer().createFunctionDisplay(
							    wvltdispgrp, fdsu );
    const uiString ztxt = toUiString("%1 %2").arg(SI().zIsTime() ?
	 uiStrings::sTime() : uiStrings::sDepth()).arg(SI().getUiZUnitString());
    waveletdisplay_->xAxis()->setCaption( ztxt );
    waveletdisplay_->yAxis(false)->setCaption( uiStrings::sAmplitude() );

    wvnamdisp_ = new uiLabel( wvltdispgrp, uiStrings::sWavelet() );
    wvnamdisp_->attach(centeredAbove, waveletdisplay_->uiobj());
    wvnamdisp_->setAlignment( Alignment::HCenter );

    mTriggerInstanceCreatedNotifier();
    windowClosed.notify( mCB(this,uiSeisWvltMan,closeDlg) );
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    if ( wvltext_ )
	wvltext_->extractionDone.remove(
				mCB(this,uiSeisWvltMan,wvltCreatedCB) );

    delete wvltext_;

    if ( wvltpropdlg_ )
	delete wvltpropdlg_;
}


void uiSeisWvltMan::addButtons()
{
    uiButtonGroup* grp = extraButtonGroup();
    new uiToolButton( grp, "impfromothsurv",
		      tr("Import Wavelet from other survey"),
		      mCB(this,uiSeisWvltMan,getFromOtherSurvey) );

    new uiToolButton( grp, "import",
		      tr("Import Wavelet from ASCII file"),
		      mCB(this,uiSeisWvltMan,impPush) );

    new uiToolButton( grp, "wavelet", tr("Create Wavelet"),
		      mCB(this,uiSeisWvltMan,crPush) );

    new uiToolButton( grp, "plus", tr("Stack Wavelets"),
		      mCB(this,uiSeisWvltMan,mrgPush) );

    new uiToolButton( grp, "wavelet_extract", tr("Extract"),
		      mCB(this,uiSeisWvltMan,extractPush) );

    new uiToolButton( grp, "wavelet_match", tr("Match Wavelets"),
		      mCB(this,uiSeisWvltMan,matchPush) );
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

    wvltext_ = new uiWaveletExtraction( this, is2d );
    wvltext_->extractionDone.notify( mCB(this,uiSeisWvltMan,wvltCreatedCB) );
    wvltext_->show();
}


void uiSeisWvltMan::matchPush( CallBacker* )
{
    uiWaveletMatchDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getMultiID() );
}


void uiSeisWvltMan::copyPush( CallBacker* )
{
    uiSeisWvltCopy copydlg( this, curioobj_ );
    if ( copydlg.go() )
	selgrp_->fullUpdate( copydlg.getMultiID() );
}


void uiSeisWvltMan::updateCB( CallBacker* )
{
}


void uiSeisWvltMan::wvltCreatedCB( CallBacker* )
{
    selgrp_->fullUpdate( wvltext_->storeKey() );
}


void uiSeisWvltMan::closeDlg( CallBacker* )
{
   if ( !wvltext_ ) return;
   wvltext_->close();
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
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    dispWavelet( wvlt );
    if ( wvlt )
    {
	const float zfac = mCast( float, SI().zDomain().userFactor() );
	WaveletAttrib wvltattrib( *wvlt );

	BufferString msg;
	msg.add( "Number of samples: " ).add( wvlt->size() ).addNewLine();
	msg.add( "Sample interval " ).add( SI().getZUnitString(true) )
	   .add( ": " ).add( wvlt->sampleRate() * zfac ).addNewLine();
	Interval<float> extremevals;
	wvlt->getExtrValues( extremevals );
	msg.add( "Min/Max amplitude: " ).add( extremevals.start_ )
		.add( "/" ).add( extremevals.stop_ ).addNewLine();
	float avgphase = wvltattrib.getAvgPhase( true );
	if ( mIsZero(avgphase,1e-3f) ) avgphase = 0.f;
	msg.add( "Average phase (deg): ").addDec( avgphase, 0 ).addNewLine();
	msg.add( "Polarity: ").add( wvltattrib.isNormalPolarity()
					? "Normal" : "Reverse" ).addNewLine();
	txt.add( msg );
	delete wvlt;

	MultiID orgid; MultiID horid; MultiID seisid; BufferString lvlnm;
	if ( Wavelet::isScaled(curioobj_->key(),orgid,horid,seisid,lvlnm) )
	{
	    msg = "Scaled: ";
	    if ( orgid == MultiID(0,-1) )
		msg.add( "Outside OpendTect" );
	    else
	    {
		msg.add( "'").add( IOM().nameOf(orgid) ).add( "'" );
		msg.add( " scaled to '").add( IOM().nameOf(seisid) ).add( "'" );
		msg.add( "\n\t(along '").add( IOM().nameOf(horid) ).add( "'" );
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
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;

    wvlt->setName( curioobj_->name().buf() );

    wvltpropdlg_ = new uiWaveletDispPropDlg( this, *wvlt );
    wvltpropdlg_->setCaption( tr("Wavelet '%1' Properties")
						    .arg(curioobj_->uiName()) );
    if ( wvltpropdlg_ ->go() )
    { delete wvltpropdlg_; wvltpropdlg_ = 0; }

    delete wvlt;
}


#define mRet(s) \
	{ ctio.setObj(0); if ( !s.isEmpty() ) uiMSG().error(s); return; }

void uiSeisWvltMan::getFromOtherSurvey( CallBacker* )
{
    CtxtIOObj ctio( mIOObjContext(Wavelet) );
    ctio.ctxt_.forread_ = true;

    uiSelObjFromOtherSurvey dlg( this, ctio );
    dlg.setHelpKey(mODHelpKey(mSeisWvltMangetFromOtherSurveyHelpID) );
    Wavelet* wvlt = 0;
    bool didsel = true;
    if ( dlg.go() )
	wvlt = Wavelet::get( ctio.ioobj_ );
    else
	didsel = false;

    dlg.setDirToCurrentSurvey();
    if ( !wvlt )
	mRet((didsel ? uiStrings::phrCannotRead(uiStrings::sWavelet()) :
		       uiStrings::sEmptyString()))
    IOM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	mRet(uiStrings::phrCannotCreate(tr("new entry in Object Management")))
    else if ( !wvlt->put(ctio.ioobj_) )
	mRet(uiStrings::phrCannotWrite(tr("wavelet to disk")))

    selgrp_->fullUpdate( ctio.ioobj_->key() );
    mRet( uiStrings::sEmptyString() )
}


bool uiSeisWvltMan::waveletSaveAs( const Wavelet& wvlt, const uiString& subj )
{
    CtxtIOObj ctio( mIOObjContext(Wavelet) );
    ctio.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio, uiStrings::phrSave(subj) );
    if ( dlg.go() )
    {
	const IOObj* ioobj = dlg.ioObj();
	if ( !ioobj )
	    return false;

	if ( !wvlt.put(ioobj) )
	{
	    uiMSG().error( uiStrings::phrCannotWrite(subj) );
	    return false;
	}
	else
	{
	    curioobj_ = ioobj->clone();
	    selgrp_->fullUpdate( curioobj_->key() );
	    selgrp_->setCurrent( curioobj_->key() );
	}

	return true;
    }

    return false;
}


void uiSeisWvltMan::reversePolarity( CallBacker* )
{
    PtrMan<Wavelet> wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt )
	return;

    wvlt->reverse();
    waveletSaveAs( *wvlt, tr("reverse polarity wavelet") );
}


void uiSeisWvltMan::rotatePhase( CallBacker* )
{
    PtrMan<Wavelet> wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt )
	return;

    uiSeisWvltRotDlg dlg( this, *wvlt );
    dlg.setCaption( curioobj_->uiName() );
    dlg.acting.notify( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    if ( dlg.go() )
	waveletSaveAs( *wvlt, tr("rotated phase wavelet") );

    dlg.acting.remove( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    mkFileInfo();
}


void uiSeisWvltMan::taper( CallBacker* )
{
    PtrMan<Wavelet> wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt )
	return;

    uiSeisWvltTaperDlg dlg( this, *wvlt );
    uiString title = tr("Taper '%1'").arg(curioobj_->uiName());
    dlg.setCaption( title );
    if ( dlg.go() )
	waveletSaveAs( *wvlt, tr("tapered wavelet") );
}


#define mErr() mErrRet( (uiStrings::phrJoinStrings(uiStrings::sCannot(),  \
				uiStrings::sDraw(), uiStrings::sWavelet())) );

void uiSeisWvltMan::rotUpdateCB( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltRotDlg*,dlg,cb);
    if ( !dlg ) mErr();

    const Wavelet* wvlt = dlg->getWavelet();
    if ( !wvlt ) mErr();

    dispWavelet( wvlt );
}


void uiSeisWvltMan::dispWavelet( const Wavelet* wvlt )
{
    wvnamdisp_->setText( curioobj_->uiName() );
    wvnamdisp_->setPrefWidthInChar( 60 );
    if( !wvlt || !wvlt->samples() )
    {
	waveletdisplay_->setEmpty();
	return;
    }
    const int wvltsz = wvlt->size();
    StepInterval<float> intxval = wvlt->samplePositions();
    const float zfac = mCast(float,SI().zDomain().userFactor());
    intxval.scale( zfac );
    waveletdisplay_->setVals( intxval, wvlt->samples(), wvltsz );
}
