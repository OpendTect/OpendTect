/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "stratlayseqgendesc.h"
#include "stratsinglaygen.h"
#include "stratreftree.h"
#include "stratlaymodgen.h"
#include "stratlayermodel.h"
#include "strattransl.h"
#include "mathproperty.h"
#include "ascstream.h"
#include "keystrs.h"
#include "ptrman.h"
#include "iopar.h"


#define mFileType "Layer Sequence Generator Description"

static const char* sKeyFileType = mFileType;
static const char* sKeyIDNew = "[New]";
static const char* sKeyElasticPropSelID = "Elastic Property Selection";

mImplFactory(Strat::LayerGenerator,Strat::LayerGenerator::factory)
mDefSimpleTranslators(StratLayerSequenceGenDesc,mFileType,od,Mdl);

const char* Strat::LayerSequenceGenDesc::sKeyWorkBenchParams()
{ return "Workbench parameters"; }

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

    const float modpos = nrseqs_ < 2 ? 0.5f : ((float)seqnr_)/(nrseqs_-1);
    if ( !desc_.generate(lm_.addSequence(),modpos) )
    {
	msg_ = desc_.errMsg();
	return ErrorOccurred();
    }

    seqnr_++;
    return seqnr_ >= nrseqs_ ? Finished() : MoreToDo();
}


Strat::LayerGenerator* Strat::LayerGenerator::get( const IOPar& iop,
						const Strat::RefTree& rt )
{
    Strat::LayerGenerator* ret = factory().create( iop.find(sKey::Type()) );
    if ( !ret ) return 0;
    if ( ret->usePar(iop,rt) )
	return ret;
    delete ret; return 0;
}


bool Strat::LayerGenerator::usePar( const IOPar&, const Strat::RefTree& )
{
    return true;
}


void Strat::LayerGenerator::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), factoryKeyword() );
}


bool Strat::LayerGenerator::generateMaterial( Strat::LayerSequence& seq,
       					      Property::EvalOpts eo ) const
{
    if ( seq.propertyRefs().isEmpty() )
	updateUsedProps( seq.propertyRefs() );
    return genMaterial( seq, eo );
}


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc( const RefTree& rt )
    : rt_(rt)
{
    elasticpropselmid_.setEmpty(); 
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

    deepErase( *this );

    IOPar iop; iop.getFrom(astrm);
    iop.get( sKeyElasticPropSelID, elasticpropselmid_ );
    PtrMan<IOPar> workbenchpars = iop.subselect( sKeyWorkBenchParams() );
    if ( workbenchpars )
	workbenchparams_ = *workbenchpars;

    while ( !atEndOfSection(astrm.next()) )
    {
	iop.setEmpty(); iop.getFrom(astrm);
	if ( iop.isEmpty() ) 
	    continue;

	LayerGenerator* lg = LayerGenerator::get( iop, rt_ );
	if ( lg )
	    *this += lg;
    }

    propsel_.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *(*this)[idx];
	lgen.updateUsedProps( propsel_ );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::putTo( std::ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	{ errmsg_ = "Cannot write file header"; return false; }

    IOPar iop; iop.set( sKeyElasticPropSelID, elasticpropselmid_ );
    iop.mergeComp( workbenchparams_, sKeyWorkBenchParams() );
    
    iop.putTo( astrm );

    for ( int idx=0; idx<size(); idx++ )
    {
	iop.setEmpty(); (*this)[idx]->fillPar(iop);
	iop.putTo( astrm );
    }

    return true;
}


void Strat::LayerSequenceGenDesc::setPropSelection(
		const PropertyRefSelection& prsel )
{
    propsel_ = prsel;
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx]->syncProps( propsel_ );
}


void Strat::LayerSequenceGenDesc::setElasticPropSel( const MultiID& mid )
{
    elasticpropselmid_ = mid;
}


const MultiID& Strat::LayerSequenceGenDesc::elasticPropSel() const
{
    return elasticpropselmid_; 
}


bool Strat::LayerSequenceGenDesc::prepareGenerate() const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	LayerGenerator& lgen = *const_cast<LayerGenerator*>(((*this)[idx]));
	lgen.gendesc_ = this;
	if ( !lgen.reset() )
	    { errmsg_ = lgen.errMsg(); return false; }
    }
    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty();

    const Property::EvalOpts eo( Property::EvalOpts::New, modpos );
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	
	if ( !lgen.generateMaterial(ls,eo) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_.add( "Error generating " ).add( lgen.name() );
	    return false;
	}
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	if ( !lgen.postProcess(ls,modpos) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_.add( "Error post-processing " ).add( lgen.name() );
	    return false;
	}
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
    , content_(&Strat::Content::unspecified())
{
    props_.add( new ValueProperty(PropertyRef::thickness()) );
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

    if ( !max )
	return props_.get(0).value( mPropertyEvalAvg );

    const float th0 = props_.get(0).value( mPropertyEvalNew(0) );
    const float th1 = props_.get(0).value( mPropertyEvalNew(1) );
    if ( mIsUdf(th0) ) return th1; if ( mIsUdf(th1) ) return th0;

    return th0 < th1 ? th1 : th0;
}


