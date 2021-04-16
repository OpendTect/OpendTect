/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/


#include "uiseisfileman.h"

#include "cbvsreadmgr.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "iopar.h"
#include "iostrm.h"
#include "seis2dlineio.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "seisstatscollector.h"
#include "timedepthconv.h"
#include "fileview.h"

#include "ui2dgeomman.h"
#include "uitoolbutton.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uimergeseis.h"
#include "uiseispsman.h"
#include "uiseissampleeditor.h"
#include "uiseiscopy.h"
#include "uiseisioobjinfo.h"
#include "uiseis2dfileman.h"
#include "uiseis2dgeom.h"
#include "uisplitter.h"
#include "uistatsdisplaywin.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiSeisFileMan)

class uiSeisSampleEdBrowserMgr : public CallBacker
{ mODTextTranslationClass(uiSeisSampleEdBrowserMgr)
public:

uiSeisSampleEdBrowserMgr()
{
    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef;
    bdef->tooltip_ = tr("Browse/Edit cube '%1'");
    bdef->cb_ = mCB(this,uiSeisSampleEdBrowserMgr,doBrowse);
    uiSeisFileMan::addBrowser( bdef );
}

void doBrowse( CallBacker* cb )
{
    mDynamicCastGet(uiSeisFileMan*,sfm,cb)
    if ( sfm && sfm->curIOObj() )
	uiSeisSampleEditor::launch( sfm, sfm->curIOObj()->key() );
}

};

static uiSeisSampleEdBrowserMgr* sampleedbrowsermgr_ = 0;


#define mHelpID is2d ? mODHelpKey(mSeisFileMan2DHelpID) : \
                       mODHelpKey(mSeisFileMan3DHelpID)
uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    :uiObjFileMan(p,uiDialog::Setup(is2d
            ? uiStrings::phrManage(tr("2D Seismics"))
            : uiStrings::phrManage(tr("3D Seismics")),
				    mNoDlgTitle,mHelpID)
				    .nrstatusflds(1).modal(false),
		  SeisTrcTranslatorGroup::ioContext(),
		    is2d ? "2D Seismic Data" : "Seismic Data" )
    , is2d_(is2d)
    , browsebut_(0)
    , man2dlinesbut_(0)
    , mergecubesbut_(0)
{
    if ( !sampleedbrowsermgr_ )
	sampleedbrowsermgr_ = new uiSeisSampleEdBrowserMgr;

    IOObjContext* freshctxt = Seis::getIOObjContext(
					is2d_ ? Seis::Line : Seis::Vol, true );
    ctxt_ = *freshctxt;
    delete freshctxt;

    createDefaultUI( true );
    selgrp_->getListField()->doubleClicked.notify(
				is2d_ ? mCB(this,uiSeisFileMan,man2DPush)
				      : mCB(this,uiSeisFileMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    copybut_ = manipgrp->addButton( "copyobj",
	uiStrings::phrCopy(is2d?uiStrings::sDataSet():uiStrings::sCube()),
				mCB(this,uiSeisFileMan,copyPush) );
    if ( is2d )
    {
	man2dlinesbut_ = manipgrp->addButton( "man2d", uiStrings::phrManage(
					   uiStrings::sLine(2)),
					   mCB(this,uiSeisFileMan,man2DPush) );
    }
    else
    {
	mergecubesbut_ = manipgrp->addButton( "mergeseis",uiStrings::phrMerge(
					tr("cube parts into one cube")),
					mCB(this,uiSeisFileMan,mergePush) );
	browsebut_ = manipgrp->addButton( "browseseis",
				tr("Browse/Edit this cube"),
				mCB(this,uiSeisFileMan,browsePush) );
    }

    histogrambut_ = manipgrp->addButton( "histogram", tr("Show histogram"),
				mCB(this,uiSeisFileMan,showHistogram) );
    attribbut_ = manipgrp->addButton( "attributes", sShowAttributeSet(),
				      mCB(this,uiSeisFileMan,showAttribSet) );

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}


static ObjectSet<uiSeisFileMan::BrowserDef> browserdefs_;


int uiSeisFileMan::addBrowser( uiSeisFileMan::BrowserDef* bd )
{
    browserdefs_ += bd;
    return browserdefs_.size() - 1;
}


#define mIsOfTranslName(nm) (FixedString(curioobj_->translator()) == nm)
#define mIsOfTranslType(typ) \
	mIsOfTranslName(typ##SeisTrcTranslator::translKey())


void uiSeisFileMan::ownSelChg()
{
    setToolButtonProperties();
}


#define mSetButToolTip(but,tt,deftt) \
    { \
	if ( but ) \
	    but->setToolTip( but->isSensitive() ? tt : deftt ); \
    }

void uiSeisFileMan::setToolButtonProperties()
{
    BufferString cursel;
    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    uiString tt;
    copybut_->setSensitive( !cursel.isEmpty() );
    mSetButToolTip(copybut_,
	tr("Make a Copy of '%1'").arg(cursel),
	uiStrings::phrCopy(is2d_?uiStrings::sDataSet():uiStrings::sCube()) );

    if ( browsebut_ )
    {
	const BrowserDef* bdef = getBrowserDef();
	const bool enabbrowse = curimplexists_ && bdef;
	browsebut_->setSensitive( enabbrowse );
	if ( !enabbrowse )
	    mSetButToolTip( browsebut_,
			    tr("No browser for '%1'").arg(cursel),
			    tr("Browse/edit selected cube") )
	else
	{
	    uiString bdeftt( bdef->tooltip_ );
	    browsebut_->setToolTip( bdeftt.arg(curioobj_->name()) );
	}
    }

    if ( mergecubesbut_ )
    {
	BufferStringSet selcubenms;
	selgrp_->getChosen( selcubenms );
	if ( selcubenms.size() > 1 )
	    mSetButToolTip(mergecubesbut_,
		tr("Merge %1").arg(selcubenms.getDispString(2)),
		uiStrings::phrMerge(uiStrings::sCube(mPlural).toLower()))
	else
	    mergecubesbut_->setToolTip( uiStrings::phrMerge(
					    uiStrings::sCube(2).toLower()) );
    }

    if ( man2dlinesbut_ )
    {
	man2dlinesbut_->setSensitive( !cursel.isEmpty() );
	mSetButToolTip(man2dlinesbut_,
		uiStrings::phrManage(tr("2D lines in '%1'")).arg(cursel),
		uiStrings::phrManage(uiStrings::sLine(2)))
    }

    if ( histogrambut_ )
    {
	const SeisIOObjInfo info( curioobj_ );
	if ( !info.haveStats() )
	    info.getDataDistribution();
	histogrambut_->setSensitive( info.haveStats() );
    }

    if ( attribbut_ )
    {
	attribbut_->setSensitive( curioobj_ );
	if ( curioobj_ )
	{
	     File::Path fp( curioobj_->mainFileName() );
	     fp.setExtension( sProcFileExtension() );
	     attribbut_->setSensitive( File::exists(fp.fullPath()) );
	     mSetButToolTip( attribbut_,
			tr("Show AttributeSet for '%1'").arg(cursel),
			sShowAttributeSet())
	}
	else
	    attribbut_->setToolTip( sShowAttributeSet() );
    }
}


const uiSeisFileMan::BrowserDef* uiSeisFileMan::getBrowserDef() const
{
    if ( curioobj_ )
    {
	for ( auto ipass : {0,1} )
	{
	    for ( int idx=0; idx<browserdefs_.size(); idx++ )
	    {
		const BrowserDef* bdef = browserdefs_[idx];
		if ( ipass == 0 && bdef->name_ == curioobj_->translator() )
		    return bdef;
		else if ( ipass == 1 && bdef->name_.isEmpty() )
		    return bdef;
	    }
	}
    }

    return 0;
}


bool uiSeisFileMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    SeisIOObjInfo oinf( ioobj );
    oinf.getUserInfo( inf );
    return oinf.isOK();
}


od_int64 uiSeisFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    return SeisIOObjInfo::getFileSize( filenm, nrfiles );
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const DBKey key( curioobj_->key() );
    DBKeySet chsnmids;
    selgrp_->getChosen( chsnmids );
    uiMergeSeis dlg( this );
    dlg.setInputIds( chsnmids );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::browsePush( CallBacker* )
{
    const BrowserDef* bdef = getBrowserDef();
    if ( bdef )
    {
	CallBack cb( bdef->cb_ );
	cb.doCall( this );
    }
}


void uiSeisFileMan::man2DPush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const DBKey key( curioobj_->key() );
    uiSeis2DFileMan dlg( this, *curioobj_ );
    dlg.go();

    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    uiSeisCopy dlg( this, curioobj_ );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.copiedID() );
}


