/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiviscoltabed.cc,v 1.19 2007-02-05 18:19:48 cvsbert Exp $";

#include "uiviscoltabed.h"

#include "coltabedit.h"
#include "iopar.h"
#include "settings.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "uicursor.h"


uiVisColTabEd::uiVisColTabEd( uiParent* p, bool vert )
    : uiGroup(p,"ColorTable Editor")
    , coltabed_(0)
    , coltab_(0)
    , coltabcb( mCB(this,uiVisColTabEd,colTabChangedCB) )
    , sequenceChange(this)
    , coltabChange(this)
{
    const char* setkey = "dTect.Color table.Name";
    BufferString ctname = "Seismics";
    mSettUse(get,setkey,"",ctname);
    ColorTable ct( ctname );
    ct.scaleTo( Interval<float>(0,1) );

    coltabed_ = new ColorTableEditor( this, ColorTableEditor::Setup()
	    			     .editable(true)
	    			     .withclip(true)
	    			     .key(setkey)
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
				coltabcliprate_ != coltabed_->getClipRate() )
    {
	autoscalechange = true;
	coltabcliprate_ = coltabed_->getClipRate();
	coltab_->setClipRate( coltabcliprate_ );
	coltabautoscale_ = coltabed_->autoScale();
	coltab_->setAutoScale( coltabautoscale_ );
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

    ColorTable newct = colseq_;
    newct.scaleTo( coltabinterval_ );
    coltabed_->setColorTable( &newct );
    coltabed_->setAutoScale( coltabautoscale_ );
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


void uiColorBarDialog::uiColorBarDialog::setColTab( int id )
{
    coltabed_->setColTab(id);
}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
