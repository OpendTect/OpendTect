/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiseiswvltman.cc,v 1.78 2012-07-20 05:02:26 cvsnageswara Exp $";


#include "uiseiswvltman.h"

#include "arrayndimpl.h"
#include "color.h"
#include "flatposdata.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uibutton.h"
#include "uiseissingtrcdisp.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseiswvltattr.h"
#include "uiseiswvltgen.h"
#include "uiseiswvltimpexp.h"
#include "uiselobjothersurv.h"
#include "uitextedit.h"
#include "uiwaveletextraction.h"


#define mErrRet(s) { uiMSG().error(s); return; }

mDefineInstanceCreatedNotifierAccess(uiSeisWvltMan)


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Wavelets",mNoDlgTitle,
                                     "103.3.0").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
    , wvltext_(0)
    , wvltpropdlg_(0)			 
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "impfromothsurv", "Get from other survey",
			mCB(this,uiSeisWvltMan,getFromOtherSurvey) );
    manipgrp->addButton( "info", "Display properties",
			mCB(this,uiSeisWvltMan,dispProperties) );
    manipgrp->addButton( "revpol", "Reverse polarity",
			mCB(this,uiSeisWvltMan,reversePolarity) );
    manipgrp->addButton( "phase", "Rotate phase",
			mCB(this,uiSeisWvltMan,rotatePhase) );
    manipgrp->addButton( "wavelet_taper", "Taper",
			mCB(this,uiSeisWvltMan,taper) );

    butgrp_ = new uiGroup( listgrp_, "Imp/Create buttons" );
    uiPushButton* impbut = new uiPushButton( butgrp_, "&Import", false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );
    impbut->setPrefWidthInChar( 12 );
    uiPushButton* crbut = new uiPushButton( butgrp_, "&Generate", false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );
    crbut->setPrefWidthInChar( 12 );
    uiPushButton* mergebut = new uiPushButton( butgrp_, "&Merge", false );
    mergebut->activated.notify( mCB(this,uiSeisWvltMan,mrgPush) );
    mergebut->attach( rightOf, crbut );
    mergebut->setPrefWidthInChar( 12 );
    uiPushButton* extractbut = new uiPushButton( butgrp_, "&Extract", false );
    extractbut->activated.notify( mCB(this,uiSeisWvltMan,extractPush) );
    extractbut->attach( rightOf, mergebut );
    extractbut->setPrefWidthInChar( 12 );
    butgrp_->attach( centeredBelow, selgrp_ );

    trcdisp_ = new uiSeisSingleTraceDisplay( listgrp_ );
    trcdisp_->setPrefWidth( 100 );
    trcdisp_->attach( ensureRightOf, selgrp_ );
    trcdisp_->setStretch( 1, 2 );

    selChg( this );
    mTriggerInstanceCreatedNotifier();
    windowClosed.notify( mCB(this,uiSeisWvltMan,closeDlg) );
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    if ( wvltext_ )
    {
	wvltext_->close();
	wvltext_->extractionDone.remove( mCB(this,uiSeisWvltMan,updateCB) );
    }

    delete wvltext_;

    if ( wvltpropdlg_ )
	delete wvltpropdlg_;
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
	mErrRet( "At least two wavelets are needed to merge wavelets" );

    uiSeisWvltMerge dlg( this, curioobj_ ? curioobj_->name() : 0 );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.storeKey() );
}


void uiSeisWvltMan::extractPush( CallBacker* cb )
{
    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().askGoOnAfter( "Which data do you want to use?",
		0, "&3D", "&2D" );
	if ( res == 2 )
	    return;
	else
	    is2d = res;
    }

    if ( !wvltext_ || wvltext_->isHidden() )
    {
	wvltext_ = new uiWaveletExtraction( 0, is2d );
	wvltext_->extractionDone.notify( mCB(this,uiSeisWvltMan,updateCB) );
    }

    wvltext_->show();
}


void uiSeisWvltMan::updateCB( CallBacker* )
{
    selgrp_->fullUpdate( wvltext_->storeKey() );
}


