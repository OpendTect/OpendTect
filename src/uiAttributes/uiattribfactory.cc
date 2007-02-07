/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uiattribfactory.cc,v 1.6 2007-02-07 11:10:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattribfactory.h"
#include "uiattrdesced.h"
#include "ptrman.h"


uiAttributeFactory& uiAF()
{
    static PtrMan<uiAttributeFactory> inst = 0;
    if ( !inst )
    {
	inst = new uiAttributeFactory;
	inst->fillStd();
    }
    return *inst;
}


int uiAttributeFactory::add( const char* dispnm, const char* attrnm,
			     const char* grpnm, uiAttrDescEdCreateFunc fn,
       			     int domtyp	)
{
    Entry* entry = getEntry( dispnm, true );
    if ( !entry )
	entry = getEntry( attrnm, false );

    if ( entry )
    {
	entry->dispnm_ = dispnm;
	entry->attrnm_ = attrnm;
	entry->grpnm_ = grpnm;
	entry->crfn_ = fn;
	entry->domtyp_ = domtyp;
    }
    else
    {
	entry = new Entry( dispnm, attrnm, grpnm, fn, domtyp );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


uiAttrDescEd* uiAttributeFactory::create( uiParent* p, const char* nm,
					  bool is2d, bool isdisp ) const
{
    Entry* entry = getEntry( nm, isdisp );
    if ( !entry ) return 0;

    uiAttrDescEd* ed = entry->crfn_( p, is2d );
    if ( ed )
    {
	ed->setDisplayName( entry->dispnm_ );
	ed->setDomainType( (uiAttrDescEd::DomainType)entry->domtyp_ );
    }
    return ed;
}


uiAttributeFactory::Entry* uiAttributeFactory::getEntry( const char* nm,
							 bool isdisp ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( (isdisp && entries_[idx]->dispnm_ == nm)
	  || (!isdisp && entries_[idx]->attrnm_ == nm) )
	    return const_cast<uiAttributeFactory*>( this )->entries_[idx];
    }

    return 0;
}


const char* uiAttributeFactory::dispNameOf( const char* attrnm ) const
{
    Entry* entry = getEntry( attrnm, false );
    return entry ? ((const char*)entry->dispnm_) : 0;
}


const char* uiAttributeFactory::attrNameOf( const char* attrnm ) const
{
    const Entry* entry = getEntry( attrnm, true );
    return entry ? ((const char*)entry->attrnm_) : 0;
}

#include "ui3dfilterattrib.h"
#include "uicoherencyattrib.h"
#include "uiconvolveattrib.h"
#include "uienergyattrib.h"
#include "uieventattrib.h"
#include "uifingerprintattrib.h"
#include "uifrequencyattrib.h"
#include "uifreqfilterattrib.h"
#include "uiinstantattrib.h"
#include "uimathattrib.h"
#include "uipositionattrib.h"
#include "uireferenceattrib.h"
#include "uiscalingattrib.h"
#include "uishiftattrib.h"
#include "uisimilarityattrib.h"
#include "uispecdecompattrib.h"
#include "uivolstatsattrib.h"


void uiAttributeFactory::fillStd()
{
    ui3DFilterAttrib::initClass();
    uiCoherencyAttrib::initClass();
    uiConvolveAttrib::initClass();
    uiEnergyAttrib::initClass();
    uiEventAttrib::initClass();
    uiFingerPrintAttrib::initClass();
    uiFrequencyAttrib::initClass();
    uiFreqFilterAttrib::initClass();
    uiInstantaneousAttrib::initClass();
    uiMathAttrib::initClass();
    uiPositionAttrib::initClass();
    uiReferenceAttrib::initClass();
    uiScalingAttrib::initClass();
    uiShiftAttrib::initClass();
    uiSimilarityAttrib::initClass();
    uiSpecDecompAttrib::initClass();
    uiVolumeStatisticsAttrib::initClass();
}
