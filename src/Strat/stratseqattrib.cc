/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqattrib.cc,v 1.1 2011-01-13 14:52:13 cvsbert Exp $";

#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "strattransl.h"


mDefSimpleTranslators(StratLayerSequenceAttribSet,
		    "Layer Sequence Attribute Set",od,Attr);


Strat::LaySeqAttrib* Strat::LaySeqAttribSet::gtAttr( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( attr(idx).name() == nm )
	    return const_cast<Strat::LaySeqAttrib*>( (*this)[idx] );
    }
    return 0;
}

Strat::LaySeqAttrib* Strat::LaySeqAttribSet::gtAttr(const PropertyRef* pr) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( &attr(idx).prop_ == pr )
	    return const_cast<Strat::LaySeqAttrib*>( (*this)[idx] );
    }
    return 0;
}


Strat::LaySeqAttribCalc::LaySeqAttribCalc( const Strat::LaySeqAttrib& desc,
					   const Strat::LayerModel& lm )
{
}


float Strat::LaySeqAttribCalc::getValue( const LayerSequence& seq ) const
{
    //TODO
    return 0;
}
