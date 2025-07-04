/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiregmarkermgr.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uistratlvllist.h"
#include "uistrattreewin.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "stratreftree.h"
#include "strattreetransl.h"
#include "stratunitrepos.h"
#include "survinfo.h"
#include "wellmarker.h"
#include "wellwriter.h"

uiChooseRegMarkerDlg::uiChooseRegMarkerDlg( uiParent* p,
					    const BufferStringSet& names )
    : uiDialog(p,Setup(tr("Set one as Regional Marker"),mNoHelpKey))
{
    list_ = new uiListBox( this, tr("Well markers"),
			   OD::ChooseOnlyOne, uiListBox::AboveMid );
    list_->addItems( names );
    list_->setHSzPol( uiObject::Wide );
    setButtonText( Button::OK, tr("Set as regional marker") );
}


bool uiChooseRegMarkerDlg::acceptOK( CallBacker* )
{
    BufferStringSet chosenitms;
    list_->getChosen( chosenitms );
    if ( chosenitms.isEmpty() )
    {
	uiMSG().error( tr("Please select atleast one marker") );
	return false;
    }

    markernm_ = *chosenitms.first();
    return true;
}


uiRegionalMarkerMgr::RegMarker::RegMarker( const Strat::Level& lvl )
    : level_(lvl)
{}


uiRegionalMarkerMgr::RegMarker::~RegMarker()
{
    if ( !level_.isUndef() && !Strat::RegMarker::isRegMarker(level_) )
	delete &level_;
}


void uiRegionalMarkerMgr::RegMarker::operator=( const RegMarker& oth )
{
    const_cast<Strat::Level&>(level_) = oth.level_;
    eqmarkers_ = oth.eqmarkers_;
}


bool uiRegionalMarkerMgr::RegMarker::operator==( const RegMarker& oth ) const
{
    if ( oth.name() == name() && oth.levelID() == levelID() )
	return true;

    return false;
}


void uiRegionalMarkerMgr::RegMarker::setLevel( const Strat::Level& lvl )
{
    if ( lvl.isUndef() )
	return;

    const_cast<Strat::Level&>(level_) = lvl;
}


Strat::LevelID uiRegionalMarkerMgr::RegMarker::levelID() const
{
    return level_.id();
}


BufferString uiRegionalMarkerMgr::RegMarker::name() const
{
    return level_.name();
}


void uiRegionalMarkerMgr::RegMarker::addAsEquivalentMarker( const char* mrkrnm,
							    bool attop )
{
    if ( eqmarkers_.isPresent(mrkrnm) )
	return;

    if ( attop )
    {
	auto* name = new BufferString( mrkrnm );
	eqmarkers_.insertAt( name, 0 );
    }

    eqmarkers_.add( mrkrnm );
}


void uiRegionalMarkerMgr::RegMarker::addAsEquivalentMarkers(
					const BufferStringSet& markernms )
{
    eqmarkers_.add( markernms, false );
}


void uiRegionalMarkerMgr::RegMarker::setEquivalentMarkersEmpty()
{
    eqmarkers_.setEmpty();
}


void uiRegionalMarkerMgr::RegMarker::removeFromEquivalentMarker(
					const BufferStringSet& markernms )
{
    for ( const auto* name : markernms )
	eqmarkers_.remove( name->buf() );
}


void uiRegionalMarkerMgr::RegMarker::getEquivalentMarkerNames(
				BufferStringSet& names, bool incllvlnm ) const
{
    if ( incllvlnm )
	names.add( level_.name() );

    names.add( eqmarkers_, false );
}


void uiRegionalMarkerMgr::RegMarker::modifyPriority( int idx, bool increase )
{
    if ( eqmarkers_.size() <= 1 )
	return;

    if ( increase )
    {
	if ( idx > 0 )
	    eqmarkers_.swap( idx, idx-1 );

	return;
    }

    if ( idx < eqmarkers_.size()-1 )
	eqmarkers_.swap( idx, idx+1 );
}


int uiRegionalMarkerMgr::RegMarker::nrEquivalentMarkers() const
{
    return eqmarkers_.size();
}


bool uiRegionalMarkerMgr::RegMarker::isEquivalent( const char* markernm ) const
{
    return eqmarkers_.isPresent( markernm );
}


