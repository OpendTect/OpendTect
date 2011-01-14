/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqattrib.cc,v 1.2 2011-01-14 14:44:09 cvsbert Exp $";

#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "strattransl.h"
#include "propertyref.h"
#include "ascstream.h"
#include "keystrs.h"
#include "iopar.h"


#define mFileType "Layer Sequence Attribute Set"
static const char* sKeyFileType = mFileType;
mDefSimpleTranslators(StratLayerSequenceAttribSet,mFileType,od,Mdl);


Strat::LaySeqAttrib* Strat::LaySeqAttribSet::gtAttr( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( attr(idx).name() == nm )
	    return const_cast<Strat::LaySeqAttrib*>( (*this)[idx] );
    }
    return 0;
}


#define mDoIOPar(fn,ky,val) \
	iop.fn( IOPar::compKey(ky,idx), val )


void Strat::LaySeqAttribSet::putTo( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const LaySeqAttrib& lsa = attr( idx );
	mDoIOPar( set, sKey::Name, lsa.name() );
	mDoIOPar( set, sKey::Property, lsa.prop_.name() );
	mDoIOPar( set, LaySeqAttrib::sKeyStats(), lsa.stat_ );
	mDoIOPar( set, LaySeqAttrib::sKeyUnits(), lsa.units_ );
	mDoIOPar( set, LaySeqAttrib::sKeyLithos(), lsa.lithos_ );
    }
}


void Strat::LaySeqAttribSet::getFrom( const IOPar& iop )
{
    for ( int idx=0; ; idx++ )
    {
	const char* res = iop.find( IOPar::compKey(sKey::Property,idx) );
	if ( !res || !*res ) break;

	const PropertyRef* pr = PROPS().find( res );
	if ( !pr ) continue;
	BufferString nm; mDoIOPar( get, sKey::Name, nm );
	if ( nm.isEmpty() || attr(nm) ) continue;

	LaySeqAttrib* lsa = new LaySeqAttrib( *this, *pr, nm );
	mDoIOPar( get, LaySeqAttrib::sKeyStats(), lsa->stat_ );
	mDoIOPar( get, LaySeqAttrib::sKeyUnits(), lsa->units_ );
	mDoIOPar( get, LaySeqAttrib::sKeyLithos(), lsa->lithos_ );
    }
}


bool Strat::LaySeqAttribSet::getFrom( std::istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return false;

    IOPar iop; iop.getFrom( astrm );
    erase(); getFrom( iop );
    return true;
}


bool Strat::LaySeqAttribSet::putTo( std::ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	return false;

    IOPar iop; putTo( iop );
    iop.putTo( astrm );
    return strm.good();
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
