/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiviscoltabed.cc,v 1.5 2003-11-07 12:22:02 bert Exp $";

#include "uiviscoltabed.h"

#include "coltabedit.h"
#include "iopar.h"
#include "settings.h"
#include "viscolortab.h"
#include "visdataman.h"

uiVisColTabEd::uiVisColTabEd( uiParent* p )
    : uiGroup( p, "ColorTable Editor" )
    , coltabed( 0 )
    , coltab( 0 )
    , coltabcb( mCB(this,uiVisColTabEd,colTabChangedCB) )
{
    const char* setkey = "dTect.Color table.Name";
    BufferString ctname = "Red-White-Black";
    mSettUse(get,setkey,"",ctname);
    ColorTable ct( ctname );
    ct.scaleTo( Interval<float>(0,1) );

    coltabed = new ColorTableEditor( this, &ct, false, setkey );
    coltabed->tablechanged.notify(mCB(this, uiVisColTabEd, colTabEdChangedCB));
    visBase::DM().removeallnotify.notify(mCB(this,uiVisColTabEd,delColTabCB));
}


uiVisColTabEd::~uiVisColTabEd()
{
    coltabed->tablechanged.remove(mCB(this,uiVisColTabEd, colTabEdChangedCB));
    visBase::DM().removeallnotify.remove(mCB(this,uiVisColTabEd,delColTabCB));
    setColTab( -1 );
    delete coltabed;
}


void uiVisColTabEd::setColTab( int id )
{
    if ( coltab && coltab->id()==id ) return;

    visBase::DataObject* obj = id>=0 ? visBase::DM().getObj( id ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);

    if ( coltab )
    {
	disableCallBacks();
	coltab->unRef();
    }

    coltab = nct;
    coltabed->setSensitive( coltab );

    if ( coltab )
    {
	coltab->ref();
	enableCallBacks();
	updateEditor();
    }
}


void uiVisColTabEd::setHistogram( const TypeSet<float>* hist )
{
    coltabed->setHistogram( hist );
}


void uiVisColTabEd::setPrefHeight( int nv )
{ coltabed->setPrefHeight( nv ); }


void uiVisColTabEd::colTabEdChangedCB( CallBacker* )
{
    if ( !coltab ) return;

    bool seqchange = false;
    bool rangechange = false;
    bool autoscalechange = false;

    bool oldrangechstatus = coltab->rangechange.enable( false );
    bool oldseqchstatus = coltab->sequencechange.enable( false );

    ColorTable newct = *coltabed->getColorTable();
    newct.scaleTo( Interval<float>( 0, 1 ) );
    if ( !(newct == colseq) )
    {
	seqchange = true;
	coltab->colorSeq().colors() = newct;
	coltab->colorSeq().colorsChanged();
	colseq = newct;
    }

    if ( coltabed->getColorTable()->getInterval() != coltabinterval )
    {
	rangechange = true;
	coltabinterval = coltabed->getColorTable()->getInterval();
	coltab->scaleTo( coltabinterval );
    }

    if ( coltabed->autoScale() != coltabautoscale ||
				    coltabcliprate != coltabed->getClipRate() )
    {
	autoscalechange = true;
	coltabcliprate = coltabed->getClipRate();
	coltab->setClipRate( coltabcliprate );
	coltabautoscale = coltabed->autoScale();
	coltab->setAutoScale( coltabautoscale );
    }

    coltab->rangechange.enable(oldrangechstatus);
    coltab->sequencechange.enable( oldseqchstatus );

    if ( autoscalechange && coltabed->autoScale() )
    {
	coltab->autoscalechange.remove(coltabcb);
	coltab->triggerAutoScaleChange();
	coltab->autoscalechange.notify(coltabcb);
    }
    else if ( rangechange )
    {
	coltab->rangechange.remove(coltabcb);
	coltab->triggerRangeChange();
	coltab->rangechange.notify(coltabcb);
    }
    else if ( seqchange )
    {
	coltab->sequencechange.remove(coltabcb);
	coltab->triggerSeqChange();
	coltab->sequencechange.notify(coltabcb);
    }
}


void uiVisColTabEd::colTabChangedCB( CallBacker* )
{ updateEditor(); }


void uiVisColTabEd::delColTabCB( CallBacker* )
{ setColTab( -1 ); }


void uiVisColTabEd::updateEditor()
{
    coltabinterval = coltab->getInterval();
    colseq = coltab->colorSeq().colors();
    coltabautoscale = coltab->autoScale();
    coltabcliprate = coltab->clipRate();

    ColorTable newct = colseq;
    newct.scaleTo( coltabinterval );
    coltabed->setColorTable( &newct );
    coltabed->setAutoScale( coltabautoscale );
    coltabed->setClipRate( coltabcliprate );
}


void uiVisColTabEd::enableCallBacks()
{
    coltab->rangechange.notify( coltabcb );
    coltab->sequencechange.notify( coltabcb );
    coltab->autoscalechange.notify( coltabcb );
}


void uiVisColTabEd::disableCallBacks()
{
    coltab->rangechange.remove(coltabcb);
    coltab->sequencechange.remove(coltabcb);
    coltab->autoscalechange.remove(coltabcb);
}


