/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

#include "uihorizontracksetup.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "autotracker.h"
#include "draw.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "horizonadjuster.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "randcolor.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimpecorrelationgrp.h"
#include "uimpeeventgrp.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uitabstack.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

void uiBaseHorizonSetupGroup::initClass()
{
    uiMPE().setupgrpfact.addFactory( uiBaseHorizonSetupGroup::create,
				     Horizon2DTracker::keyword() );
    uiMPE().setupgrpfact.addFactory( uiBaseHorizonSetupGroup::create,
				     Horizon3DTracker::keyword() );
}


uiSetupGroup* uiBaseHorizonSetupGroup::create( uiParent* p, const char* typestr)
{
    const FixedString type( typestr );
    if ( type != EM::Horizon3D::typeStr() && type != EM::Horizon2D::typeStr() )
	return 0;

    return new uiBaseHorizonSetupGroup( p, typestr );
}


uiBaseHorizonSetupGroup::uiBaseHorizonSetupGroup( uiParent* p,
						  const char* typestr )
    : uiHorizonSetupGroup( p, typestr )
{}


uiHorizonSetupGroup::uiHorizonSetupGroup( uiParent* p, const char* typestr )
    : uiSetupGroup(p,"")
    , trackmgr_(0)
    , sectiontracker_(0)
    , horadj_(0)
    , is2d_(FixedString(typestr)==EM::Horizon2D::typeStr())
    , modeChanged_(this)
    , varianceChanged_(this)
    , propertyChanged_(this)
    , state_(Stopped)
{
    IOObjContext ctxt =
	is2d_ ? mIOObjContext(EMHorizon2D) : mIOObjContext(EMHorizon3D);
    ctxt.forread = false;
    horizonfld_ = new uiIOObjSel( this, ctxt );
    horizonfld_->selectionDone.notify(
		mCB(this,uiHorizonSetupGroup,horizonSelCB) );

    tabgrp_ = new uiTabStack( this, "TabStack" );
    tabgrp_->attach( leftAlignedBelow, horizonfld_ );
    uiGroup* modegrp = createModeGroup();
    tabgrp_->addTab( modegrp, tr("Mode") );

    eventgrp_ = new uiEventGroup( tabgrp_->tabGroup(), is2d_ );
    tabgrp_->addTab( eventgrp_, tr("Event") );

    correlationgrp_ = new uiCorrelationGroup( tabgrp_->tabGroup(), is2d_ );
    tabgrp_->addTab( correlationgrp_, tr("Correlation") );

//    uiGroup* vargrp = createVarianceGroup();
//    tabgrp_->addTab( vargrp, tr("Variance") );

    uiGroup* propertiesgrp = createPropertyGroup();
    tabgrp_->addTab( propertiesgrp, uiStrings::sProperties(true) );

    mDynamicCastGet(uiDialog*,dlg,p)
    toolbar_ = new uiToolBar( dlg, "Tracking tools", uiToolBar::Left );
    initToolBar();
}


void uiHorizonSetupGroup::initToolBar()
{
    startbutid_ = toolbar_->addButton( "autotrack", "Start Tracking [T]",
				mCB(this,uiHorizonSetupGroup,startCB) );
    toolbar_->setShortcut( startbutid_, "t" );

    stopbutid_ = toolbar_->addButton( "stop", "Stop Tracking [S]",
				mCB(this,uiHorizonSetupGroup,stopCB) );
    toolbar_->setShortcut( stopbutid_, "s" );

    savebutid_ = toolbar_->addButton( "save", "Save Horizon [Ctrl+S]",
				mCB(this,uiHorizonSetupGroup,saveCB) );
    toolbar_->setShortcut( savebutid_, "ctrl+s" );

    retrackbutid_ = toolbar_->addButton( "retrackhorizon", "Retrack All",
				mCB(this,uiHorizonSetupGroup,retrackCB) );
}


void uiHorizonSetupGroup::updateButtonSensitivity()
{
    const bool enable = state_ == Stopped;
    toolbar_->setSensitive( startbutid_, enable );
    toolbar_->setSensitive( savebutid_, enable );
    toolbar_->setSensitive( retrackbutid_, enable );
}


void uiHorizonSetupGroup::horizonSelCB( CallBacker* )
{
    const IOObj* ioobj = horizonfld_->ioobj( true );
    if ( !sectiontracker_ || !ioobj )
	return;

    EM::EMObject& emobj = sectiontracker_->emObject();
    emobj.setMultiID( ioobj->key() );
}