uiRegionalMarkerMgr::uiRegionalMarkerMgr( uiParent* p,
					  ObjectSet<Well::Data>& wds )
    : uiDialog(p,Setup(tr("Manage Regional Markers"),mTODOHelpKey))
    , wds_(wds)
{
    regmlist_ = new uiRegMarkerList( this );
    regmlist_->setNrLines( 25 );
    mAttachCB( regmlist_->selectionChanged,
	       uiRegionalMarkerMgr::regMarkerSelectedCB );
    mAttachCB( regmlist_->regMarkersRemoved,
	       uiRegionalMarkerMgr::allRegMarkersRemovedCB );

    auto* eqmarkersgrp = new uiGroup( this, "Equivalent markers group" );
    eqmarkersgrp->attach( rightOf, regmlist_ );
    uiListBox::Setup eqmsu( OD::ChooseZeroOrMore, tr("Equivalent markers"),
			    uiListBox::AboveMid );
    eqvmarkerlist_ = new uiListBox( eqmarkersgrp, eqmsu,
				    "Equivalent markers" );
    eqvmarkerlist_->setHSzPol( uiObject::Wide );
    eqvmarkerlist_->attach( heightSameAs, regmlist_ );
    eqmarkersgrp->setHAlignObj( eqvmarkerlist_->box() );
    eqmarkersgrp->setHCenterObj( eqvmarkerlist_->box() );
    mAttachCB( eqvmarkerlist_->itemChosen,
	       uiRegionalMarkerMgr::eqvMarkerChosenCB );
    auto* horgrp = new uiButtonGroup( eqmarkersgrp,
				      "Tools", OD::Horizontal );
    horgrp->attach( centeredBelow, eqvmarkerlist_ );
    eqaddnewbut_ = new uiToolButton( horgrp, "addnew", tr("Create new set") );
    mAttachCB( eqaddnewbut_->activated,
	       uiRegionalMarkerMgr::moveToNewEqMarkerSetCB );
    eqvtoregbut_ = new uiPushButton( horgrp,
				     tr("Set as Regional Marker"), false );
    mAttachCB( eqvtoregbut_->activated,
	       uiRegionalMarkerMgr::changeRegionalMarkerCB );
    eqvtoregbut_->attach( rightOf, eqaddnewbut_ );
    auto* vertgrp = new uiButtonGroup( eqmarkersgrp, "Tools", OD::Vertical );
    vertgrp->attach( centeredRightOf, eqvmarkerlist_ );
    new uiToolButton( vertgrp, "add", tr("Add to set"),
		      mCB(this,uiRegionalMarkerMgr,addAsEqMarkerCB) );
    new uiToolButton( vertgrp, "rightarrow", uiStrings::sRemove(),
		      mCB(this,uiRegionalMarkerMgr,removeFromEqMarkerCB) );
    new uiToolButton( vertgrp, "clear", tr("Remove all equivalent markers"),
		      mCB(this,uiRegionalMarkerMgr,
			  removeAllEquivalentMarkersCB) );
    incprioritybut_ = new uiToolButton( vertgrp, "uparrow",
					tr("Increase priority") );
    mAttachCB( incprioritybut_->activated,
	       uiRegionalMarkerMgr::modifyEqvMarkerPriority );
    decprioritybut_ = new uiToolButton( vertgrp, "downarrow",
					tr("Decrease priority") );
    mAttachCB( decprioritybut_->activated,
	       uiRegionalMarkerMgr::modifyEqvMarkerPriority );

    auto* amgrp = new uiGroup( this, "All markers group" );
    uiListBox::Setup amsu( OD::ChooseZeroOrMore, tr("Available markers"),
			   uiListBox::AboveMid );
    allmrkrlist_ = new uiListBox( amgrp, amsu, "All markers" );
    allmrkrlist_->setHSzPol( uiObject::Wide );
    amgrp->attach( rightOf, eqmarkersgrp );
    auto* allmrkrsbutgrp = new uiGroup( amgrp, "All markers button group" );
    allmrkrsaddnewbut_ = new uiToolButton( allmrkrsbutgrp, "addnew",
						tr("Create new set") );
    allmarkerstoregbut_ = new uiPushButton( allmrkrsbutgrp,
				    tr("Set as Regional Marker(s)"), false );
    allmrkrsbutgrp->attach( centeredBelow, allmrkrlist_ );
    allmarkerstoregbut_->attach( rightOf, allmrkrsaddnewbut_ );
    mAttachCB( allmrkrsaddnewbut_->activated,
	       uiRegionalMarkerMgr::createNewEqMarkerSetCB );
    mAttachCB( allmarkerstoregbut_->activated,
	       uiRegionalMarkerMgr::makeRegionalMarkerCB );

    mAttachCB( postFinalize(), uiRegionalMarkerMgr::initDlg );
}


uiRegionalMarkerMgr::~uiRegionalMarkerMgr()
{
    detachAllNotifiers();
}