void uiSeisFileMan::manPS( CallBacker* )
{
    uiSeisPreStackMan dlg( this, is2d_ );
    dlg.go();
}


void uiSeisFileMan::showHistogram( CallBacker* )
{
    const SeisIOObjInfo info( curioobj_ ); IOPar iop;
    if ( !info.getStats(iop) )
	return;

    uiStatsDisplay::Setup su;
    uiStatsDisplayWin* statswin = new uiStatsDisplayWin( this, su, 1, false );
    statswin->setDeleteOnClose( true );

    ConstRefMan<FloatDistrib> distrib
			= Seis::StatsCollector::getDistribution( iop );
    if ( !distrib )
	statswin->statsDisplay()->usePar( iop );
    else
    {
	const Interval<float> rg = Seis::StatsCollector::getExtremes( iop );
	const od_int64 nrsamples = Seis::StatsCollector::getNrSamples( iop );
	statswin->statsDisplay()->setData( *distrib, nrsamples, rg );
    }

    BufferString datanm( curioobj_->name() );
    const FixedString findres( iop.find( sKey::Source() ) );
    if ( !findres.isEmpty() )
    {
	if ( findres == SeisIOObjInfo::sKeyPartialScan() )
	    datanm.add( " [Partial Scan]" );
	else if ( findres == SeisIOObjInfo::sKeyFullScan() )
	    datanm.add( " [Full Scan]" );
    }

    statswin->setDataName( datanm );
    statswin->show();
}


void uiSeisFileMan::showAttribSet( CallBacker* )
{
    if ( !curioobj_ ) return;

    File::Path fp( curioobj_->mainFileName() );
    fp.setExtension( sProcFileExtension() );
    File::launchViewer( fp.fullPath(), File::ViewPars() );
}
