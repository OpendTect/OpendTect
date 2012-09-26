/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiviscoltabed.h"

#include "coltab.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "iopar.h"
#include "settings.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visdata.h"
#include "vissurvobj.h"
#include "uicolortable.h"
#include "mousecursor.h"


const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }
const char* uiVisColTabEd::sKeySymMidval()	{ return "Symmetry Midvalue"; }

uiVisColTabEd::uiVisColTabEd( uiParent* p, bool vert )
    : survobj_( 0 )
{
    const ColTab::Sequence colseq("");
    uicoltab_ = new uiColorTable( p, colseq, vert );
    if ( !vert ) uicoltab_->setStretch( 0, 0 );

    visBase::DM().removeallnotify.notify(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
}


#define mImplNotification( refop, notify) \
    if ( survobj_ ) \
    { \
	if ( survobj_->getColTabMapperSetup(channel_,version_) ) \
	    survobj_->getColTabMapperSetup( channel_, version_ )-> \
		rangeChange.notify( mCB(this,uiVisColTabEd,mapperChangeCB) ); \
	mDynamicCastGet( visBase::DataObject*, dataobj, survobj_ ); \
	if ( dataobj ) \
	    dataobj->refop(); \
    }

uiVisColTabEd::~uiVisColTabEd()
{
    mImplNotification( unRef, remove );
    visBase::DM().removeallnotify.remove(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
    delete uicoltab_;
}


void uiVisColTabEd::setColTab( const ColTab::Sequence* seq, bool editseq,
			       const ColTab::MapperSetup* setup,
       			       bool enabletrans )
{
    uicoltab_->setSequence( seq, editseq, true );
    uicoltab_->setMapperSetup( setup, true );
    uicoltab_->enableTransparencyEdit( enabletrans );
}


void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel,
			       int version )
{
    mImplNotification( unRef, remove );

    survobj_ = so;
    channel_ = channel;
    version_ = version;

    mImplNotification( ref, notify );

    uicoltab_->setSequence( so ? so->getColTabSequence(channel_) : 0,
			    so && so->canSetColTabSequence(), true );
    uicoltab_->setMapperSetup(
	    so ? so->getColTabMapperSetup(channel_,version_) : 0, true );
    uicoltab_->enableTransparencyEdit(
	    so && so->canHandleColTabSeqTrans(channel_) );
}


void uiVisColTabEd::mapperChangeCB( CallBacker* )
{
    uicoltab_->setMapperSetup(
	    survobj_->getColTabMapperSetup( channel_, version_ ), true );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    mImplNotification( unRef, remove);
    survobj_ = 0;
}



const ColTab::Sequence& uiVisColTabEd::getColTabSequence() const
{ return uicoltab_->colTabSeq(); }


const ColTab::MapperSetup& uiVisColTabEd::getColTabMapperSetup() const
{ return uicoltab_->colTabMapperSetup(); }


NotifierAccess& uiVisColTabEd::seqChange()
{ return uicoltab_->seqChanged; }


NotifierAccess& uiVisColTabEd::mapperChange()
{ return uicoltab_->scaleChanged; }


void uiVisColTabEd::setHistogram( const TypeSet<float>* hist )
{
    if ( uicoltab_ )
        uicoltab_->setHistogram( hist );
    else return;
}


void uiVisColTabEd::setPrefHeight( int height )
{ uicoltab_->setPrefHeight( height ); }

void uiVisColTabEd::setPrefWidth( int width )
{ uicoltab_->setPrefWidth( width ); }

/*
void uiVisColTabEd::colTabEdChangedCB( CallBacker* )
{
    if ( !viscoltab_ ) return;
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    bool seqchange = false;
    bool rangechange = false;
    bool autoscalechange = false;

    bool oldrangechstatus = viscoltab_->rangechange.enable( false );
    bool oldseqchstatus = viscoltab_->sequencechange.enable( false );

    ColTab::Sequence& newct = uicoltab_->colTabSeq();
    if ( !(newct == colseq_) )
    {
	seqchange = true;
	viscoltab_->colorSeq().colors() = newct;
	viscoltab_->colorSeq().colorsChanged();
	colseq_ = newct;
    }

    if ( uicoltab_->getInterval() != coltabinterval_ )
    {
	rangechange = true;
	coltabinterval_ = uicoltab_->getInterval();
	if ( coltabinterval_.start != coltabinterval_.stop )
	    viscoltab_->scaleTo( coltabinterval_ );
    }

    if ( uicoltab_->autoScale() != coltabautoscale_ ||
	 coltabcliprate_ != uicoltab_->getClipRate() ||
	 uicoltab_->getSymMidval() != coltabsymidval_ )
    {
	autoscalechange = true;
	coltabsymidval_ = uicoltab_->getSymMidval();
	viscoltab_->setSymMidval( coltabsymidval_ );
	coltabautoscale_ = uicoltab_->autoScale();
	viscoltab_->setAutoScale( coltabautoscale_ );
	coltabcliprate_ = uicoltab_->getClipRate();
	viscoltab_->setClipRate( coltabcliprate_ );
    }

    viscoltab_->rangechange.enable( oldrangechstatus );
    viscoltab_->sequencechange.enable( oldseqchstatus );

    if ( autoscalechange && uicoltab_->autoScale() )
    {
	viscoltab_->autoscalechange.remove( coltabcb );
	viscoltab_->triggerAutoScaleChange();
	viscoltab_->autoscalechange.notify( coltabcb );
    }
    else if ( rangechange )
    {
	viscoltab_->rangechange.remove( coltabcb );
	viscoltab_->triggerRangeChange();
	viscoltab_->rangechange.notify( coltabcb );
    }
    else if ( seqchange )
    {
	viscoltab_->sequencechange.remove( coltabcb );
	viscoltab_->triggerSeqChange();
	viscoltab_->sequencechange.notify( coltabcb );
    }

    coltabChange.trigger();
}



void uiVisColTabEd::colTabChangedCB( CallBacker* cb )
{
    updateEditor(); 
}


void uiVisColTabEd::updateEditor()
{
    coltabinterval_ = viscoltab_->getInterval();
    colseq_ = viscoltab_->colorSeq().colors();
    coltabautoscale_ = viscoltab_->autoScale();
    coltabcliprate_ = viscoltab_->clipRate();
    coltabsymidval_ = viscoltab_->symMidval();

    ColTab::Sequence& newct = colseq_;
    uicoltab_->setTable( viscoltab_->colorSeq().colors() );
    uicoltab_->setAutoScale( coltabautoscale_ );
    uicoltab_->setClipRate( coltabcliprate_ );
    uicoltab_->setSymMidval( coltabsymidval_ );
    uicoltab_->setInterval( coltabinterval_ );
}


void uiVisColTabEd::enableCallBacks()
{
    viscoltab_->rangechange.notify( coltabcb );
    viscoltab_->sequencechange.notify( coltabcb );
    viscoltab_->autoscalechange.notify( coltabcb );
}


void uiVisColTabEd::disableCallBacks()
{
    viscoltab_->rangechange.remove( coltabcb );
    viscoltab_->sequencechange.remove( coltabcb );
    viscoltab_->autoscalechange.remove( coltabcb );
}


*/

bool uiVisColTabEd::usePar( const IOPar& par )
{
    return true;
    /*
    
    BufferString coltabname;
    if ( !par.get(sKeyColorSeq(),coltabname) ) return false;

    uicoltab_->setTable( coltabname );

    bool autoscale = true;
    par.getYN( sKeyAutoScale(), autoscale );
    uicoltab_->setAutoScale( autoscale );

    if ( autoscale )
    {
	float cliprate = ColTab::defClipRate()/100;
	par.get( sKeyClipRate(), cliprate );
	uicoltab_->setClipRate( cliprate );

	bool symmetry = false;
	par.getYN( sKeySymmetry(), symmetry );

	float symmidval = mUdf(float);
	par.get( sKeySymMidval(), symmidval );
	uicoltab_->setSymMidval( symmetry ? 0 : symmidval );
    }
    else
    {
	Interval<float> coltabrange;
	par.get( sKeyScaleFactor(), coltabrange );
	uicoltab_->setInterval( coltabrange );
    }

    colTabEdChangedCB( 0 );
    return true;
    */
}


void uiVisColTabEd::fillPar( IOPar& par )
{
    /*
    par.set( sKeyColorSeq(), colseq_.name() );
    par.setYN( sKeyAutoScale(), coltabautoscale_ );
    if ( coltabautoscale_ )
    {
	par.set( sKeyClipRate(), coltabcliprate_ );
	par.set( sKeySymMidval(), coltabsymidval_ );
    }
    else
	par.set( sKeyScaleFactor(), coltabinterval_ );
    */
}

/*void uiVisColTabEd::setDefaultColTab()
{
    BufferString defcoltabnm = getDefColTabName();
    coltabed_->setColorTable( defcoltabnm );
    coltabed_->setAutoScale( true );
    coltabed_->setClipRate( ColorTable::defPercClip()/100 );
    colTabEdChangedCB( 0 );
}*/


// ----- uiColorBarDialog -----
uiColorBarDialog::uiColorBarDialog( uiParent* p, const char* title )
    	: uiDialog(p, uiDialog::Setup(title,0,mNoHelpID).modal(false)
		   .oktext("Exit").dlgtitle("").canceltext(""))
	, winClosing( this )
	, coltabed_( new uiVisColTabEd(this,true) )
{
    setDeleteOnClose( false );
}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