void uiSeisWvltMan::closeDlg( CallBacker* )
{
   if ( !wvltext_ ) return;
   wvltext_->close();
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    trcdisp_->setData( wvlt );

    if ( wvlt )
    {
	const float zfac = SI().zDomain().userFactor();

	BufferString tmp;
	tmp.add( "Number of samples: " ).add( wvlt->size() ).add( "\n" );
	tmp.add( "Sample interval " ).add( SI().getZUnitString(true) )
	   .add( ": " ).add( wvlt->sampleRate() * zfac ).add( "\n" );
	tmp.add( "Min/Max amplitude: " ).add( wvlt->getExtrValue(false) )
	   .add( "/" ).add( wvlt->getExtrValue() ).add( "\n" ); 
	txt.add( tmp );
	delete wvlt;

	MultiID orgid; MultiID horid; MultiID seisid; BufferString lvlnm;
	if ( Wavelet::isScaled(curioobj_->key(),orgid,horid,seisid,lvlnm) )
	{
	    tmp = "Scaled: ";
	    if ( orgid == MultiID("0") )
		tmp.add( "Outside OpendTect" );
	    else
	    {
		tmp.add( "'").add( IOM().nameOf(orgid) ).add( "'" );
		tmp.add( " scaled to '").add( IOM().nameOf(seisid) ).add( "'" );
		tmp.add( "\n\t(along '").add( IOM().nameOf(horid) ).add( "'" );
		tmp.add( " at '").add( lvlnm ).add( "')" );
	    }
	    txt.add( tmp ).add( "\n" );
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
    if ( wvltpropdlg_ ->go() )
    { delete wvltpropdlg_; wvltpropdlg_ = 0; }

    delete wvlt;
}


#define mRet(s) \
	{ ctio.setObj(0); delete &ctio; if ( s ) uiMSG().error(s); return; }

void uiSeisWvltMan::getFromOtherSurvey( CallBacker* )
{
    CtxtIOObj& ctio = *mMkCtxtIOObj(Wavelet);
    ctio.ctxt.forread = true;

    uiSelObjFromOtherSurvey dlg( this, ctio );
    dlg.setHelpID("0.3.11");
    Wavelet* wvlt = 0;
    bool didsel = true;
    if ( dlg.go() ) 
	wvlt = Wavelet::get( ctio.ioobj );
    else
	didsel = false;

    dlg.setDirToCurrentSurvey();
    if ( !wvlt )
	mRet((didsel?"Could not read wavelet":0))
    IOM().getEntry( ctio );
    if ( !ctio.ioobj )
	mRet("Cannot create new entry in Object Management")
    else if ( !wvlt->put(ctio.ioobj) )
	mRet("Cannot write wavelet to disk")

    selgrp_->fullUpdate( ctio.ioobj->key() );
    mRet( 0 )
}


void uiSeisWvltMan::reversePolarity( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;

    float* samps = wvlt->samples();
    for ( int idx=0; idx<wvlt->size(); idx++ )
	samps[idx] *= -1;

    if ( !wvlt->put(curioobj_) )
	uiMSG().error("Cannot write new polarity reversed wavelet to disk");
    else
	selgrp_->fullUpdate( curioobj_->key() );

    delete wvlt;
}


void uiSeisWvltMan::rotatePhase( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;
    
    uiSeisWvltRotDlg dlg( this, *wvlt );
    dlg.acting.notify( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    if ( dlg.go() )
    {	
	if ( !wvlt->put(curioobj_) )
	    uiMSG().error("Cannot write rotated phase wavelet to disk");
	else
	    selgrp_->fullUpdate( curioobj_->key() );
    }

    dlg.acting.remove( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    mkFileInfo();

    delete wvlt;
}


void uiSeisWvltMan::taper( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;
    
    uiSeisWvltTaperDlg dlg( this, *wvlt );
    if ( dlg.go() )
    {	
	if ( !wvlt->put(curioobj_) )
	    uiMSG().error("Cannot write tapered wavelet to disk");
	else
	    selgrp_->fullUpdate( curioobj_->key() );
    }
}


#define mErr() mErrRet("Cannot draw wavelet");

void uiSeisWvltMan::rotUpdateCB( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltRotDlg*,dlg,cb);
    if ( !dlg ) mErr();

    const Wavelet* wvlt = dlg->getWavelet();
    if ( !wvlt ) mErr();

    trcdisp_->setData( wvlt );
}

