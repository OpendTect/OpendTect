/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiviscoltabed.cc,v 1.24 2007-10-22 11:50:23 cvsnanne Exp $";

#include "uiviscoltabed.h"

#include "coltabedit.h"
#include "iopar.h"
#include "settings.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "uicursor.h"


static const char* sSetKey = "dTect.Color ta`ble.Name";

const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }

BufferString getDefColTabName()
{
    BufferString ctname = "Seismics";
    mSettUse(get,sSetKey,"",ctname);
    return ctname;
}


uiVisColTabEd::uiVisColTabEd( uiParent* p, bool vert )
    : coltabed_(0)
    , coltab_(0)
    , coltabcb( mCB(this,uiVisColTabEd,colTabChangedCB) )
    , sequenceChange(this)
    , coltabChange(this)
{
    ColorTable ct( getDefColTabName() );
    ct.scaleTo( Interval<float>(0,1) );

    coltabed_ = new ColorTableEditor( p, ColorTableEditor::Setup()
	    			     .editable(true)
	    			     .withclip(true)
	    			     .key(sSetKey)
	    			     .vertical(vert),
	    			     &ct );
    coltabed_->tablechanged.notify( mCB(this,uiVisColTabEd,colTabEdChangedCB) );
    visBase::DM().removeallnotify.notify( mCB(this,uiVisColTabEd,delColTabCB) );
    setColTab( -1 );
}


uiVisColTabEd::~uiVisColTabEd()
{
    coltabed_->tablechanged.remove( mCB(this,uiVisColTabEd,colTabEdChangedCB) );
    visBase::DM().removeallnotify.remove( mCB(this,uiVisColTabEd,delColTabCB) );
    setColTab( -1 );
    delete coltabed_;
}


int uiVisColTabEd::getColTab() const
{ return coltab_ ? coltab_->id() : -1; }


void uiVisColTabEd::setColTab( int id )
{
    if ( coltab_ && coltab_->id()==id ) return;

    visBase::DataObject* obj = id>=0 ? visBase::DM().getObject( id ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);

    if ( coltab_ )
    {
	disableCallBacks();
	coltab_->unRef();
    }

    coltab_ = nct;
    if ( coltab_ )
    {
	coltab_->ref();
	enableCallBacks();
	updateEditor();
    }

    coltabed_->setSensitive( coltab_ );
}


void uiVisColTabEd::setHistogram( const TypeSet<float>* hist )
{
    coltabed_->setHistogram( hist );
}


void uiVisColTabEd::setPrefHeight( int nv )
{ coltabed_->setPrefHeight( nv ); }


void uiVisColTabEd::colTabEdChangedCB( CallBacker* )
{
    if ( !coltab_ ) return;
    uiCursorChanger cursorchanger( uiCursor::Wait );

    bool seqchange = false;
    bool rangechange = false;
    bool autoscalechange = false;

    bool oldrangechstatus = coltab_->rangechange.enable( false );
    bool oldseqchstatus = coltab_->sequencechange.enable( false );

    ColorTable newct = *coltabed_->getColorTable();
    newct.scaleTo( Interval<float>(0,1) );
    if ( !(newct == colseq_) )
    {
	seqchange = true;
	coltab_->colorSeq().colors() = newct;
	coltab_->colorSeq().colorsChanged();
	colseq_ = newct;
    }

    if ( coltabed_->getColorTable()->getInterval() != coltabinterval_ )
    {
	rangechange = true;
	coltabinterval_ = coltabed_->getColorTable()->getInterval();
	coltab_->scaleTo( coltabinterval_ );
    }

    if ( coltabed_->autoScale() != coltabautoscale_ ||
				coltabcliprate_ != coltabed_->getClipRate() ||
				coltabed_->getSymmetry() != coltabsymmetry_ )
    {
	autoscalechange = true;
	coltabsymmetry_ = coltabed_->getSymmetry();
	coltab_->setSymmetry( coltabsymmetry_ );
	coltabautoscale_ = coltabed_->autoScale();
	coltab_->setAutoScale( coltabautoscale_ );
	coltabcliprate_ = coltabed_->getClipRate();
	coltab_->setClipRate( coltabcliprate_ );
    }

    coltab_->rangechange.enable( oldrangechstatus );
    coltab_->sequencechange.enable( oldseqchstatus );

    if ( autoscalechange && coltabed_->autoScale() )
    {
	coltab_->autoscalechange.remove( coltabcb );
	coltab_->triggerAutoScaleChange();
	coltab_->autoscalechange.notify( coltabcb );
    }
    else if ( rangechange )
    {
	coltab_->rangechange.remove( coltabcb );
	coltab_->triggerRangeChange();
	coltab_->rangechange.notify( coltabcb );
    }
    else if ( seqchange )
    {
	coltab_->sequencechange.remove( coltabcb );
	coltab_->triggerSeqChange();
	coltab_->sequencechange.notify( coltabcb );
	sequenceChange.trigger();
    }

    coltabChange.trigger();
}