void uiRegionalMarkerMgr::initDlg( CallBacker* )
{
    fillLists();
    updateCaption();
}


void uiRegionalMarkerMgr::fillLists()
{
    NotifyStopper sllns( regmlist_->selectionChanged );
    fillRegionalMarkersList();
    fillAllMarkersList();
    sllns.enableNotification();
    regmlist_->setCurrentItem( 0 );
    mAttachCB( Strat::eRGMLVLS().levelToBeRemoved,
	       uiRegionalMarkerMgr::regMarkerRemovedCB );
}


void uiRegionalMarkerMgr::fillRegionalMarkersList()
{
    regmarkers_.setEmpty();
    eqvmarkerlist_->setEmpty();
    BufferStringSet lvlnames;
    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	auto* regmrkr = new RegMarker( *new Strat::Level(lvls.getByIdx(idx)) );
	regmarkers_ += regmrkr;
	lvlnames.add( lvls.getByIdx(idx).name() );
    }

    int* sortedidxs = lvlnames.getSortIndexes();
    regmarkers_.useIndexes( sortedidxs );
}


void uiRegionalMarkerMgr::fillAllMarkersList()
{
    uiUserShowWait usw( this, tr("Fetching all markers ... ") );
    allmarkernames_.setEmpty();
    allmarkerlistnames_.setEmpty();
    allmrkrlist_->setEmpty();
    const Strat::RegMarkerSet& rgms = Strat::RGMLVLS();
    BufferStringSet regmnames;
    regmlist_->getItems( regmnames );
    for ( const auto* wd : wds_ )
    {
	const Well::MarkerSet& markers = wd->markers();
	for ( const auto* marker : markers )
	{
	    const BufferString name = marker->getDatabaseName();
	    if ( !allmarkernames_.addIfNew(name) )
		continue;

	    if ( regmnames.isPresent(name) )
		continue;

	    const Strat::LevelID lvlid = marker->levelID();
	    if ( lvlid.isUdf() || !lvlid.isValid() )
	    {
		allmarkerlistnames_.add( name.buf() );
		continue;
	    }

	    const bool levelattached = rgms.isPresent(lvlid)
				       || Strat::LVLS().isPresent(lvlid);
	    if ( !levelattached )
	    {
		allmarkerlistnames_.add( name.buf() );
		continue;
	    }

	    const bool isalevel =  name == rgms.get(lvlid).name()
				  || name == Strat::LVLS().get(lvlid).name();
	    if ( isalevel )
		continue;
	    else
	    {
		for ( auto* regmarker : regmarkers_ )
		{
		    if ( lvlid!=regmarker->levelID()
			 || regmarker->isEquivalent(name) )
			continue;

		    regmarker->addAsEquivalentMarker( name );
		    break;
		}
	    }
	}
    }

    allmarkerlistnames_.sort();
    allmrkrlist_->addItems( allmarkerlistnames_ );
}


void uiRegionalMarkerMgr::allRegMarkersRemovedCB( CallBacker* cb )
{
    BufferStringSet recoveredmarkers;
    for ( int idx=regmarkers_.size()-1; idx>=0; idx-- )
    {
	const RegMarker& rgm = *regmarkers_.get( idx );
	const Strat::LevelID rgmid = rgm.levelID();
	if ( rgmid.isValid() && !Strat::RegMarker::isRegMarker(rgmid) )
	    continue;

	rgm.getEquivalentMarkerNames( recoveredmarkers, true );
	regmarkers_.removeSingle( idx );
    }

    if ( recoveredmarkers.isEmpty() )
	return;

    allmrkrlist_->addItems( recoveredmarkers );
    allmrkrlist_->sortItems();
    regmlist_->setCurrentItem( 0 );
}


void uiRegionalMarkerMgr::regMarkerRemovedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( Strat::LevelID, regmid, cb );
    if ( regmid.isUdf() )
	return;

    BufferStringSet recoveredmarkers;
    for ( int idx=regmarkers_.size()-1; idx>=0; idx-- )
    {
	const RegMarker& rgm = *regmarkers_.get( idx );
	if ( rgm.levelID() != regmid )
	    continue;

	rgm.getEquivalentMarkerNames( recoveredmarkers, false );
	if ( allmarkernames_.isPresent(rgm.name()) )
	    recoveredmarkers.add( rgm.name() );

	allmarkerlistnames_.add( recoveredmarkers, false );
	regmarkers_.removeSingle( idx );
	break;
    }

    if ( recoveredmarkers.isEmpty() )
	return;

    allmrkrlist_->addItems( recoveredmarkers );
    allmrkrlist_->sortItems();
    regmlist_->setCurrentItem( 0 );
}


