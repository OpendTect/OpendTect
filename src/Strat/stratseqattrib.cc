/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqattrib.cc,v 1.4 2011-01-17 16:16:56 cvsbert Exp $";

#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "strattransl.h"
#include "stratlayer.h"
#include "propertyref.h"
#include "ascstream.h"
#include "separstr.h"
#include "keystrs.h"
#include "iopar.h"


#define mFileType "Layer Sequence Attribute Set"
static const char* sKeyFileType = mFileType;
mDefSimpleTranslators(StratLayerSequenceAttribSet,mFileType,od,Attr);
DefineEnumNames(Strat::LaySeqAttrib,Transform,1,"Value Transformation")
{ "Power", "Log", "Exp", 0 };


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
	FileMultiString fms( LaySeqAttrib::TransformNames()[lsa.transform_] );
	if ( mIsUdf(lsa.transformval_) )
	    fms += sKey::FloatUdf;
	else
	    fms += lsa.transformval_;
	mDoIOPar( set, LaySeqAttrib::sKeyTransform(), fms );
    }
}


void Strat::LaySeqAttribSet::getFrom( const IOPar& iop )
{
    for ( int idx=0; ; idx++ )
    {
	const char* res = iop.find( IOPar::compKey(sKey::Property,idx) );
	if ( !res || !*res ) break;

	const PropertyRef* pr = PROPS().find( res );
	if ( !pr && Strat::Layer::thicknessRef().name() == res )
	    pr = &Strat::Layer::thicknessRef();
	if ( !pr )
	    continue;
	BufferString nm; mDoIOPar( get, sKey::Name, nm );
	if ( nm.isEmpty() || attr(nm) ) continue;

	LaySeqAttrib* lsa = new LaySeqAttrib( *this, *pr, nm );
	mDoIOPar( get, LaySeqAttrib::sKeyStats(), lsa->stat_ );
	mDoIOPar( get, LaySeqAttrib::sKeyUnits(), lsa->units_ );
	mDoIOPar( get, LaySeqAttrib::sKeyLithos(), lsa->lithos_ );
	const char* ky = IOPar::compKey( LaySeqAttrib::sKeyTransform(), idx );
	const FileMultiString fms( iop.find(ky) );
	const int sz = fms.size();
	mSetUdf( lsa->transformval_ );
	if ( sz > 1 )
	{
	    lsa->transform_ = LaySeqAttrib::parseEnumTransform( fms[0] );
	    lsa->transformval_ = toFloat( fms[1] );
	    if ( lsa->transform_==LaySeqAttrib::Log && lsa->transformval_<0 )
		mSetUdf( lsa->transformval_ );
	}
	*this += lsa;
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