void uiHorizonSetupGroup::startCB( CallBacker* )
{
    if ( state_ == Started )
	return;

    if ( trackInVolume() )
    {
	state_ = Started;
	updateButtonSensitivity();
    }
}


void uiHorizonSetupGroup::stopCB( CallBacker* )
{
    engine().stopTracking();
    state_ = Stopped;
    updateButtonSensitivity();
}


void uiHorizonSetupGroup::saveCB( CallBacker* )
{
    if ( !sectiontracker_ ) return;

    EM::EMObject& emobj = sectiontracker_->emObject();
    if ( emobj.multiID().isUdf() )
    {
	horizonfld_->doSel(0);
	emobj.setMultiID( horizonfld_->key() );
    }

    PtrMan<Executor> exec = emobj.saver();
    if ( exec ) exec->execute();
    mDynamicCastGet(EM::Horizon3D*,hor3d,&emobj)
    if ( hor3d )
    {
	ExecutorGroup execgrp( "Saving AuxData" );
	execgrp.add( hor3d->auxdata.auxDataSaver(0,true) );
	execgrp.add( hor3d->auxdata.auxDataSaver(1,true) );
	execgrp.execute();
    }
}


void uiHorizonSetupGroup::retrackCB( CallBacker* )
{
    if ( !sectiontracker_ || !commitToTracker() )
	return;

    EM::EMObject& emobj = sectiontracker_->emObject();
    emobj.setBurstAlert( true );
    emobj.removeAllUnSeedPos();
    const int trackeridx = MPE::engine().getTrackerByObject( emobj.id() );
    EMTracker* emtracker = engine().getTracker( trackeridx );
    EMSeedPicker* seedpicker = emtracker ? emtracker->getSeedPicker(false) : 0;
    if ( !seedpicker ) return;

    seedpicker->reTrack();
    if ( trackInVolume() )
    {
	state_ = Started;
	updateButtonSensitivity();
    }

    emobj.setBurstAlert( false );
}


bool uiHorizonSetupGroup::trackInVolume()
{
    if ( !sectiontracker_ || mode_!=EMSeedPicker::TrackFromSeeds )
	return false;

    EM::EMObject& emobj = sectiontracker_->emObject();
    const int trackeridx = MPE::engine().getTrackerByObject( emobj.id() );
    EMTracker* emtracker = engine().getTracker( trackeridx );
    EMSeedPicker* seedpicker = emtracker ? emtracker->getSeedPicker(false) : 0;
    const Attrib::SelSpec* as = seedpicker ? seedpicker->getSelSpec() : 0;
    if ( !as ) return false;

    if ( !as->isStored() )
    {
	uiMSG().error( "Volume tracking can only be done on stored volumes.");
	return false;
    }

    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet( false, true );
    const MultiID mid = ads ? ads->getStoredKey(as->id()) : MultiID::udf();
    if ( mid.isUdf() )
    {
	uiMSG().error( "Cannot find picked data in database" );
	return false;
    }

    mDynamicCastGet(RegularSeisDataPack*,sdp,Seis::PLDM().get(mid));
    if ( !sdp )
    {
	uiMSG().error( "Seismic data is not preloaded yet" );
	return false;
    }

    engine().setAttribData( *as, sdp->id() );
    engine().setActiveVolume( sdp->sampling() );
    if ( trackmgr_ )
    {
	trackmgr_->finished.remove(
		mCB(this,uiHorizonSetupGroup,trackingFinishedCB) );
    }

    trackmgr_ = engine().trackInVolume( trackeridx );
    if ( !trackmgr_ ) return false;

    trackmgr_->finished.notify(
		mCB(this,uiHorizonSetupGroup,trackingFinishedCB) );
    return true;
}


void uiHorizonSetupGroup::trackingFinishedCB( CallBacker* )
{
    state_ = Stopped;
    updateButtonSensitivity();
}


uiGroup* uiHorizonSetupGroup::createModeGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Mode" );

    modeselgrp_ = new uiButtonGroup( grp, "ModeSel", OD::Vertical );
    modeselgrp_->setExclusive( true );
    grp->setHAlignObj( modeselgrp_ );

    if ( !is2d_ && Horizon3DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon3DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
			Horizon3DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify(
			mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon3DSeedPicker::defaultSeedConMode();
	}
    }
    else if ( is2d_ && Horizon2DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon2DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
			Horizon2DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify(
			mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon2DSeedPicker::defaultSeedConMode();
	}
    }

    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, modeselgrp_ );
    BufferStringSet strs; strs.add( "Seed Trace" ).add( "Adjacent Parent" );
    methodfld_ = new uiGenInput( grp, tr("Method"), StringListInpSpec(strs) );
    methodfld_->attach( alignedBelow, modeselgrp_ );
    methodfld_->attach( ensureBelow, sep );

    return grp;
}