void uiRegionalMarkerMgr::eqvMarkerChosenCB( CallBacker* )
{
    const int nrchosen = eqvmarkerlist_->nrChosen();

    const bool canchgpriority = nrchosen == 0;
    incprioritybut_->setSensitive( canchgpriority );
    decprioritybut_->setSensitive( canchgpriority );

    const bool setregmallowed = nrchosen <= 1;
    eqvtoregbut_->setSensitive( setregmallowed );
}


void uiRegionalMarkerMgr::moveToNewEqMarkerSetCB( CallBacker* )
{
    BufferStringSet chosenmrkrs;
    eqvmarkerlist_->getChosen( chosenmrkrs );
    if ( chosenmrkrs.size()<2 )
    {
	uiMSG().message( tr("Please choose atleast two markers.") );
	return;
    }

    const BufferString currregmarker = regmlist_->getText();
    bool goon = uiMSG().askGoOn( tr("The selected markers will no longer use "
		    "\"%1\" as their Regional marker. You must assign a new "
		    "Regional marker from the current selection.\n\nDo you "
		    "want to continue?").arg(currregmarker.buf()) );
    if ( !goon )
	return;

    uiChooseRegMarkerDlg dlg( this, chosenmrkrs );
    if ( !dlg.go() )
	return;

    const BufferString regmarkernm = dlg.chosenMarkerName();
    const BufferStringSet newregmrkr( regmarkernm.buf() );
    setAsRegionalMarker( newregmrkr );
    for ( auto* marker : regmarkers_ )
    {
	if ( marker->name() == currregmarker )
	    marker->removeFromEquivalentMarker( chosenmrkrs );
    }

    chosenmrkrs.remove( regmarkernm );
    addToEquivalentMarkerSet( regmarkernm, chosenmrkrs );
}


void uiRegionalMarkerMgr::createNewEqMarkerSetCB( CallBacker* )
{
    BufferStringSet chosenmrkrs;
    allmrkrlist_->getChosen( chosenmrkrs );
    if ( chosenmrkrs.isEmpty() )
    {
	uiMSG().message( tr("Please choose atleast one marker.") );
	return;
    }

    if ( chosenmrkrs.size()==1 )
    {
	setAsRegionalMarker( chosenmrkrs );
	return;
    }

    uiChooseRegMarkerDlg dlg( this, chosenmrkrs );
    if ( !dlg.go() )
	return;

    const BufferString regmarkernm = dlg.chosenMarkerName();
    const BufferStringSet newregmrkr( regmarkernm.buf() );
    setAsRegionalMarker( newregmrkr );
    chosenmrkrs.remove( regmarkernm );
    addToEquivalentMarkerSet( regmarkernm, chosenmrkrs );
}


void uiRegionalMarkerMgr::addAsEqMarkerCB( CallBacker* )
{
    BufferStringSet chosenmrkrs;
    allmrkrlist_->getChosen( chosenmrkrs );
    if ( chosenmrkrs.isEmpty() )
    {
	uiMSG().message( tr("Please choose atleast one marker.") );
	return;
    }

    const BufferString currregmarker = regmlist_->getText();
    addToEquivalentMarkerSet( currregmarker, chosenmrkrs );
}


void uiRegionalMarkerMgr::addToEquivalentMarkerSet( const char* regmrkr,
					const BufferStringSet& markernms )
{
    if ( StringView(regmrkr).isEmpty() || markernms.isEmpty() )
	return;

    for ( auto* regmarker : regmarkers_ )
    {
	if ( regmarker->name()!=StringView(regmrkr) )
	    continue;

	regmarker->addAsEquivalentMarkers( markernms );
	BufferStringSet curreqvitems;
	regmarker->getEquivalentMarkerNames( curreqvitems );
	eqvmarkerlist_->setEmpty();
	eqvmarkerlist_->addItems( curreqvitems );
	updatePriorityButtonState( regmarker->nrEquivalentMarkers()>1 );
	break;
    }

    for ( const auto* marker : markernms )
    {
	allmrkrlist_->removeItem( marker->buf() );
	allmarkerlistnames_.remove( marker->buf() );
    }
}


