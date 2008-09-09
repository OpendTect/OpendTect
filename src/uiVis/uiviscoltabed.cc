/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiviscoltabed.cc,v 1.34 2008-09-09 10:52:11 cvsbert Exp $";

#include "uiviscoltabed.h"

#include "coltab.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "iopar.h"
#include "settings.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "uicolortable.h"
#include "mousecursor.h"


static const char* sSetKey = "dTect.Color table.Name";

const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }
const char* uiVisColTabEd::sKeySymMidval()	{ return "Symmetry Midvalue"; }

uiVisColTabEd::uiVisColTabEd( uiParent* p, bool vert )
    : viscoltab_(0)
    , colseq_(*new ColTab::Sequence(""))
    , coltabcb(mCB(this,uiVisColTabEd,colTabChangedCB))
    , sequenceChange(this)
    , coltabChange(this)
{
    uicoltab_ = new uiColorTable( p, colseq_, vert );
    if ( !vert ) uicoltab_->setStretch( 0, 0 );
    uicoltab_->seqChanged.notify( mCB(this,uiVisColTabEd,colTabEdChangedCB) );
    uicoltab_->scaleChanged.notify( mCB(this,uiVisColTabEd,colTabEdChangedCB) );
    visBase::DM().removeallnotify.notify( mCB(this,uiVisColTabEd,delColTabCB) );
    setColTab( -1 );
    coltabChange.trigger();
}


uiVisColTabEd::~uiVisColTabEd()
{
    visBase::DM().removeallnotify.remove( mCB(this,uiVisColTabEd,delColTabCB) );
    setColTab( -1 );
    delete uicoltab_;
}


int uiVisColTabEd::getColTab() const
{ return viscoltab_ ? viscoltab_->id() : -1; }


void uiVisColTabEd::setColTab( int id )
{
    if ( viscoltab_&& viscoltab_->id()==id ) return;

    visBase::DataObject* obj = id>=0 ? visBase::DM().getObject( id ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);

    if ( viscoltab_ )
    {
	disableCallBacks();
	viscoltab_->unRef();
    }

    viscoltab_ = nct;
    if ( viscoltab_ )
    {
	viscoltab_->ref();
	enableCallBacks();
	updateEditor();
    }

    if ( id > 0 )
    {
	uicoltab_->setTable( viscoltab_->colorSeq().colors() );
        uicoltab_->setInterval( viscoltab_->getInterval() );
    }
    uicoltab_->setSensitive( viscoltab_ );
}


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


void uiVisColTabEd::delColTabCB( CallBacker* )
{ setColTab( -1 ); }


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


void uiVisColTabEd::updateColTabList()
{ //coltabed_->updateColTabList();
}


bool uiVisColTabEd::usePar( const IOPar& par )
{
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
}


void uiVisColTabEd::fillPar( IOPar& par )
{
    par.set( sKeyColorSeq(), colseq_.name() );
    par.setYN( sKeyAutoScale(), coltabautoscale_ );
    if ( coltabautoscale_ )
    {
	par.set( sKeyClipRate(), coltabcliprate_ );
	par.set( sKeySymMidval(), coltabsymidval_ );
    }
    else
	par.set( sKeyScaleFactor(), coltabinterval_ );
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
uiColorBarDialog::uiColorBarDialog( uiParent* p, int coltabid,
				    const char* title )
    	: uiDialog(p, uiDialog::Setup(title,0,mNoHelpID).modal(false)
		   .oktext("Exit").dlgtitle("").canceltext(""))
	, winClosing( this )
	, coltabed_( new uiVisColTabEd(this,true) )
{
    coltabed_->setColTab( coltabid );
    //coltabed_->setPrefHeight( 320 );
}


void uiColorBarDialog::setColTab( int id )
{
    coltabed_->setColTab( id );
}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