uiGroup* uiHorizonSetupGroup::createVarianceGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Variance" );

    usevarfld_ = new uiGenInput( grp, tr("Use Variance"), BoolInpSpec(false) );
    usevarfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selUseVariance) );
    usevarfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,varianceChangeCB) );

    const IOObjContext ctxt =
	uiSeisSel::ioContext( is2d_ ? Seis::Line : Seis::Vol, true );
    uiSeisSel::Setup ss( is2d_, false );
    variancefld_ = new uiSeisSel( grp, ctxt, ss );
    variancefld_->attach( alignedBelow, usevarfld_ );

    varthresholdfld_ =
	new uiGenInput( grp, tr("Variance threshold"), FloatInpSpec() );
    varthresholdfld_->attach( alignedBelow, variancefld_ );
    varthresholdfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,varianceChangeCB) );

    grp->setHAlignObj( usevarfld_ );
    return grp;
}


uiGroup* uiHorizonSetupGroup::createPropertyGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Properties" );
    colorfld_ = new uiColorInput( grp,
				uiColorInput::Setup(getRandStdDrawColor() )
				.withdesc(false).lbltxt(tr("Horizon Color")) );
    colorfld_->colorChanged.notify(
			mCB(this,uiHorizonSetupGroup,colorChangeCB) );
    grp->setHAlignObj( colorfld_ );

    linewidthfld_ = new uiSlider( grp, uiSlider::Setup(tr("Line Width"))
					.withedit(true),
				  "Line Width" );
    linewidthfld_->setInterval( 1, 15, 1 );
    linewidthfld_->valueChanged.notify(
			mCB(this,uiHorizonSetupGroup,colorChangeCB) );
    linewidthfld_->attach( alignedBelow, colorfld_ );

    seedtypefld_ = new uiGenInput( grp, tr("Seed Shape/Color"),
			StringListInpSpec(MarkerStyle3D::TypeNames()) );
    seedtypefld_->valuechanged.notify(
			mCB(this,uiHorizonSetupGroup,seedTypeSel) );
    seedtypefld_->attach( alignedBelow, linewidthfld_ );

    seedcolselfld_ = new uiColorInput( grp,
				uiColorInput::Setup(Color::White())
				.withdesc(false) );
    seedcolselfld_->attach( rightTo, seedtypefld_ );
    seedcolselfld_->colorChanged.notify(
				mCB(this,uiHorizonSetupGroup,seedColSel) );

    seedsliderfld_ = new uiSlider( grp,
				uiSlider::Setup(tr("Seed Size")).
				withedit(true),	"Seed Size" );
    seedsliderfld_->setInterval( 1, 15 );
    seedsliderfld_->valueChanged.notify(
			mCB(this,uiHorizonSetupGroup,seedSliderMove) );
    seedsliderfld_->attach( alignedBelow, seedtypefld_ );

    return grp;
}


uiHorizonSetupGroup::~uiHorizonSetupGroup()
{
}


NotifierAccess* uiHorizonSetupGroup::eventChangeNotifier()
{ return eventgrp_->changeNotifier(); }


NotifierAccess*	uiHorizonSetupGroup::correlationChangeNotifier()
{ return correlationgrp_->changeNotifier(); }


void uiHorizonSetupGroup::selUseVariance( CallBacker* )
{
    const bool usevar = usevarfld_->getBoolValue();
    variancefld_->setSensitive( usevar );
    varthresholdfld_->setSensitive( usevar );
}


void uiHorizonSetupGroup::seedModeChange( CallBacker* )
{
    mode_ = (EMSeedPicker::SeedModeOrder) modeselgrp_->selectedId();
    modeChanged_.trigger();

    const bool usedata = mode_ != EMSeedPicker::DrawBetweenSeeds;
    const bool invol = mode_ == EMSeedPicker::TrackFromSeeds;
    tabgrp_->setTabEnabled( eventgrp_, usedata );
    tabgrp_->setTabEnabled( correlationgrp_, usedata );

    toolbar_->setSensitive( startbutid_, invol );
    toolbar_->setSensitive( stopbutid_, invol );
    toolbar_->setSensitive( retrackbutid_, usedata );
}