void uiRegionalMarkerMgr::removeFromEqMarkerCB( CallBacker* )
{
    BufferStringSet chosenmrkrs;
    eqvmarkerlist_->getChosen( chosenmrkrs );
    if ( chosenmrkrs.isEmpty() )
    {
	uiMSG().message( tr("Please choose atleast one marker.") );
	return;
    }

    const BufferString currregmarker = regmlist_->getText();
    for ( auto* regmarker : regmarkers_ )
    {
	if ( regmarker->name()!=currregmarker )
	    continue;

	if ( chosenmrkrs.isPresent(currregmarker) )
	{
	    pErrMsg( "This item should not be choosable" );
	    uiMSG().error( tr("One of the markers selected (%1) is a Regional "
			      "marker. Please remove it from the \"Regional "
			      "marker\" list first, to proceed. \n"
			      "Alternatively, please deselect %1 and retry.")
			      .arg(currregmarker.buf()) );
	    return;
	}

	regmarker->removeFromEquivalentMarker( chosenmrkrs );
	for ( const auto* name : chosenmrkrs )
	    eqvmarkerlist_->removeItem( name->buf() );

	updatePriorityButtonState( regmarker->nrEquivalentMarkers()>1 );
	break;
    }

    allmarkerlistnames_.add( chosenmrkrs, false );
    allmrkrlist_->addItems( chosenmrkrs );
    allmarkerlistnames_.sort();
    allmrkrlist_->sortItems();
}


void uiRegionalMarkerMgr::removeAllEquivalentMarkersCB( CallBacker* )
{
    const BufferString currregmarker = regmlist_->getText();
    bool goon = uiMSG().askGoOn( tr("Proceeding will remove the Regional "
		    "marker( %1 ) assignments for all the markers in the "
		    "Equivalent markers list and return the affected markers"
		    " to all markers list.\n\nDo you want to continue?")
		    .arg(currregmarker.buf()) );
    if ( !goon )
	return;

    BufferStringSet alleqmarkers;
    for ( auto* regmarker : regmarkers_ )
    {
	if ( regmarker->name()!=currregmarker )
	    continue;

	regmarker->getEquivalentMarkerNames( alleqmarkers, false );
	regmarker->setEquivalentMarkersEmpty();
	updatePriorityButtonState( false );
	break;
    }

    allmarkerlistnames_.add( alleqmarkers, false );
    allmrkrlist_->addItems( alleqmarkers );
    allmarkerlistnames_.sort();
    allmrkrlist_->sortItems();
    const bool regmarkerisamarker
			= eqvmarkerlist_->isPresent( currregmarker.buf() );
    eqvmarkerlist_->setEmpty();
    if ( regmarkerisamarker )
    {
	eqvmarkerlist_->addItem( currregmarker.buf() );
	eqvmarkerlist_->setItemSelectable( 0, false );
    }
}


void uiRegionalMarkerMgr::modifyEqvMarkerPriority( CallBacker* cb )
{
    mDynamicCastGet( const uiToolButton*, but, cb );
    if ( !but )
	return;

    const bool increase = but == incprioritybut_;
    int curridx = eqvmarkerlist_->currentItem();
    const BufferString regmname = regmlist_->getText();
    if ( allmarkernames_.isPresent(regmname) )
	curridx--;

    for ( auto* regm : regmarkers_ )
    {
	if ( regm->name() == regmname )
	{
	    regm->modifyPriority( curridx, increase );
	    regMarkerSelectedCB( nullptr );
	    return;
	}
    }
}


void uiRegionalMarkerMgr::changeRegionalMarkerCB( CallBacker* )
{
    BufferStringSet chosenmarker;
    eqvmarkerlist_->getChosen( chosenmarker );
    if ( chosenmarker.isEmpty() )
    {
	uiMSG().error( tr("Please select a marker to be assigned as Regional "
			  "Marker for this set") );
	return;
    }

    const BufferString currregmarker = regmlist_->getText();
    const BufferString& newregmarker = *chosenmarker.first();
    uiString txt( tr("The selected marker is currently part of a group of "
		     "equivalent markers associated with an existing "
		     "Regional marker (%1).").arg(currregmarker.buf()) );
    txt.addNewLine().append(tr("Replacing current Regional Marker will remove "
		     "\"%1\" from the list Regional Markers and \"%2\""
		     "will be added in its place. \"%1\" will continue to be "
		     "an equivelent marker.").arg(currregmarker.buf())
		     .arg(newregmarker.buf()));
    const int ans = uiMSG().question( txt, tr("Replace regional marker"),
				      tr("Create new Regional marker") );

    if ( ans==1 )
	replaceRegionalMarker( newregmarker );
    else if ( ans==0 )
	setEqvMarkerAsRegional( newregmarker );
}


