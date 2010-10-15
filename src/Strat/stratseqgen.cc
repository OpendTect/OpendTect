/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqgen.cc,v 1.2 2010-10-15 13:38:41 cvsbert Exp $";

#include "stratsinglayseqgendesc.h"
#include "stratreftree.h"
#include "stratlaymodgen.h"
#include "stratlayermodel.h"
#include "propertyimpl.h"
#include "ascstream.h"
#include "keystrs.h"
#include "ptrman.h"
#include "iopar.h"

static const char* sKeyFileType = "Layer Sequence Generator Description";
mImplFactory(Strat::LayerGenDesc,Strat::LayerGenDesc::factory)


Strat::LayerModelGenerator::LayerModelGenerator(
		const Strat::LayerSequenceGenDesc& desc, Strat::LayerModel& lm )
    : Executor("Layer Sequence Generator")
    , desc_(desc)
    , lm_(lm)
    , nrseqs_(1)
    , seqnr_(0)
{
    reset();
}


void Strat::LayerModelGenerator::reset()
{
    lm_.setEmpty();
    seqnr_ = 0;
    msg_ = "Generating layer sequences";
}


int Strat::LayerModelGenerator::nextStep()
{
    const float modpos = nrseqs_ < 2 ? 0.5 : ((float)seqnr_)/(nrseqs_-1);
    desc_.generate( lm_.addSequence(), modpos );
    seqnr_++;
    return seqnr_ >= nrseqs_ ? Finished() : MoreToDo();
}


Strat::LayerGenDesc* Strat::LayerGenDesc::get( const IOPar& iop,
						const Strat::RefTree& rt )
{
    Strat::LayerGenDesc* ret = factory().create( iop.find(sKey::Type) );
    if ( !ret ) return 0;
    ret->usePar( iop, rt );
    return ret;
}


bool Strat::LayerSequenceGenDesc::getFrom( std::istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	{ errmsg_ = "Bad header found"; return false; }

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	if ( iop.isEmpty() ) continue;
	*this += LayerGenDesc::get( iop, rt_ );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::putTo( std::ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	{ errmsg_ = "Cannot write file header"; return false; }

    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar iop; (*this)[idx]->fillPar(iop);
	iop.putTo( astrm );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty(); warnmsgs_.erase();

    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenDesc& lgd = *((*this)[idx]);
	if ( !lgd.genMaterial(ls,modpos) )
	{
	    errmsg_ = lgd.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_.add( "Error generating " )
		       .add( lgd.name() );
	    return false;
	}
	else if ( lgd.warnMsg() )
	    warnmsgs_.addIfNew( lgd.warnMsg() );
    }

    return true;
}


const char* Strat::SingleLayerGenDesc::name() const
{
    BufferString ret; ret = unit().fullCode();
    return ret.buf();
}


const Strat::LeafUnitRef& Strat::SingleLayerGenDesc::unit() const
{
    return unit_ ? *unit_ : LeafUnitRef::undef();
}


void Strat::SingleLayerGenDesc::getPropertySelection(
					PropertyRefSelection& prs ) const
{
    for ( int idx=0; idx<props_.size(); idx++ )
	prs += &props_.get( idx ).ref();
}


void Strat::SingleLayerGenDesc::usePar( const IOPar& iop, const RefTree& rt )
{
    unit_ = 0;
    const char* res = iop.find( sKey::Unit );
    if ( res && *res )
    {
	const UnitRef* ur = rt.find( res );
	if ( ur && ur->isLeaf() )
	    unit_ = static_cast<const LeafUnitRef*>( ur );
    }

    props_.erase();
    for ( int pidx=0; ; pidx++ )
    {
	PtrMan<IOPar> proppar = iop.subselect(
				IOPar::compKey(sKey::Property,pidx) );
	if ( !proppar || proppar->isEmpty() )
	    break;

	const char* propnm = proppar->find( sKey::Name );
	if ( !propnm || !*propnm )
	    continue;
	const PropertyRef* pref = PROPS().find( res );
	if ( !pref && Layer::thicknessRef().name() == res )
	    pref = &Layer::thicknessRef();
	if ( !pref )
	    continue;

	const char* typ = proppar->find( sKey::Type );
	if ( !typ || !*typ ) typ = ValueProperty::typeStr();
	Property* prop = Property::factory().create( typ, *pref );

	res = proppar->find( sKey::Value );
	if ( res && *res )
	    prop->setDef( res );

	props_.set( prop );
    }

    props_.prepareEval();
}


void Strat::SingleLayerGenDesc::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Unit, unit().fullCode() );
    for ( int pidx=0; pidx<props_.size(); pidx++ )
    {
	const Property& prop = props_.get( pidx );
	const BufferString ky( IOPar::compKey(sKey::Property,pidx) );
	iop.set( IOPar::compKey(ky,sKey::Name), prop.name() );
	iop.set( IOPar::compKey(ky,sKey::Type), prop.type() );
	iop.set( IOPar::compKey(ky,sKey::Value), prop.def() );
    }
}


bool Strat::SingleLayerGenDesc::genMaterial( LayerSequence& seq,
					     Property::EvalOpts eo ) const
{
    const PropertyRefSelection& prs = seq.propertyRefs();
    if ( prs.isEmpty() ) 
    {
	PropertyRefSelection newprs;
	for ( int idx=0; idx<props_.size(); idx++ )
	    newprs += &props_.get(idx).ref();
	seq.setPropertyRefs( newprs );
    }

    Layer* newlay = new Layer( unit() );
    for ( int ipr=0; ipr<prs.size(); ipr++ )
    {
	const PropertyRef* pr = prs[ipr];
	for ( int iprop=0; ipr<props_.size(); iprop++ )
	{
	    const Property& prop = props_.get( ipr );
	    if ( pr == &prop.ref() )
		{ newlay->setValue( ipr, prop.value(eo) ); break; }
	}
    }

    seq.layers() += newlay;
    return true;
}