void uiHorizonSetupGroup::varianceChangeCB( CallBacker* )
{ varianceChanged_.trigger(); }


void uiHorizonSetupGroup::colorChangeCB( CallBacker* )
{
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedTypeSel( CallBacker* )
{
    const MarkerStyle3D::Type newtype =
	(MarkerStyle3D::Type)(MarkerStyle3D::None+seedtypefld_->getIntValue());
    if ( markerstyle_.type_ == newtype )
	return;
    markerstyle_.type_ = newtype;
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedSliderMove( CallBacker* )
{
    const float sldrval = seedsliderfld_->getFValue();
    const int newsize = mNINT32(sldrval);
    if ( markerstyle_.size_ == newsize )
	return;
    markerstyle_.size_ = newsize;
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedColSel( CallBacker* )
{
    const Color newcolor = seedcolselfld_->color();
    if ( markerstyle_.color_ == newcolor )
	return;
    markerstyle_.color_ = newcolor;
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    horadj_ = horadj;
    if ( !horadj_ ) return;

    initStuff();
    correlationgrp_->setSectionTracker( st );
    eventgrp_->setSectionTracker( st );
}


void uiHorizonSetupGroup::initModeGroup()
{
    if ( (!is2d_ && Horizon3DSeedPicker::nrSeedConnectModes()>0) ||
	 (is2d_ && Horizon2DSeedPicker::nrSeedConnectModes()>0) )
	modeselgrp_->selectButton( mode_ );

    methodfld_->setValue(
	horadj_->getCompareMethod()==EventTracker::SeedTrace ? 0 : 1 );
}


void uiHorizonSetupGroup::initStuff()
{
    initModeGroup();
//    initVarianceGroup();
//    selUseVariance(0);
    initPropertyGroup();
}


void uiHorizonSetupGroup::initVarianceGroup()
{
}


void uiHorizonSetupGroup::initPropertyGroup()
{
    seedsliderfld_->setValue( markerstyle_.size_ );
    seedcolselfld_->setColor( markerstyle_.color_ );
    seedtypefld_->setValue( markerstyle_.type_ - MarkerStyle3D::None );
}


void uiHorizonSetupGroup::setMode( EMSeedPicker::SeedModeOrder mode )
{
    mode_ = mode;
    modeselgrp_->selectButton( mode_ );
}


int uiHorizonSetupGroup::getMode()
{
    return modeselgrp_ ? modeselgrp_->selectedId() : -1;
}


void uiHorizonSetupGroup::setSeedPos( const Coord3& crd )
{
    eventgrp_->setSeedPos( crd );
    correlationgrp_->setSeedPos( crd );
}


void uiHorizonSetupGroup::setColor( const Color& col )
{ colorfld_->setColor( col ); }

const Color& uiHorizonSetupGroup::getColor()
{ return colorfld_->color(); }

void uiHorizonSetupGroup::setLineWidth( int w )
{ linewidthfld_->setValue( w ); }

int uiHorizonSetupGroup::getLineWidth() const
{ return linewidthfld_->getIntValue(); }


void uiHorizonSetupGroup::setMarkerStyle( const MarkerStyle3D& markerstyle )
{
    markerstyle_ = markerstyle;
    initPropertyGroup();
}


const MarkerStyle3D& uiHorizonSetupGroup::getMarkerStyle()
{
    return markerstyle_;
}


bool uiHorizonSetupGroup::commitToTracker( bool& fieldchange ) const
{
    if ( !sectiontracker_ || !horadj_ )
	return false;

    EM::EMObject& emobj = sectiontracker_->emObject();
    if ( horizonfld_->ioobj(true) )
	emobj.setMultiID( horizonfld_->key() );

    fieldchange = false;
    correlationgrp_->commitToTracker( fieldchange );
    eventgrp_->commitToTracker( fieldchange );

    if ( !horadj_ || horadj_->getNrAttributes()<1 )
    {
	uiMSG().warning( tr("Unable to apply tracking setup") );
	return true;
    }

    horadj_->setCompareMethod( methodfld_->getIntValue()==0 ?
		EventTracker::SeedTrace : EventTracker::AdjacentParent );

    return true;
}


void uiHorizonSetupGroup::showGroupOnTop( const char* grpnm )
{
    tabgrp_->setCurrentPage( grpnm );
    mDynamicCastGet(uiDialog*,dlg,parent())
    if ( dlg && !dlg->isHidden() )
    {
	 dlg->showNormal();
	 dlg->raise();
    }
}


} //namespace MPE
