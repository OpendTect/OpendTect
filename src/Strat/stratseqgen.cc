/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/


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
static const char* sKeyTopdepth = "Top depth";
static const char* sKeyElasticPropSelID = "Elastic Property Selection";

mImplFactory(Strat::LayerGenerator,Strat::LayerGenerator::factory)
mDefSimpleTranslators(StratLayerSequenceGenDesc,mFileType,od,Mdl);

const char* Strat::LayerSequenceGenDesc::sKeyWorkBenchParams() { return "WB"; }


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
	msg_ = tr("Generating layer sequences");
    else
    {
	msg_ = desc_.errMsg();
	seqnr_ = -1;
    }
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


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc(
	const LayerSequenceGenDesc& seq )
    : rt_(seq.rt_)
{
    *this = seq;
}


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc( const RefTree& rt )
    : rt_(rt)
    , startdepth_(0)
{
    elasticpropselmid_.setEmpty();
    int pidx = PROPS().indexOf( PropertyRef::Den );
    if ( pidx >= 0 ) propsel_ += PROPS()[pidx];
    pidx = PROPS().indexOf( PropertyRef::Vel );
    if ( pidx >= 0 ) propsel_ += PROPS()[pidx];
    pidx = PROPS().indexOf( PropertyRef::Imp );
    if ( pidx >= 0 ) propsel_ += PROPS()[pidx];
}


Strat::LayerSequenceGenDesc::~LayerSequenceGenDesc()
{
    deepErase( *this );
}


Strat::LayerSequenceGenDesc& Strat::LayerSequenceGenDesc::operator=(
	const Strat::LayerSequenceGenDesc& other )
{
    if ( this == &other ) return *this;
    deepErase( *this );
    for ( int idx=0; idx<other.size(); idx++ )
	*this += other[idx]->clone();
    setPropSelection( other.propsel_ );
    workbenchparams_ = other.workbenchparams_;
    elasticpropselmid_ = other.elasticpropselmid_;
    startdepth_ = other.startdepth_;
    return *this;
}


bool Strat::LayerSequenceGenDesc::getFrom( od_istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	{ errmsg_ = tr("Bad header found"); return false; }

    deepErase( *this );

    IOPar iop; iop.getFrom(astrm);
    iop.get( sKeyTopdepth, startdepth_ );
    iop.get( sKeyElasticPropSelID, elasticpropselmid_ );
    PtrMan<IOPar> workbenchpars = iop.subselect( sKeyWorkBenchParams() );
    if ( !workbenchpars || workbenchpars->isEmpty() )
	workbenchpars = iop.subselect( "Workbench parameters" );
    if ( workbenchpars )
	workbenchparams_ = *workbenchpars;
    else
	workbenchparams_.setEmpty();

    while ( !atEndOfSection(astrm.next()) )
    {
	iop.setEmpty(); iop.getFrom(astrm);
	if ( iop.isEmpty() )
	    continue;

	LayerGenerator* lg = LayerGenerator::get( iop, rt_ );
	if ( lg )
	    { lg->setGenDesc( this ); *this += lg; }
    }

    if ( isEmpty() )
    {
	errmsg_ = tr( "Empty description : no valid units found" );
	return false;
    }

    propsel_.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *(*this)[idx];
	lgen.updateUsedProps( propsel_ );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::putTo( od_ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	{ errmsg_ = tr("Cannot write file header"); return false; }

    IOPar iop;
    iop.set( sKeyTopdepth, startdepth_ );
    iop.set( sKeyElasticPropSelID, elasticpropselmid_ );
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
    errmsg_.setEmpty();
    for ( int idx=0; idx<size(); idx++ )
    {
	LayerGenerator& lgen = *const_cast<LayerGenerator*>(((*this)[idx]));
	lgen.gendesc_ = this;
	if ( !lgen.reset() )
	    errmsg_ = lgen.errMsg();
    }
    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty();

    ls.setStartDepth( startdepth_ );
    const Property::EvalOpts eo( Property::EvalOpts::New, modpos );
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);

	if ( !lgen.generateMaterial(ls,eo) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr("Error generating %1" ).arg( lgen.name() );
	    return false;
	}
    }

    for ( int idx=size()-1; idx>=0; idx-- )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	if ( !lgen.postProcess(ls,modpos) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr( "Error post-processing %1" ).arg( lgen.name() );
	    return false;
	}
    }
    ls.prepareUse();

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

    mDeclStaticString( ret ); ret = unnm;
    if ( dupls > 0 )
	ret.add( " [" ).add( dupls+1 ).add( "]" );
    return ret.buf();
}


