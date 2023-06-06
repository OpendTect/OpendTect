/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellfiltergrp.h"

#include "welldatafilter.h"

#include "ioman.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uimnemonicsel.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uitoolbutton.h"
#include "wellman.h"
#include "wellman.h"
#include "welltransl.h"

#include "hiddenparam.h"

class InitDesc
{
public:

					InitDesc()	{}
					~InitDesc()	{}

    BufferStringSet			selwellnms_;
    BufferStringSet			sellognms_;
    BufferStringSet			selmrkrnms_;
    MnemonicSelection			selmns_;
    bool				logmode_ = true;
    const ObjectSet<Well::Data>*	wds_ = nullptr;
    MnemonicSelection			mns_;
    BufferStringSet			lognms_;
    BufferStringSet			markernms_;
};


static HiddenParam<uiWellFilterGrp,InitDesc*> hp( nullptr );
static HiddenParam<uiWellFilterGrp,uiComboBox*> selopscbhp( nullptr );
static HiddenParam<uiWellFilterGrp,int> basedonentiresethp( 1 );
static const int cSelWellsEntireSet = 0;

uiWellFilterGrp::uiWellFilterGrp( uiParent* p, OD::Orientation orient )
    : uiGroup(p)
    , orient_(orient)
    , markerSelectionChg(this)
{
    hp.setParam( this, new InitDesc );
    selopscbhp.setParam( this, nullptr );
    basedonentiresethp.setParam( this, 1 );

    auto* maingrp = new uiGroup( this, "Main group" );
    const bool hor = orient_ == OD::Horizontal;
    const IOObjContext ctxt = mIOObjContext( Well );
    uiIOObjSelGrp::Setup suw( OD::ChooseZeroOrMore );
    suw.withinserters(false);
    uiIOObjSelGrp* welllistselgrp = new uiIOObjSelGrp( maingrp, ctxt, suw );
    welllistselgrp->displayManipGroup( false, true );
    welllist_ = welllistselgrp->getListField();
    welllist_->chooseAll();
    welllist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    logormnslist_ = new uiListBox( maingrp, "logs", OD::ChooseZeroOrMore );
    logormnsfilter_ = new uiListBoxFilter( *logormnslist_ );
    logormnslist_->setHSzPol( uiObject::Wide );
    logormnslist_->attach( hor ? rightOf : alignedBelow, welllistselgrp );
    logormnslist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    markerlist_ = new uiListBox( maingrp, "markers", OD::ChooseZeroOrMore );
    markerfilter_ = new uiListBoxFilter( *markerlist_ );
    markerlist_->attach( hor ? rightOf : alignedBelow, logormnslist_ );
    markerlist_->setHSzPol( uiObject::Wide );
    markerlist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    CallBack cb = mCB(this,uiWellFilterGrp,selButPush);
    fromwellbut_ = new uiToolButton( maingrp,
		 hor ? uiToolButton::RightArrow : uiToolButton::DownArrow,
		tr("Show logs/markers present selected wells"), cb );
    fromwellbut_->attach(hor ? centeredBelow : centeredRightOf, welllistselgrp);
    fromlogormnsbut_ = new uiToolButton( maingrp,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected logs/mnemonics"), cb );
    fromlogormnsbut_->attach( hor ? centeredBelow : centeredRightOf,
	    		      logormnslist_ );
    frommarkerbut_ = new uiToolButton( maingrp,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected markers"), cb );
    frommarkerbut_->attach( hor ? centeredBelow : centeredRightOf,
			    markerlist_ );

    if ( hor )
    {
	uiSeparator* sep = new uiSeparator( this );
	sep->attach( centeredBelow, maingrp );

	auto* optionsgrp = new uiGroup( this, "Selection options goup" );
	uiLabel* lbl = new uiLabel( optionsgrp,
		    tr( "Make Well selection from Logs/markers based on: " ) );
	uiStringSet strs( tr( "Entire selected logs/markers set" ),
			  tr( "Selected logs/markers individually" ) );
	auto* seloptionscb = new uiComboBox( optionsgrp,
					strs, "Selection options" );
	seloptionscb->setStretch( 0, 0 );
	optionsgrp->attach( ensureBelow, sep );
	seloptionscb->attach( rightOf, lbl );
	mAttachCB( seloptionscb->selectionChanged,
		   uiWellFilterGrp::fromSelTypeChgdCB );
	selopscbhp.setParam( this, seloptionscb );
    }

    mAttachCB( welllistselgrp->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( logormnslist_->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( markerlist_->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( markerlist_->selectionChanged, uiWellFilterGrp::markerSelChgCB );
    mAttachCB( postFinalize(), uiWellFilterGrp::initGrp );
}


uiWellFilterGrp::uiWellFilterGrp( uiParent* p, const ObjectSet<Well::Data>& wds,
				  const BufferStringSet& lognms,
				  const BufferStringSet& markernms,
				  OD::Orientation orient )
    : uiWellFilterGrp(p, orient)
{
    setFilterItems( wds, lognms, markernms );
}


uiWellFilterGrp::uiWellFilterGrp( uiParent* p, const ObjectSet<Well::Data>& wds,
				  const MnemonicSelection& mns,
				  const BufferStringSet& markernms,
				  OD::Orientation orient )
    : uiWellFilterGrp(p, orient)
{
    setFilterItems( wds, mns, markernms );
}


uiWellFilterGrp::~uiWellFilterGrp()
{
    detachAllNotifiers();
    hp.removeAndDeleteParam( this );
    selopscbhp.removeParam( this );
}


void uiWellFilterGrp::initGrp( CallBacker* )
{
    const InitDesc& initdesc = *hp.getParam(this);
    fillListBoxes();
    initdesc.logmode_ ? setSelection( initdesc.selwellnms_,
				      initdesc.sellognms_,
				      initdesc.selmrkrnms_ )
		      : setSelection( initdesc.selwellnms_, initdesc.selmns_,
				      initdesc.selmrkrnms_ );
}


void uiWellFilterGrp::setFilterItems( const ObjectSet<Well::Data>& wds,
				      const BufferStringSet& lognms,
				      const BufferStringSet& markernms )
{
    InitDesc& initdesc = *hp.getParam(this);
    initdesc.wds_ = &wds;
    initdesc.logmode_ = true;
    initdesc.mns_.setEmpty();
    BufferStringSet sortedlognms = lognms;
    sortedlognms.sort();
    initdesc.lognms_ = sortedlognms;
    initdesc.markernms_ = markernms;
}


void uiWellFilterGrp::setFilterItems( const ObjectSet<Well::Data>& wds,
				      const MnemonicSelection& mns,
				      const BufferStringSet& markernms )
{
    InitDesc& initdesc = *hp.getParam(this);
    initdesc.wds_ = &wds;
    initdesc.mns_ = mns;
    initdesc.logmode_ = false;
    initdesc.markernms_ = markernms;
}


void uiWellFilterGrp::fillListBoxes()
{
    const InitDesc& initdesc = *hp.getParam(this);
    if ( initdesc.logmode_ )
    {
	logormnsfilter_->setItems( initdesc.lognms_ );
	logormnslist_->chooseAll();
    }
    else if ( !initdesc.mns_.isEmpty() )
    {
	BufferStringSet mnemnms;
	for ( const auto* mn : initdesc.mns_ )
	    mnemnms.addIfNew( mn->name() );

	logormnsfilter_->setItems( mnemnms );
	logormnslist_->chooseAll();
    }

    markerfilter_->setItems( initdesc.markernms_ );
    markerlist_->chooseAll();
    setMaxLinesForLists();
}


void uiWellFilterGrp::setLogMode( bool yn )
{
    const InitDesc& initdesc = *hp.getParam(this);
    if ( initdesc.logmode_ == yn )
	return;

    if ( yn )
    {
	BufferStringSet lognms;
	Well::Man::getAllLogNames( lognms, true );
	setFilterItems( *initdesc.wds_, lognms, initdesc.markernms_ );
    }
    else
    {
	MnemonicSelection mns;
	Well::Man::getAllMnemonics( mns, true );
	setFilterItems( *initdesc.wds_, mns, initdesc.markernms_ );
    }
}


void uiWellFilterGrp::setMaxLinesForLists()
{
    if ( orient_==OD::Vertical )
	return;

    int maxsz = mMAX( welllist_->size(),
		      mMAX(logormnslist_->size(),markerlist_->size()) );
    if ( maxsz > 25 )
	maxsz = 25;

    welllist_->setNrLines( maxsz );
    logormnslist_->setNrLines( maxsz );
    markerlist_->setNrLines( maxsz );
}


void uiWellFilterGrp::setSelected( const DBKeySet& wellids,
				   const BufferStringSet& lognms,
				   const BufferStringSet& mrkrnms )
{
    BufferStringSet wellnms;
    for ( const auto* wellid : wellids )
	wellnms.add( IOM().objectName(*wellid) );

    setSelected( wellnms, lognms, mrkrnms );
}


void uiWellFilterGrp::setSelected( const BufferStringSet& wellnms,
				   const BufferStringSet& lognms,
				   const BufferStringSet& mrkrnms )
{
    if ( !finalized() )
    {
	fillInitSelection( wellnms, lognms, mrkrnms );
	return;
    }

    setSelection( wellnms, lognms, mrkrnms );
}


void uiWellFilterGrp::setSelected( const BufferStringSet& wellnms,
				   const MnemonicSelection& mns,
				   const BufferStringSet& mrkrnms )
{
    if ( !finalized() )
    {
	fillInitSelection( wellnms, mns, mrkrnms );
	return;
    }

    setSelection( wellnms, mns, mrkrnms );
}


void uiWellFilterGrp::setSelected( const DBKeySet& wellids,
				   const MnemonicSelection& mns,
				   const BufferStringSet& mrkrnms )
{
    BufferStringSet wellnms;
    for ( const auto* wellid : wellids )
	wellnms.add( IOM().objectName(*wellid) );

    setSelected( wellnms, mns, mrkrnms );
}


void uiWellFilterGrp::setSelection( const BufferStringSet& wellnms,
				    const BufferStringSet& lognms,
				    const BufferStringSet& mrkrnms )
{
    const InitDesc& initdesc = *hp.getParam(this);
    welllist_->setChosen( wellnms );
    if ( initdesc.logmode_ )
	logormnslist_->setChosen( lognms );

    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}


void uiWellFilterGrp::setSelection( const BufferStringSet& wellnms,
				    const MnemonicSelection& mns,
				    const BufferStringSet& mrkrnms )
{
    const InitDesc& initdesc = *hp.getParam(this);
    welllist_->setChosen( wellnms );
    BufferStringSet mnemnms;
    for ( const auto* mn : mns )
	mnemnms.addIfNew( mn->name() );

    if ( !initdesc.logmode_ )
	logormnslist_->setChosen( mnemnms );

    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}


void uiWellFilterGrp::getSelected( DBKeySet& wellids,
				   BufferStringSet& lognms,
				   BufferStringSet& mrkrnms ) const
{
    BufferStringSet wellnms;
    getSelected( wellnms, lognms, mrkrnms );
    wellids.setEmpty();
    for ( const auto* wellnm : wellnms )
    {
	const IOObj* ioobj = Well::findIOObj( *wellnm, nullptr );
	if ( !ioobj )
	    continue;
	wellids += ioobj->key();
    }
}


void uiWellFilterGrp::getSelected( BufferStringSet& wellnms,
				   BufferStringSet& lognms,
				   BufferStringSet& mrkrnms ) const
{
    const InitDesc& initdesc = *hp.getParam(this);
    welllist_->getChosen( wellnms );
    if ( initdesc.logmode_ )
	logormnslist_->getChosen( lognms );
    else
	lognms.setEmpty();

    markerlist_->getChosen( mrkrnms );
}


void uiWellFilterGrp::getSelected( BufferStringSet& wellnms,
				   MnemonicSelection& mns,
				   BufferStringSet& mrkrnms ) const
{
    const InitDesc& initdesc = *hp.getParam(this);
    welllist_->getChosen( wellnms );
    mns.setEmpty();
    if ( !initdesc.logmode_ )
    {
	BufferStringSet selmnnms;
	logormnslist_->getChosen( selmnnms );
	for ( const auto* mnnm : selmnnms )
	    mns.addIfNew( initdesc.mns_.getByName(*mnnm) );
    }

    markerlist_->getChosen( mrkrnms );
}


void uiWellFilterGrp::getSelected( DBKeySet& wellids,
				   MnemonicSelection& mns,
				   BufferStringSet& mrkrnms ) const
{
    BufferStringSet wellnms;
    getSelected( wellnms, mns, mrkrnms );
    wellids.setEmpty();
    for ( const auto* wellnm : wellnms )
    {
	const IOObj* ioobj = Well::findIOObj( *wellnm, nullptr );
	if ( !ioobj )
	    continue;
	wellids += ioobj->key();
    }
}


BufferStringSet uiWellFilterGrp::getSelectedMarkers() const
{
    BufferStringSet mrkrnms;
    markerlist_->getChosen( mrkrnms );
    return mrkrnms;
}


void uiWellFilterGrp::noLogFilterCB( CallBacker* )
{
    const InitDesc& initdesc = *hp.getParam(this);
    BufferStringSet wellstohide;
    TypeSet<int> idxstohide;
    Well::WellDataFilter wdf( *initdesc.wds_ );
    wdf.getWellsWithNoLogs( wellstohide );
    for ( const auto* wellnm : wellstohide )
	idxstohide += welllist_->indexOf( *wellnm );

    for ( const auto& idx : idxstohide )
	welllist_->setChosen( idx, false );
}


void uiWellFilterGrp::mnemFilterCB( CallBacker* )
{
    const InitDesc& initdesc = *hp.getParam(this);
    MnemonicSelection mns;
    BufferStringSet suitablelogs;
    uiMultiMnemonicsSel dlg( this, mns );
    if ( !dlg.go() )
	return;

    Well::WellDataFilter wdf( *initdesc.wds_ );
    wdf.getLogsForMnems( mns, suitablelogs );
    logormnslist_->setChosen( suitablelogs );
}


void uiWellFilterGrp::wellTypeFilter( OD::WellType wt )
{
    const InitDesc& initdesc = *hp.getParam(this);
    BufferStringSet wellnms;
    Well::WellDataFilter wdf( *initdesc.wds_ );
    wdf.getWellsOfType( wt, wellnms );
    welllist_->setChosen( wellnms );
}


void uiWellFilterGrp::markerZoneFilter( const BufferString& topnm,
					const BufferString& botnm )
{
    const InitDesc& initdesc = *hp.getParam(this);
    BufferStringSet wellnms, lognms, markernms;
    MnemonicSelection mns;
    markernms.add( topnm ).add( botnm );
    Well::WellDataFilter wdf( *initdesc.wds_ );
    wdf.getWellsFromMarkers( markernms, wellnms );
    if ( initdesc.logmode_ )
    {
	wdf.getLogsInMarkerZone( wellnms, topnm, botnm, lognms );
	welllist_->setChosen( wellnms );
	logormnslist_->setChosen( lognms );
    }
    else
    {
	BufferStringSet mnnms;
	wdf.getMnemsInMarkerZone( wellnms, topnm, botnm, mns );
	welllist_->setChosen( wellnms );
	for ( const auto* mn : mns )
	    mnnms.add( mn->name() );

	logormnslist_->setChosen( mnnms );
    }
}


void uiWellFilterGrp::depthRangeFilter( const Interval<float> depthrg )
{
    const InitDesc& initdesc = *hp.getParam(this);
    BufferStringSet wellnms, lognms;
    MnemonicSelection mns;
    Well::WellDataFilter wdf( *initdesc.wds_ );
    if ( initdesc.logmode_ )
    {
	wdf.getLogsInDepthInterval( depthrg, wellnms, lognms );
	welllist_->setChosen( wellnms );
	logormnslist_->setChosen( lognms );
    }
    else
    {
	BufferStringSet mnnms;
	wdf.getMnemsInDepthInterval( depthrg, wellnms, mns );
	welllist_->setChosen( wellnms );
	for ( const auto* mn : mns )
	    mnnms.add( mn->name() );

	logormnslist_->setChosen( mnnms );
    }
}


void uiWellFilterGrp::logValRangeFilter( const MnemonicSelection& mns,
				 const TypeSet<Interval<float>>& logvalrg )
{
    const InitDesc& initdesc = *hp.getParam(this);
    BufferStringSet wellnms, lognms;
    Well::WellDataFilter wdf( *initdesc.wds_ );
    wdf.getLogsInValRange( mns, logvalrg, wellnms, lognms );
    welllist_->setChosen( wellnms );
    logormnslist_->setChosen( lognms );
}


void uiWellFilterGrp::fromSelTypeChgdCB( CallBacker* )
{
    uiComboBox* seloptionscb = selopscbhp.getParam( this );
    if ( !seloptionscb )
	return;

    const int seloption = seloptionscb->currentItem();
    if ( seloption == cSelWellsEntireSet )
	basedonentiresethp.setParam( this, 1 );
    else
	basedonentiresethp.setParam( this, 0 );
}


void uiWellFilterGrp::selChgCB( CallBacker* )
{
    const InitDesc& initdesc = *hp.getParam(this);
    const int selwells = welllist_->nrChosen();
    const int totalwells = welllist_->size();
    welllist_->setLabelText( tr("Selected Wells %1/%2").arg(selwells).
							arg(totalwells), 0 );

    const char* logormn = initdesc.logmode_ ? "Logs" : "Mnemonics";
    const int sellogsormns = logormnslist_->nrChosen();
    const int totallogsormns =	logormnslist_->size();
    logormnslist_->setLabelText( tr("Selected %1 %2/%3").arg(logormn)
						    .arg(sellogsormns)
						    .arg(totallogsormns), 0 );

    const int selmarkers = markerlist_->nrChosen();
    const int totalmarkers = markerlist_->size();
    markerlist_->setLabelText( tr("Selected Markers %1/%2").arg(selmarkers)
						     .arg(totalmarkers), 0 );
}


void uiWellFilterGrp::markerSelChgCB( CallBacker* )
{
    markerSelectionChg.trigger();
}


void uiWellFilterGrp::selButPush( CallBacker* cb )
{
    const InitDesc& initdesc = *hp.getParam( this );
    const bool basedonentireset = basedonentiresethp.getParam(this) == 1;
    mDynamicCastGet(uiToolButton*,but,cb)
    BufferStringSet wellnames, lognames, markernames;
    MnemonicSelection mns;
    Well::WellDataFilter wdf( *initdesc.wds_ );
    if ( but == fromwellbut_ )
    {
	welllist_->getChosen( wellnames );
	wdf.getMarkersLogsMnemsFromWells( wellnames,
					  lognames, mns, markernames );
	if ( initdesc.logmode_ )
	    logormnslist_->setChosen( lognames );
	else
	{
	    BufferStringSet mnnms;
	    for ( const auto* mn : mns )
		mnnms.add( mn->name() );

	    logormnslist_->setChosen( mnnms );
	}

	markerlist_->setChosen( markernames );
    }
    else if ( but == fromlogormnsbut_ )
    {
	if ( initdesc.logmode_ )
	{
	    logormnslist_->getChosen( lognames );
	    wdf.getWellSelFromLogs( lognames, wellnames, basedonentireset );
	}
	else
	{
	    BufferStringSet mnnms;
	    logormnslist_->getChosen( mnnms );
	    for ( const auto* mnnm : mnnms )
		mns.addIfNew( initdesc.mns_.getByName(*mnnm) );

	    wdf.getWellSelFromMnems( mns, wellnames, basedonentireset );
	}

	welllist_->setChosen( wellnames );
    }
    else if ( but == frommarkerbut_ )
    {
	markerlist_->getChosen( markernames );
	wdf.getWellSelFromMarkers( markernames, wellnames, basedonentireset );
	welllist_->setChosen( wellnames );
    }
}


void uiWellFilterGrp::fillInitSelection( const BufferStringSet& wellnms,
					 const BufferStringSet& lognms,
					 const BufferStringSet& mrkrnms )
{
    InitDesc& initdesc = *hp.getParam(this);
    initdesc.selwellnms_ = wellnms;
    initdesc.sellognms_ = lognms;
    initdesc.selmrkrnms_ = mrkrnms;
}


void uiWellFilterGrp::fillInitSelection( const BufferStringSet& wellnms,
					 const MnemonicSelection& mns,
					 const BufferStringSet& mrkrnms )
{
    InitDesc& initdesc = *hp.getParam(this);
    initdesc.selwellnms_ = wellnms;
    initdesc.selmns_ = mns;
    initdesc.selmrkrnms_ = mrkrnms;
}


bool uiWellFilterGrp::getLogMode() const
{
    const InitDesc& initdesc = *hp.getParam(this);
    return initdesc.logmode_;
}