void uiVisColTabEd::colTabChangedCB( CallBacker* )
{ updateEditor(); }


void uiVisColTabEd::delColTabCB( CallBacker* )
{ setColTab( -1 ); }


void uiVisColTabEd::updateEditor()
{
    coltabinterval_ = coltab_->getInterval();
    colseq_ = coltab_->colorSeq().colors();
    coltabautoscale_ = coltab_->autoScale();
    coltabcliprate_ = coltab_->clipRate();
    coltabsymmetry_ = coltab_->getSymmetry();

    ColorTable newct = colseq_;
    newct.scaleTo( coltabinterval_ );
    coltabed_->setColorTable( &newct );
    coltabed_->setAutoScale( coltabautoscale_ );
    coltabed_->setSymmetry( coltabsymmetry_ );
    coltabed_->setClipRate( coltabcliprate_ );
}


void uiVisColTabEd::enableCallBacks()
{
    coltab_->rangechange.notify( coltabcb );
    coltab_->sequencechange.notify( coltabcb );
    coltab_->autoscalechange.notify( coltabcb );
}


void uiVisColTabEd::disableCallBacks()
{
    coltab_->rangechange.remove( coltabcb );
    coltab_->sequencechange.remove( coltabcb );
    coltab_->autoscalechange.remove( coltabcb );
}


void uiVisColTabEd::updateColTabList()
{ coltabed_->updateColTabList(); }


bool uiVisColTabEd::usePar( const IOPar& par )
{
    BufferString coltabname;
    if ( !par.get(sKeyColorSeq(),coltabname) ) return false;

    coltabed_->setColorTable( coltabname );

    bool autoscale = true;
    par.getYN( sKeyAutoScale(), autoscale );
    coltabed_->setAutoScale( autoscale );

    if ( autoscale )
    {
	float cliprate = ColorTable::defPercClip()/100;
	par.get( sKeyClipRate(), cliprate );
	coltabed_->setClipRate( cliprate );

	bool symmetry;
	par.getYN( sKeySymmetry(), symmetry );
	coltabed_->setSymmetry( symmetry );
    }
    else
    {
	Interval<float> coltabrange;
	par.get( sKeyScaleFactor(), coltabrange );
	coltabed_->setInterval( coltabrange );
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
	par.setYN( sKeySymmetry(), coltabsymmetry_ );
    }
    else
	par.set( sKeyScaleFactor(), coltabinterval_ );
}

void uiVisColTabEd::setDefaultColTab()
{
    BufferString defcoltabnm = getDefColTabName();
    coltabed_->setColorTable( defcoltabnm );
    coltabed_->setAutoScale( true );
    coltabed_->setClipRate( ColorTable::defPercClip()/100 );
    colTabEdChangedCB( 0 );
}


// ----- uiColorBarDialog -----
uiColorBarDialog::uiColorBarDialog( uiParent* p, int coltabid,
				    const char* title )
    	: uiDialog(p, uiDialog::Setup(title,0,"50.0.6").modal(false)
		   .oktext("Exit").dlgtitle("").canceltext(""))
	, winClosing( this )
	, coltabed_( new uiVisColTabEd(this,true) )
{
    coltabed_->setColTab( coltabid );
    coltabed_->setPrefHeight( 320 );
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