static int fetchSeqNr( char* inpnm )
{
    int seqnr = 1;
    char* ptr = firstOcc( inpnm, '[' );
    if ( ptr )
    {
	*ptr++ = '\0';
	char* endptr = firstOcc( ptr+1, ']' );
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

    const int seqnr = fetchSeqNr( unnm.getCStr() );
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


Strat::SingleLayerGenerator::SingleLayerGenerator(
	const SingleLayerGenerator& laygen )
    : unit_(laygen.unit_ )
    , content_( laygen.content_ )
    , props_( laygen.props_ )
{
}


Strat::SingleLayerGenerator::SingleLayerGenerator( const LeafUnitRef* ur )
    : unit_(ur)
    , content_(&Strat::Content::unspecified())
{
    props_.add( new ValueProperty(PropertyRef::thickness()) );
}


Strat::LayerGenerator* Strat::SingleLayerGenerator::createClone() const
{
    Strat::SingleLayerGenerator* newlaygen =
	new Strat::SingleLayerGenerator( *this );
    return newlaygen;
}


const char* Strat::SingleLayerGenerator::name() const
{
    mDeclStaticString( ret ); ret = unit().fullCode();
    return ret.buf();
}


float Strat::SingleLayerGenerator::dispThickness( bool max ) const
{
    if ( props_.isEmpty() )
	return 1;
    const Property& thprop = props_.get( 0 );
    if ( !thprop.ref().isThickness() )
    {
	pErrMsg( "Thickness should always be the first property" );
	return 1;
    }

    if ( !max )
	return thprop.value( mPropertyEvalAvg );

    const float th0 = thprop.value( mPropertyEvalNew(0) );
    const float th1 = thprop.value( mPropertyEvalNew(1) );
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
	    if ( pr.hasFixedDef() )
		props_.add( pr.fixedDef().clone() );
	    else
		props_.add( new ValueProperty(pr) );
	}
    }

    //put everything in same order
    PropertySet copypropset( props_ );
    props_.erase();
    for ( int idx=0; idx<prsel.size(); idx++ )
    {
	const PropertyRef& pr = *prsel[idx];
	const int copyidx = copypropset.indexOf( pr );
	if ( copyidx<0 )
	    props_.add( new ValueProperty(pr) );
	else
	    props_.add( copypropset.get(copyidx).clone() );
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


bool Strat::SingleLayerGenerator::genMaterial( Strat::LayerSequence& seq,
					       Property::EvalOpts eo ) const
{
    const PropertyRefSelection& prs = seq.propertyRefs();

    Layer* newlay = new Layer( unit() );
    newlay->setContent( content() );
    const_cast<PropertySet*>(&props_)->resetMemory();

    TypeSet<int> indexesofprsmath;
    TypeSet<int> correspondingidxinprops;

    // first non-Math
    for ( int ipr=0; ipr<prs.size(); ipr++ )
    {
	const PropertyRef* pr = prs[ipr];

	for ( int iprop=0; iprop<props_.size(); iprop++ )
	{
	    const Property& prop = props_.get( iprop );
	    if ( pr != &prop.ref() )
		continue;

	    mDynamicCastGet(const MathProperty*,mp,&prop)
	    if ( !mp )
	    {
		const float val = prop.value( eo );
		if (mIsUdf(val) && !prop.errMsg().isEmpty())
		    { errmsg_ = prop.errMsg(); return false; }
		else if ( ipr == 0 && val < 1e-8 )
		    { delete newlay; return true; }

		newlay->setValue( iprop, val ) ;
	    }
	    else
	    {
		indexesofprsmath += ipr;
		correspondingidxinprops += iprop;
	    }
	    break;
	}
    }

    // then Math
    for ( int mathidx=0; mathidx<indexesofprsmath.size(); mathidx++ )
    {
	const int ipr = indexesofprsmath[mathidx];
	const PropertyRef* pr = prs[ipr];
	const Property& prop = props_.get( correspondingidxinprops[mathidx] );
	if ( pr != &prop.ref() )
	    { pErrMsg("Huh? should never happen"); continue; }
	if ( eo.isPrev() )
	    newlay->setValue( ipr, prop.value( eo ) );
	else
	{
	    mDynamicCastGet(const MathProperty&,mprop,prop)
	    newlay->setValue( ipr, mprop.getForm(), prs, eo.relpos_ );
	}
    }

    seq.layers() += newlay;
    return true;
}