const Strat::LeafUnitRef& Strat::SingleLayerGenerator::unit() const
{
    return unit_ ? *unit_ : RT().undefLeaf();
}


void Strat::SingleLayerGenerator::syncProps( const PropertyRefSelection& prsel )
{
    // remove old
    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const PropertyRef* pr = &props_.get(idx).ref();
	if ( !prsel.isPresent(pr) )
	    { props_.remove(idx); idx--; }
    }
    // add new
    for ( int idx=0; idx<prsel.size(); idx++ )
    {
	const PropertyRef& pr = *prsel[idx];
	if ( props_.indexOf(pr) < 0 )
	{
	    if ( pr.disp_.defval_ )
		props_.add( pr.disp_.defval_->clone() );
	    else
		props_.add( new ValueProperty(pr) );
	}
    }
}


void Strat::SingleLayerGenerator::updateUsedProps(
					PropertyRefSelection& prsel ) const
{
    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const PropertyRef* pr = &props_.get(idx).ref();
	if ( !prsel.isPresent(pr) )
	    prsel += pr;
    }
}


bool Strat::SingleLayerGenerator::usePar( const IOPar& iop, const RefTree& rt )
{
    unit_ = 0;
    const char* res = iop.find( sKey::Unit() );
    if ( res && *res )
    {
	const UnitRef* ur = rt.find( res );
	if ( ur && ur->isLeaf() )
	    unit_ = static_cast<const LeafUnitRef*>( ur );
    }
    res = iop.find( sKey::Content() );
    if ( res && *res )
    {
	content_ = rt.contents().getByName( res );
	if ( !content_ ) content_ = &Content::unspecified();
    }

    props_.erase();
    for ( int pidx=0; ; pidx++ )
    {
	PtrMan<IOPar> proppar = iop.subselect(
				IOPar::compKey(sKey::Property(),pidx) );
	if ( !proppar || proppar->isEmpty() )
	    break;

	Property* prop = Property::get( *proppar );
	if ( prop )
	    props_.set( prop );
    }

    reset();
    return true;
}


void Strat::SingleLayerGenerator::fillPar( IOPar& iop ) const
{
    LayerGenerator::fillPar( iop );
    iop.set( sKey::Unit(), unit().fullCode() );

    if ( content().isUnspecified() )
	iop.removeWithKey( sKey::Content() );
    else
	iop.set( sKey::Content(), content().name() );

    for ( int pidx=0; pidx<props_.size(); pidx++ )
    {
	IOPar subpar; props_.get(pidx).fillPar( subpar );
	const BufferString ky( IOPar::compKey(sKey::Property(),pidx) );
	iop.mergeComp( subpar, ky );
    }
}


bool Strat::SingleLayerGenerator::reset() const
{
    if ( !props_.prepareUsage() )
	{ errmsg_ = props_.errMsg(); return false; }
    return true;
}


static bool setLayVal( Strat::Layer& lay, int iprop, const Property& prop,
			const Property::EvalOpts& eo )
{
    const float val = prop.value( eo );
    if ( mIsUdf(val) && prop.errMsg() && *prop.errMsg() )
	return false;

    lay.setValue( iprop, val ) ;
    return true;
}

#define mSetLayVal \
{ \
    if ( !setLayVal(*newlay,ipr,prop,eo) ) \
    {  \
	errmsg_ = prop.errMsg(); \
	delete newlay; return false; \
    } \
}




bool Strat::SingleLayerGenerator::genMaterial( Strat::LayerSequence& seq,
						Property::EvalOpts eo ) const
{
    const PropertyRefSelection& prs = seq.propertyRefs();

    Layer* newlay = new Layer( unit() );
    newlay->setContent( content() );

    TypeSet<int> indexesofprsmath;
    TypeSet<int> correspondingidxinprops;

    // first non-Math
    for ( int ipr=0; ipr<prs.size(); ipr++ )
    {
	const PropertyRef* pr = prs[ipr];

	for ( int iprop=0; iprop<props_.size(); iprop++ )
	{
	    const Property& prop = props_.get( iprop );
	    if ( pr == &prop.ref() )
	    {
		mDynamicCastGet(const MathProperty*,mp,&prop)
		if ( !mp )
		    mSetLayVal
		else
		{
		    indexesofprsmath += ipr;
		    correspondingidxinprops += iprop;
		}
		break;
	    }
	}
    }

    // then Math
    for ( int mathidx=0; mathidx<indexesofprsmath.size(); mathidx++ )
    {
	const int ipr = indexesofprsmath[mathidx];
	const PropertyRef* pr = prs[ipr];
	const Property& prop = props_.get( correspondingidxinprops[mathidx] );
	if ( pr != &prop.ref() ) continue; 	//huh? should never happen
	mSetLayVal
    }

    const float th = newlay->thickness();
    if ( th < 1e-8 )
	delete newlay;
    else
	seq.layers() += newlay;
    return true;
}