void uiRegionalMarkerMgr::replaceRegionalMarker( const BufferString& newmrkr )
{
    if ( newmrkr.isEmpty() )
	return;

    const BufferString currregmarker = regmlist_->getText();
    const Strat::LevelID currlvlid
			= Strat::LVLS().getByName(currregmarker.buf()).id();
    BufferStringSet eqmarkers;
    eqvmarkerlist_->getItems( eqmarkers );
    Strat::RegMarkerSet& ergms = Strat::eRGMLVLS();
    Strat::RegMarker& lvl = ergms.getByName( currregmarker );
    lvl.setName( newmrkr );
    OD::Color newcolor;
    for ( const auto* wd : wds_ )
    {
	if ( wd->markers().isPresent(newmrkr,true) )
	{
	    newcolor = wd->markers().getByName(newmrkr,true)->color();
	    break;
	}
    }

    lvl.setColor( newcolor );
    const Strat::LevelID newid = ergms.set( lvl );
    for ( auto* marker : regmarkers_ )
    {
	if ( marker->levelID() != currlvlid )
	    continue;

	if ( !Strat::RegMarker::isRegMarker(currlvlid) )
	{
	    marker->setEquivalentMarkersEmpty();
	    auto* newregmrkr = new RegMarker( Strat::RGMLVLS().get(newid) );
	    eqmarkers.remove( newmrkr.buf() );
	    newregmrkr->addAsEquivalentMarkers( eqmarkers );
	    regmarkers_ += newregmrkr;
	    break;
	}

	RegMarker newregmarker( Strat::RGMLVLS().get(newid) );
	eqmarkers.remove( newmrkr.buf() );
	if ( !eqmarkers.isPresent(currregmarker) )
	    eqmarkers.add( currregmarker );

	newregmarker.addAsEquivalentMarkers( eqmarkers );
	*marker = newregmarker;
	break;
    }

    BufferStringSet lvlnms;
    for ( const auto* regm : regmarkers_ )
	lvlnms.add( regm->name() );

    regmarkers_.useIndexes( lvlnms.getSortIndexes() );
    regmlist_->setCurrentItem( newmrkr.buf() );
}


void uiRegionalMarkerMgr::setEqvMarkerAsRegional( const BufferString& newmrkr )
{
    if ( newmrkr.isEmpty() )
	return;

    const BufferString currmarker = regmlist_->getText();
    for ( auto* marker : regmarkers_ )
    {
	if ( marker->name() != currmarker )
	    continue;

	marker->removeFromEquivalentMarker( BufferStringSet(newmrkr.buf()) );
	break;
    }

    OD::Color markercolor;
    for ( const auto* wd : wds_ )
    {
	if ( wd->markers().isPresent(newmrkr) )
	{
	    markercolor = wd->markers().getByName(newmrkr,true)->color();
	    break;
	}
    }

    Strat::eRGMLVLS().add( newmrkr, markercolor );
    auto* regmarker = new RegMarker(
			    Strat::RGMLVLS().getByName(newmrkr.buf()) );
    regmarkers_ += regmarker;

    BufferStringSet lvlnms;
    for ( const auto* regm : regmarkers_ )
	lvlnms.add( regm->name() );

    regmarkers_.useIndexes( lvlnms.getSortIndexes() );
    regmlist_->setCurrentItem( newmrkr.buf() );
}


void uiRegionalMarkerMgr::makeRegionalMarkerCB( CallBacker* )
{
    BufferStringSet chosenmrkrs;
    allmrkrlist_->getChosen( chosenmrkrs );
    if ( chosenmrkrs.isEmpty() )
    {
	uiMSG().message( tr("Please choose atleast one marker.") );
	return;
    }

    setAsRegionalMarker( chosenmrkrs );
}


void uiRegionalMarkerMgr::setAsRegionalMarker( const BufferStringSet& mrkrs )
{
    Strat::RegMarkerSet& ergms = Strat::eRGMLVLS();
    for ( const auto* markername : mrkrs )
    {
	OD::Color markercolor;
	for ( const auto* wd : wds_ )
	{
	    if ( wd->markers().isPresent(*markername,true) )
	    {
		const Well::Marker* mrkr
				= wd->markers().getByName(*markername,true);
		if ( !mrkr )
		    continue;

		markercolor = wd->markers().getByName(*markername,
						      true)->color();
		break;
	    }
	}

	ergms.add( *markername, markercolor );
	allmrkrlist_->removeItem( markername->buf() );
	const int idx = allmarkerlistnames_.indexOf( *markername );
	if ( !allmarkerlistnames_.validIdx(idx) )
	{
	    pErrMsg( "This should not be the case. Please debug." );
	    continue;
	}

	allmarkerlistnames_.removeSingle( idx );
    }

    BufferStringSet regmnames;
    for ( const auto* regm : regmarkers_ )
	regmnames.add( regm->name() );

    for ( const auto* marker : mrkrs )
    {
	if ( regmnames.isPresent(*marker) )
	    continue;

	auto* regmarker = new RegMarker( ergms.getByName(marker->buf()) );
	regmarkers_ += regmarker;
    }

    BufferStringSet lvlnms;
    for ( const auto* regm : regmarkers_ )
	lvlnms.add( regm->name() );

    regmarkers_.useIndexes( lvlnms.getSortIndexes() );
    regmlist_->setCurrentItem( mrkrs.last()->buf() );
}


