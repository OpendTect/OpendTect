/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqgen.cc,v 1.16 2011-01-13 14:52:13 cvsbert Exp $";

#include "stratsinglaygen.h"
#include "stratreftree.h"
#include "stratlaymodgen.h"
#include "stratlayermodel.h"
#include "strattransl.h"
#include "propertyimpl.h"
#include "ascstream.h"
#include "keystrs.h"
#include "ptrman.h"
#include "iopar.h"


static const char* sKeyFileType = "Layer Sequence Generator Description";
static const char* sKeyIDNew = "[New]";
mImplFactory(Strat::LayerGenerator,Strat::LayerGenerator::factory)

mDefSimpleTranslators(StratLayerSequenceGenDesc,
		    "Layer Sequence Generation Description",od,Mdl);


Strat::LayerModelGenerator::LayerModelGenerator(
		const Strat::LayerSequenceGenDesc& desc, Strat::LayerModel& lm,
		int nrseqs )
    : Executor("Layer Sequence Generator")
    , desc_(desc)
    , lm_(lm)
    , nrseqs_(nrseqs)
    , seqnr_(0)
{
    reset();
}


void Strat::LayerModelGenerator::reset()
{
    lm_.setEmpty();
    seqnr_ = 0;
    if ( desc_.prepareGenerate() )
	msg_ = "Generating layer sequences";
    else
	msg_ = desc_.errMsg();
}


int Strat::LayerModelGenerator::nextStep()
{
    if ( seqnr_ == -1 )
	return ErrorOccurred();

    const float modpos = nrseqs_ < 2 ? 0.5 : ((float)seqnr_)/(nrseqs_-1);
    if ( !desc_.generate(lm_.addSequence(),modpos) )
    {
	msg_ = desc_.errMsg();
	return ErrorOccurred();
    }

    for ( int idx=0; idx<desc_.warnMsgs().size(); idx++ )
	ErrMsg( desc_.warnMsgs().get(idx) );
    seqnr_++;
    return seqnr_ >= nrseqs_ ? Finished() : MoreToDo();
}


Strat::LayerGenerator* Strat::LayerGenerator::get( const IOPar& iop,
						const Strat::RefTree& rt )
{
    Strat::LayerGenerator* ret = factory().create( iop.find(sKey::Type) );
    if ( !ret ) return 0;
    ret->usePar( iop, rt );
    return ret;
}


void Strat::LayerGenerator::usePar( const IOPar&, const Strat::RefTree& )
{
}


void Strat::LayerGenerator::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, factoryKeyword() );
}


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc( const RefTree& rt )
    : rt_(rt)
{
}


Strat::LayerSequenceGenDesc::~LayerSequenceGenDesc()
{
    deepErase( *this );
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
	*this += LayerGenerator::get( iop, rt_ );
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


bool Strat::LayerSequenceGenDesc::prepareGenerate() const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	if ( !lgen.reset() )
	    { errmsg_ = lgen.errMsg(); return false; }
    }
    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty(); warnmsgs_.erase();

    const Property::EvalOpts eo( false, modpos );
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	if ( !lgen.genMaterial(ls,eo) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_.add( "Error generating " )
		       .add( lgen.name() );
	    return false;
	}
	else if ( lgen.warnMsg() )
	    warnmsgs_.addIfNew( lgen.warnMsg() );
    }

    return true;
}


const char* Strat::LayerSequenceGenDesc::userIdentification( int unnr ) const
{
    if ( unnr >= size() )
	return sKeyIDNew;

    const BufferString unnm( (*this)[unnr]->name() );
    int dupls = 0;
    for ( int idx=0; idx<unnr; idx++ )
    {
	const BufferString nm( (*this)[idx]->name() );
	if ( nm == unnm )
	    dupls++;
    }

    static BufferString ret; ret = unnm;
    if ( dupls > 0 )
	ret.add( " [" ).add( dupls+1 ).add( "]" );
    return ret.buf();
}


static int fetchSeqNr( char* inpnm )
{
    int seqnr = 1;
    char* ptr = strchr( inpnm, '[' );
    if ( ptr )
    {
	*ptr++ = '\0';
	char* endptr = strchr( ptr+1, ']' );
	if ( endptr ) *endptr = '\0';
	seqnr = toInt( ptr );
    }
    return seqnr > 1 ? seqnr : 1;
}


int Strat::LayerSequenceGenDesc::indexFromUserIdentification(
					const char* unm ) const
{
    BufferString unnm( unm );
    if ( unnm == sKeyIDNew )
	return size();

    const int seqnr = fetchSeqNr( unnm.buf() );
    int nrfound = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( unnm == (*this)[idx]->name() )
	{
	    nrfound++;
	    if ( seqnr == nrfound )
		return idx;
	}
    }

    return -1;
}


Strat::SingleLayerGenerator::SingleLayerGenerator( const LeafUnitRef* ur )
    : unit_(ur)
{
    props_.add( new ValueProperty(Layer::thicknessRef()) );
}


const char* Strat::SingleLayerGenerator::name() const
{
    static BufferString ret; ret = unit().fullCode();
    return ret.buf();
}


float Strat::SingleLayerGenerator::dispThickness( bool max ) const
{
    if ( props_.isEmpty() )
	return 1;

    const float th0 = props_.get(0).value( Property::EvalOpts(false,0) );
    const float th1 = props_.get(0).value( Property::EvalOpts(false,1) );
    return th0 < th1 ? (max ? th1 : th0) : (max ? th0 : th1);
}


const Strat::LeafUnitRef& Strat::SingleLayerGenerator::unit() const
{
    return unit_ ? *unit_ : RT().undefLeaf();
}


void Strat::SingleLayerGenerator::getPropertySelection(
				PropertyRefSelection& pr ) const
{
    for ( int idx=0; idx<props_.size(); idx++ )
	pr += &props_.get( idx ).ref();
}


void Strat::SingleLayerGenerator::usePar( const IOPar& iop, const RefTree& rt )
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
	const PropertyRef* pref = PROPS().find( propnm );
	if ( !pref && (pidx == 0 || Layer::thicknessRef().name() == propnm) )
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

    reset();
}


bool Strat::SingleLayerGenerator::reset() const
{
    if ( !props_.prepareUsage() )
	{ errmsg_ = props_.errMsg(); return false; }
    return true;
}


void Strat::SingleLayerGenerator::fillPar( IOPar& iop ) const
{
    LayerGenerator::fillPar( iop );
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


bool Strat::SingleLayerGenerator::genMaterial( Strat::LayerSequence& seq,
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
	for ( int iprop=0; iprop<props_.size(); iprop++ )
	{
	    const Property& prop = props_.get( iprop );
	    if ( pr == &prop.ref() )
		{ newlay->setValue( ipr, prop.value(eo) ); break; }
	}
    }

    const float th = newlay->thickness();
    if ( th < 1e-8 )
	delete newlay;
    else
	seq.layers() += newlay;
    return true;
}