void uiRegionalMarkerMgr::regMarkerSelectedCB( CallBacker* )
{
    BufferStringSet eqmarkernames;
    const int idx = regmlist_->currentItem();
    if ( !regmlist_->validIdx(idx) || !regmarkers_.validIdx(idx) )
    {
	eqvmarkerlist_->setEmpty();
	return;
    }

    const RegMarker* currregmarker = regmarkers_.get( idx );
    const bool regmarkernameincluded
		    = allmarkernames_.isPresent( currregmarker->name() );
    currregmarker->getEquivalentMarkerNames( eqmarkernames,
					     regmarkernameincluded );
    updatePriorityButtonState( currregmarker->nrEquivalentMarkers()>1 );
    eqvmarkerlist_->setEmpty();
    if ( eqmarkernames.isEmpty() )
	return;

    eqvmarkerlist_->setLabelText( tr("Equivalent markers (%1)")
				  .arg(currregmarker->name().buf()) );
    eqvmarkerlist_->addItems( eqmarkernames );
    if ( regmarkernameincluded )
	eqvmarkerlist_->setItemSelectable( 0, false );
}


void uiRegionalMarkerMgr::updatePriorityButtonState( bool makesensitive )
{
    incprioritybut_->setSensitive( makesensitive );
    decprioritybut_->setSensitive( makesensitive );
}


void uiRegionalMarkerMgr::resetRegMarkers()
{
    Strat::RegMarkerSet& rgmset = Strat::eRGMLVLS();
    NotifyStopper addns( rgmset.levelAdded, this );
    NotifyStopper remns( rgmset.levelToBeRemoved, this );
    rgmset.reset();
}


bool uiRegionalMarkerMgr::rejectOK( CallBacker* )
{
    regmarkers_.setEmpty();
    resetRegMarkers();
    return true;
}


bool uiRegionalMarkerMgr::isInputOK( uiString& errmsg ) const
{
    if ( allmarkerlistnames_.size() != allmrkrlist_->size() )
    {
	pErrMsg( "allmarkerlistnames_ and allmrkrlist_ should have same size" );
	errmsg = tr( "Something went wrong." );
	return false;
    }

    BufferStringSet itemsinallmrkrlist;
    allmrkrlist_->getItems( itemsinallmrkrlist );
    if ( allmarkerlistnames_ != itemsinallmrkrlist )
    {
	pErrMsg( "allmarkerlistnames_ and allmrkrlist_ should have same "
		 "elements" );
	errmsg = tr( "Something went wrong." );
	return false;
    }

    std::unordered_set<std::string> noregmmarkers;
    std::unordered_set<std::string> regmarkers;
    std::unordered_set<std::string> eqvregmarkers;
    for ( const auto* name : allmarkerlistnames_ )
    {
	if ( !noregmmarkers.insert( name->buf() ).second )
	{
	    pErrMsg( "allmarkerlistnames_ has duplicate marker names" );
	    errmsg = tr( "Something went wrong." );
	    return false;
	}
    }

    for ( const auto* regm : regmarkers_ )
    {
	const std::string name = regm->name().buf();
	if ( allmarkernames_.isPresent(name.c_str()) )
	{
	    if ( noregmmarkers.find(name) != noregmmarkers.end() )
	    {
		pErrMsg( "allmarkerlistnames_ has marker names that are "
			 "assigned as regional marker" );
		errmsg = tr( "Something went wrong." );
		return false;
	    }

	    if ( !regmarkers.insert(name).second )
	    {
		pErrMsg( "regmarkers_ has duplicate marker names" );
		errmsg = tr( "Something went wrong." );
		return false;
	    }
	}

	BufferStringSet eqvmarkers;
	regm->getEquivalentMarkerNames( eqvmarkers, false );
	if ( eqvmarkers.isEmpty() )
	    continue;

	for ( const auto* eqvname : eqvmarkers )
	{
	    if ( noregmmarkers.find(eqvname->buf()) != noregmmarkers.end() )
	    {
		pErrMsg( "allmarkerlistnames_ has marker names that are "
			 "assigned as equivalent marker" );
		errmsg = tr( "Something went wrong." );
		return false;
	    }

	    if ( regmarkers.find(eqvname->buf()) != regmarkers.end() )
	    {
		pErrMsg( "An regional marker has also been assigned as "
			 "equivalent marker" );
		errmsg = tr( "Something went wrong." );
		return false;
	    }

	    if ( !eqvregmarkers.insert(eqvname->buf()).second )
	    {
		pErrMsg( "Equivalent markers has duplicate marker names" );
		errmsg = tr( "Something went wrong." );
		return false;
	    }
	}
    }

    const int totalsetsz = noregmmarkers.size() + regmarkers.size()
						+ eqvregmarkers.size();
    if ( totalsetsz != allmarkernames_.size() )
    {
	pErrMsg( "Count of all list items should be equal to allmarkernames_ "
		 "size" );
	errmsg = tr( "Something went wrong." );
	return false;
    }

    return true;
}


void uiRegionalMarkerMgr::saveWells( const std::unordered_set<int>& widxs )
{
    if ( widxs.empty() )
	return;

    RefObjectSet<Well::Data> editedwds;
    TypeSet<Well::LoadReqs> storereqs;
    for ( const auto& idx : widxs )
    {
	editedwds += wds_.get( idx );
	storereqs += Well::Mrkrs;
    }

    uiTaskRunner uitr( this );
    MultiWellWriter mwr( editedwds, storereqs );
    const bool res = uitr.execute( mwr );
    if ( res && !mwr.allWellsWritten() )
	uiMSG().warning( tr( "Some wells could not be saved" ) );
}


bool uiRegionalMarkerMgr::acceptOK( CallBacker* )
{
    if ( regmarkers_.isEmpty() && removedrgms_.isEmpty() )
	return true;

    uiString errmsg;
    if ( !isInputOK(errmsg) )
    {
	uiMSG().error( errmsg );
	return true;
    }

    const bool levelsremoved = !removedrgms_.isEmpty();
    const bool allrgmsremoved = regmarkers_.isEmpty() && levelsremoved;
    std::unordered_set<int> chgdwellidxs;
    if ( allrgmsremoved )
    {
	int idx = 0;
	for ( auto* wd : wds_ )
	{
	    for ( auto* marker : wd->markers() )
		marker->setNoLevelID();

	    chgdwellidxs.insert( idx );
	    idx++;
	}

	if ( !Strat::RGMLVLS().save() )
	{
	    resetRegMarkers();
	    return false;
	}

	saveWells( chgdwellidxs );
	return true;
    }

    for ( const auto* regm : regmarkers_ )
    {
	BufferStringSet eqvmarkers;
	regm->getEquivalentMarkerNames( eqvmarkers, false );
	const BufferString regmname = regm->name();
	if ( !allmarkernames_.isPresent(regmname) && eqvmarkers.isEmpty() )
	    continue;

	const Strat::LevelID newlvlid = regm->levelID();
	for ( int idx=0; idx<wds_.size(); idx++ )
	{
	    RefMan<Well::Data> wd = wds_[idx];
	    if ( !wd )
		continue;

	    Well::MarkerSet& markers = wd->markers();
	    if ( markers.isEmpty() )
		continue;

	    if ( levelsremoved )
	    {
		for ( auto* marker : markers )
		{
		    if ( removedrgms_.isPresent(marker->levelID()) )
		    {
			marker->setNoLevelID();
			chgdwellidxs.insert( idx );
		    }
		}
	    }

	    if ( markers.isPresent(regmname,true) )
	    {
		Well::Marker* marker = markers.getByName( regmname, true );
		if ( !marker )
		{
		    pErrMsg( "Probably shouldn't have returned true "
			     "for isPresent" );
		    continue;
		}

		if ( marker->levelID() == newlvlid )
		    continue;

		marker->setLevelID( newlvlid );
		chgdwellidxs.insert( idx );
		continue;
	    }

	    for ( const auto* eqvm : eqvmarkers )
	    {
		if ( !markers.isPresent(*eqvm,true) )
		    continue;

		Well::Marker* marker = markers.getByName( eqvm->buf(), true );
		if ( !marker )
		{
		    pErrMsg( "Probably shouldn't have returned true "
			     "for isPresent" );
		    continue;
		}

		if ( marker->levelID() == newlvlid )
		    break;

		marker->setLevelID( newlvlid );
		chgdwellidxs.insert( idx );
		break;
	    }
	}
    }

    if ( !Strat::RGMLVLS().save() )
    {
	resetRegMarkers();
	return false;
    }

    saveWells( chgdwellidxs );
    return true;
}
