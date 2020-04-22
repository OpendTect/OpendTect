/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/


#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "mathformula.h"
#include "mathproperty.h"
#include "separstr.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "elasticpropsel.h"

static const char* sKeyXPos = "XPos";
static const char* sKeyRelZ = "RelZ";


//------ LayerValue ------


BufferString Strat::LayerValue::dumpStr() const
{
    BufferString ret;
    if ( isSimple() )
	ret = toString( value() );
    else
    {
	mDynamicCastGet(const FormulaLayerValue&,flv,*this);
	IOPar iop; flv.fillPar( iop );
	iop.putTo( ret );
    }
    return ret;
}


Strat::FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
	const Strat::Layer& lay, const PropertyRefSelection& prs,
	float xpos, float relz )
    : form_(form)
    , lay_(lay)
    , myform_(false)
{
    setXPos( xpos );
    setRelZ( relz );
    useForm( prs );
}


Strat::FormulaLayerValue::FormulaLayerValue( const IOPar& iop,
		const Strat::Layer& lay, const PropertyRefSelection& prs )
    : form_(*new Math::Formula(false,MathProperty::getSpecVars()))
    , myform_(true)
    , lay_(lay)
    , xpos_(0.5f)
    , relz_(0.5f)
{
    const_cast<Math::Formula&>(form_).usePar( iop );

    const char* res = iop.find( sKeyXPos );
    if ( res )
	setXPos( toFloat(res) );
    res = iop.find( sKeyRelZ );
    if ( res )
	setRelZ( toFloat(res) );

    useForm( prs );
}


Strat::FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
			    const Strat::Layer& lay, float xpos, float relz,
			    bool cpform )
    : form_(cpform ? *new Math::Formula(form) : form)
    , myform_(cpform)
    , lay_(lay)
{
    setXPos( xpos );
    setRelZ( relz );
}


void Strat::FormulaLayerValue::setXPos( float xpos )
{
    if ( xpos < 0.f )
	xpos = 0.f;
    if ( xpos > 1.f )
	xpos = 1.f;
    xpos_ = xpos;
}


void Strat::FormulaLayerValue::setRelZ( float relz )
{
    if ( relz < 0.f )
	relz = 0.f;
    if ( relz > 1.f )
	relz = 1.f;
    relz_ = relz;
}


void Strat::FormulaLayerValue::useForm( const PropertyRefSelection& prs )
{
    const int nrinps = form_.nrInputs();

    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	int inpidx = -1;
	float inpval = 0.f;
	if ( form_.isConst(iinp) )
	   inpval = (float)form_.getConstVal( iinp );
        else if ( !form_.isSpec(iinp) )
	{
	    const char* pnm = form_.inputDef( iinp );
	    inpidx = prs.indexOf( pnm );
	    if ( inpidx < 0 )
	    {
		errmsg_ = tr("%1 - Formula cannot be resolved:\n'%2'"
                             "\nCannot find '%3'")
                        .arg(lay_.name()).arg( form_.text() ).arg( pnm );

		return;
	    }
	}

	inpidxs_ += inpidx; // not more than one because no shifts allowed
	inpvals_ += inpval;
    }
}


Strat::FormulaLayerValue::~FormulaLayerValue()
{
    if ( myform_ )
	delete &form_;
}


Strat::FormulaLayerValue* Strat::FormulaLayerValue::clone(
					const Layer* lay ) const
{
    FormulaLayerValue* ret = new FormulaLayerValue( form_, lay ? *lay : lay_,
						    xpos_, relz_, myform_ );
    ret->inpidxs_ = inpidxs_;
    ret->inpvals_ = inpvals_;
    ret->errmsg_ = errmsg_;
    return ret;
}


float Strat::FormulaLayerValue::value() const
{
    if ( isBad() )
	return mUdf(float);

    const int nrinps = form_.nrInputs();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const int inpidx = inpidxs_[iinp];
	if ( inpidx >= 0 )
	    inpvals_[iinp] = lay_.value( inpidx );
	else if ( form_.isSpec(iinp) )
	{
	    const int specidx = form_.specIdx( iinp );
	    if ( specidx < 2 )
		inpvals_[iinp] = lay_.depth();
	    else if ( specidx > 3 )
		inpvals_[iinp] = xpos_;
	    else
		inpvals_[iinp] = relz_;
	}
	// consts are already filled
    }

    return form_.getValue( inpvals_.arr() );
}


void Strat::FormulaLayerValue::fillPar( IOPar& iop ) const
{
    form_.fillPar( iop );
    iop.set( sKeyXPos, xpos_ );
    iop.set( sKeyRelZ, relz_ );
}


//------ Layer ------


const PropertyRef& Strat::Layer::thicknessRef()
{
    return PropertyRef::thickness();
}


Strat::Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
    , content_(0)
{
    vals_.setNullAllowed( true );
    setThickness( 0.0f );
}


Strat::Layer::Layer( const Strat::Layer& oth )
{
    vals_.setNullAllowed( true );
    *this = oth;
}


Strat::Layer::~Layer()
{
    deepErase( vals_ );
}


Strat::Layer& Strat::Layer::operator =( const Strat::Layer& oth )
{
    if ( this != &oth )
    {
	content_ = oth.content_;
	ref_ = oth.ref_;
	ztop_ = oth.ztop_;
	deepErase( vals_ );
	for ( int ival=0; ival<oth.vals_.size(); ival++ )
	{
	    const LayerValue* lv = oth.vals_[ival];
	    if ( !lv )
		vals_ += 0;
	    else
	    {
		LayerValue* newlv = lv->clone( this );
		vals_ += newlv;
	    }
	}
    }
    return *this;
}


BufferString Strat::Layer::name() const
{
    return BufferString( unitRef().fullCode().buf() );
}


bool Strat::Layer::isMath( int ival ) const
{
    const LayerValue* lv = getLayerValue( ival );
    return lv ? !lv->isSimple() : false;
}


const Strat::LayerValue* Strat::Layer::getLayerValue( int ival ) const
{
    return vals_.validIdx( ival ) ? vals_[ival] : 0;
}


Color Strat::Layer::dispColor( bool lith ) const
{
    return unitRef().dispColor( lith );
}


void Strat::Layer::getValues( TypeSet<float>& out ) const
{
    const int nrvals = nrValues();
    out.setSize( nrvals );
    for ( int ival=0; ival<nrvals; ival++ )
	out[ival] = value( ival );
}


const Strat::RefTree& Strat::Layer::refTree() const
{
    return unitRef().refTree();
}


Strat::Layer::ID Strat::Layer::id() const
{
    return unitRef().fullCode();
}


float Strat::Layer::value( int ival ) const
{
    const LayerValue* lv = vals_.validIdx(ival) ? vals_[ival] : 0;
    return lv ? lv->value() : mUdf(float);
}


#define mEnsureEnoughVals() while ( vals_.size() <= ival ) vals_ += 0


void Strat::Layer::setValue( int ival, float val )
{
    mEnsureEnoughVals();

    Strat::LayerValue* lv = vals_[ival];
    if ( lv && lv->isSimple() )
	static_cast<SimpleLayerValue*>(lv)->setValue( val );
    else
	setLV( ival, new SimpleLayerValue(val) );
}


void Strat::Layer::setValue( int ival, const Math::Formula& form,
			     const PropertyRefSelection& prs,
			     float xpos, float relz )
{
    mEnsureEnoughVals();

    setLV( ival, new FormulaLayerValue(form,*this,prs,xpos,relz) );
}


void Strat::Layer::setValue( int ival, const IOPar& iop,
				const PropertyRefSelection& prs )
{
    mEnsureEnoughVals();

    if ( iop.size() == 1 && iop.getKey(0) == sKey::Value() )
	setValue( ival, toFloat(iop.getValue(0)) );
    else
	setLV( ival, new FormulaLayerValue(iop,*this,prs) );
}


void Strat::Layer::setValue( int ival, LayerValue* lv )
{
    mEnsureEnoughVals();
    setLV( ival, lv );
}


void Strat::Layer::setLV( int ival, LayerValue* lv )
{
    delete vals_.replace( ival, lv );
}


float Strat::Layer::thickness() const
{
    float val = value( 0 );
    if ( val < 0 )
	{ pErrMsg("thickness < 0 found" ); val = 0.0f; }
    return val;
}


void Strat::Layer::setXPos( float xpos )
{
    const int nrvals = vals_.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	Strat::LayerValue* lv = vals_[ival];
	if ( lv )
	    lv->setXPos( xpos );
    }
}


void Strat::Layer::setRelZ( float relz )
{
    const int nrvals = vals_.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	Strat::LayerValue* lv = vals_[ival];
	if ( lv )
	    lv->setRelZ( relz );
    }
}


void Strat::Layer::setThickness( float v )
{
    setValue( 0, v );
}


const Strat::Lithology& Strat::Layer::lithology() const
{
    return unitRef().getLithology();
}


const Strat::Content& Strat::Layer::content() const
{
    return content_ ? *content_ : Content::unspecified();
}


//------ LayerSequence ------


Strat::LayerSequence::LayerSequence( const PropertyRefSelection* prs )
    : z0_(0)
{
    if ( prs ) props_ = *prs;
}


Strat::LayerSequence::~LayerSequence()
{
    setEmpty();
}


Strat::LayerSequence& Strat::LayerSequence::operator =(
					const Strat::LayerSequence& oth )
{
    if ( this != &oth )
    {
	deepCopy( layers_, oth.layers_ );
	z0_ = oth.z0_;
	props_ = oth.props_;
    }
    return *this;
}


void Strat::LayerSequence::setXPos( float xpos )
{
    const int nrlays = layers_.size();
    for ( int ilay=0; ilay<nrlays; ilay++ )
	layers_[ilay]->setXPos( xpos );
}


const Strat::RefTree& Strat::LayerSequence::refTree() const
{
    return isEmpty() ? RT() : layers_[0]->refTree();
}


int Strat::LayerSequence::layerIdxAtZ( float zreq ) const
{
    const int nrlays = layers_.size();
    if ( nrlays == 0 || zreq < layers_[0]->zTop()
		     || zreq > layers_[nrlays-1]->zBot() )
	return -1;

    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	if ( zreq < layers_[ilay]->zBot() )
	    return ilay;
    }
    return nrlays-1;
}


int Strat::LayerSequence::nearestLayerIdxAtZ( float zreq ) const
{
    const int nrlays = layers_.size();
    if ( nrlays < 2 )
	return nrlays == 1 ? 0 : -1;

    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	if ( zreq < layers_[ilay]->zBot() )
	    return ilay;
    }
    return nrlays - 1;
}


Interval<float> Strat::LayerSequence::zRange() const
{
    if ( isEmpty() )
	return Interval<float>( z0_, z0_ );
    return Interval<float>( z0_, layers_[layers_.size()-1]->zBot() );
}


Interval<float> Strat::LayerSequence::propRange( int propnr ) const
{
    if ( propnr < 0 )
	return zRange();

    Interval<float> rg( mUdf(float), mUdf(float) );
    const auto nrlays = layers_.size();
    if ( nrlays < 1 || propnr >= propertyRefs().size() )
	return rg;

    for ( auto ilay=0; ilay<nrlays; ilay++ )
    {
	const auto layval = layers_.get(ilay)->value( propnr );
	if ( mIsUdf(layval) )
	    continue;

	if ( mIsUdf(rg.start) )
	    rg.start = rg.stop = layval;
	else
	    rg.include( layval );
    }

    return rg;
}


int Strat::LayerSequence::indexOf( const Strat::Level& lvl, int startat ) const
{
    const RefTree& rt = refTree();
    Strat::UnitRefIter it( rt, Strat::UnitRefIter::LeavedNodes );
    const Strat::LeavedUnitRef* lvlunit = 0;
    while ( it.next() )
    {
	const Strat::LeavedUnitRef* un
		= static_cast<const Strat::LeavedUnitRef*>( it.unit() );
	if ( un->levelID() == lvl.id() )
	    { lvlunit = un; break; }
    }
    if ( !lvlunit )
	return -1;

    for ( int ilay=startat; ilay<size(); ilay++ )
    {
	const LeafUnitRef& lur = layers_[ilay]->unitRef();
	if ( lur.upNode() == lvlunit )
	    return ilay;
    }
    return -1;
}


float Strat::LayerSequence::depthOf( const Strat::Level& lvl,
				     float notfoundval ) const
{
    const int sz = size();
    if ( sz < 1 )
	return notfoundval;
    const int idx = indexOf( lvl, 0 );
    return idx < 0 ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


int Strat::LayerSequence::positionOf( const Strat::Level& lvl ) const
{
    const RefTree& rt = refTree();
    Strat::UnitRefIter it( rt, Strat::UnitRefIter::LeavedNodes );
    ObjectSet<const Strat::UnitRef> unlist;
    bool foundlvl = false;
    while ( it.next() )
	// gather all units below level into unlist
    {
	const Strat::LeavedUnitRef* un
		= static_cast<const Strat::LeavedUnitRef*>( it.unit() );
	if ( foundlvl || un->levelID() == lvl.id() )
	    { foundlvl = true; unlist += un; }
    }
    if ( !foundlvl )
	return -1;

    for ( int ilay=0; ilay<size(); ilay++ )
		// find first layer whose parent is in unlist
    {
	const LeafUnitRef& un = layers_[ilay]->unitRef();
	for ( int iun=0; iun<unlist.size(); iun++ )
	{
	    if ( unlist[iun]->isParentOf(un) )
		return ilay;
	}
    }

    // level must be below last layer
    return size();
}


float Strat::LayerSequence::depthPositionOf( const Strat::Level& lvl,
						float notfoundval ) const
{
    const int sz = size();
    if ( sz < 1 )
	return notfoundval;
    const int idx = positionOf( lvl );
    if ( idx < 0 )
	return notfoundval;
    return idx >= sz ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


void Strat::LayerSequence::getLayersFor( const UnitRef* ur,
					 ObjectSet<const Layer>& lys ) const
{
    const int sz = size();
    if ( sz < 1 )
	return;
    if ( !ur )
	ur = &refTree();

    for ( int ilay=0; ilay<sz; ilay++ )
    {
	const Layer* ly = layers_[ilay];
	if ( ur == &ly->unitRef() || ur->isParentOf(ly->unitRef()) )
	    lys += ly;
    }
}


void Strat::LayerSequence::getSequencePart( const Interval<float>& depthrg,
					    bool cropfirstlast,
					    LayerSequence& out ) const
{
    out.setEmpty();
    const int sz = size();
    if ( sz < 1 || depthrg.isUdf() )
	return;

    for ( int ilay=0; ilay<layers_.size(); ilay++ )
    {
	const Layer& lay = *layers_[ilay];
	if ( lay.zBot() < depthrg.start + 1e-6f )
	    continue;
	else if ( lay.zTop() > depthrg.stop - 1e-6f )
	    break;

	Layer* newlay = new Layer( lay );
	if ( lay.zTop() < depthrg.start )
	    newlay->setThickness( lay.zBot() - depthrg.start );
	else if ( lay.zBot() > depthrg.stop )
	    newlay->setThickness( depthrg.stop - lay.zTop() );

	out.layers() += newlay;
    }

    out.z0_ = depthrg.start;
    out.prepareUse();
}


void Strat::LayerSequence::prepareUse() const
{
    float z = z0_;
    for ( int ilay=0; ilay<size(); ilay++ )
    {
	Layer& ly = *const_cast<Layer*>( layers_[ilay] );
	ly.setZTop( z );
	z += ly.thickness();
    }
}


//------ LayerModel ------


Strat::LayerModel::LayerModel()
{
    proprefs_ += &Layer::thicknessRef();
}


Strat::LayerModel::~LayerModel()
{
    deepErase( seqs_ );
}


Strat::LayerModel& Strat::LayerModel::operator =( const Strat::LayerModel& oth )
{
    if ( this == &oth )
	return *this;

    setEmpty();
    proprefs_ = oth.proprefs_;
    elasticpropsel_ = oth.elasticpropsel_;
    for ( const auto seq : oth.seqs_ )
    {
	auto* newseq = new LayerSequence( *seq );
	newseq->propertyRefs() = proprefs_;
	seqs_ += newseq;
    }

    return *this;
}


bool Strat::LayerModel::isValid() const
{
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
    {
	if ( !seqs_[iseq]->isEmpty() )
	    return true;
    }

    return false;
}


bool Strat::LayerModel::isEmpty() const
{
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
	if ( !seqs_.get(iseq)->isEmpty() )
	    return false;
    return true;
}


int Strat::LayerModel::nrLayers() const
{
    int ret = 0;
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
	ret += seqs_[iseq]->size();
    return ret;
}


Interval<float> Strat::LayerModel::zRange() const
{
    if ( isEmpty() )
	return Interval<float>( 0.f, 0.f );

    Interval<float> ret( seqs_.first()->zRange() );
    for ( int iseq=1; iseq<seqs_.size(); iseq++ )
	ret.include( seqs_[iseq]->zRange(), false );
    return ret;
}


void Strat::LayerModel::setEmpty()
{
    deepErase( seqs_ );
}


Strat::LayerSequence& Strat::LayerModel::addSequence()
{
    LayerSequence* newseq = new LayerSequence( &proprefs_ );
    seqs_ += newseq;
    return *newseq;
}


Strat::LayerSequence& Strat::LayerModel::addSequence(
				const Strat::LayerSequence& inpls )
{
    LayerSequence* newls = new LayerSequence( &proprefs_ );
    newls->setStartDepth( inpls.startDepth() );

    const PropertyRefSelection& inpprops = inpls.propertyRefs();
    for ( int ilay=0; ilay<inpls.size(); ilay++ )
    {
	const Layer& inplay = *inpls.layers()[ilay];
	Layer* newlay = new Layer( inplay.unitRef() );
	newlay->setThickness( inplay.thickness() );
	newlay->setContent( inplay.content() );
	for ( int iprop=1; iprop<proprefs_.size(); iprop++ )
	{
	    const int idxof = inpprops.indexOf( proprefs_[iprop] );
	    newlay->setValue( iprop,
			idxof < 0 ? mUdf(float) : inplay.value(idxof) );
	}
	newls->layers() += newlay;
    }

    newls->prepareUse();
    seqs_ += newls;
    return *newls;
}


void Strat::LayerModel::removeSequence( int seqidx )
{
    if ( seqidx >= 0 && seqidx < seqs_.size() )
	delete seqs_.removeSingle( seqidx );
}


void Strat::LayerModel::prepareUse() const
{
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
	seqs_[iseq]->prepareUse();
}


const Strat::RefTree& Strat::LayerModel::refTree() const
{
    return isEmpty() ? RT() : seqs_[0]->refTree();
}


bool Strat::LayerModel::read( od_istream& strm, bool loadinto, int addeach )
{
    if ( !loadinto )
	deepErase( seqs_ );

    BufferString word;
    strm.getWord( word, false );
    if ( word[0] != '#' || word[1] != 'M' )
	{ ErrMsg( "File needs to start with '#M'" ); return false; }

    int nrseqs, nrprops;
    strm >> nrprops >> nrseqs;
    if ( nrprops < 1 )
	{ ErrMsg( "No properties found in file" ); return false; }
    strm.skipLine();

    BufferString keyw;
    strm.getWord( keyw );
    const bool mathpreserve = keyw == "#MATH";
    if ( mathpreserve )
	{ strm.skipLine(); strm.skipWord(); }

    PropertyRefSelection newprops;
    for ( int iprop=0; iprop<nrprops; iprop++ )
    {
	if ( iprop )
	    strm.skipWord(); // skip "#P.."
	BufferString propnm;
	strm.getLine( propnm );
	if ( iprop != 0 )
	{
	    const PropertyRef* p = PROPS().find( propnm.buf() );
	    if ( !p )
	    {
		ErrMsg( BufferString("Property not found: ",propnm) );
		return false;
	    }
	    newprops += p;
	}
    }
    if ( !strm.isOK() )
	{ ErrMsg("No sequences found"); return false; }

    TypeSet<int> propidxs;
    if ( loadinto )
    {
	for ( int iprop=0; iprop<newprops.size(); iprop++ )
	    propidxs += proprefs_.indexOf( newprops[iprop] );
    }
    else
    {
	proprefs_ = newprops;
	for ( int iprop=0; iprop<proprefs_.size(); iprop++ )
	    propidxs += iprop;
    }

    const RefTree& rt = RT();
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	strm.skipUntil( 'S' ); // skip "#S.."
	BufferString linestr;
	strm.getLine( linestr );
	SeparString separlinestr( linestr.buf(), od_tab );
	LayerSequence* seq = new LayerSequence( &proprefs_ );
	int nrlays = separlinestr.getIValue( 1 );
	if ( separlinestr.size()>2 )
	{
	    float startdepth = separlinestr.getFValue( 2 );
	    seq->setStartDepth( startdepth );
	}
	if ( !strm.isOK() )
	    { ErrMsg("Error during read"); return false; }

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm.skipWord(); // skip "#L.."
	    if ( !strm.getWord(word,false) )
	    {
		ErrMsg( BufferString("Incomplete sequence found: ",iseq) );
		delete seq; seq = 0; break;
	    }
	    FileMultiString fms( word );
	    const UnitRef* ur = rt.find( fms[0] );
	    mDynamicCastGet(const LeafUnitRef*,lur,ur)
	    Layer* newlay = new Layer( lur ? *lur : rt.undefLeaf() );
	    if ( fms.size() > 1 )
	    {
		const Content* c = rt.contents().getByName(fms[1]);
		newlay->setContent( c ? *c : Content::unspecified() );
	    }
	    float val; strm >> val;
	    newlay->setThickness( val );
	    if ( !mathpreserve )
	    {
		for ( int iprop=1; iprop<nrprops; iprop++ )
		{
		    strm >> val;
		    const auto propidx = propidxs[iprop];
		    if ( propidx >= 0 )
			newlay->setValue( propidx, val );
		}
		strm.skipLine();
	    }
	    else
	    {
		BufferString txt;
		for ( int iprop=1; iprop<nrprops; iprop++ )
		{
		    strm >> txt;
		    const auto propidx = propidxs[iprop];
		    if ( propidx < 0 )
			continue;
		    if ( txt.isNumber() )
			newlay->setValue( propidx, toFloat(txt) );
		    else
		    {
			IOPar iop; iop.getFrom( txt );
			newlay->setValue( propidx, iop, proprefs_ );
		    }
		}
	    }
	    seq->layers() += newlay;
	}
	if ( !seq )
	    break;
	else if ( iseq % addeach )
	    delete seq;
	else
	{
	    seq->prepareUse();
	    seqs_ += seq;
	}
    }
    return true;
}


bool Strat::LayerModel::write( od_ostream& strm, int modnr,
					bool mathpreserve ) const
{
    const int nrseqs = seqs_.size();
    const int nrprops = proprefs_.size();
    strm << "#M" << modnr << od_tab << nrprops << od_tab << nrseqs << od_endl;

    if ( mathpreserve )
	strm << "#MATH PRESERVED" << od_endl;

    for ( int iprop=0; iprop<nrprops; iprop++ )
	strm << "#P" << iprop << od_tab << proprefs_[iprop]->name() << od_endl;

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const LayerSequence& seq = *seqs_[iseq];
	const int nrlays = seq.size();
	strm << "#S" << iseq << od_tab << nrlays
		     << od_tab << seq.startDepth() <<od_endl;

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm << "#L" << ilay << od_tab;
	    const Layer& lay = *seq.layers()[ilay];
	    if ( lay.content().isUnspecified() )
		strm << '\'' << lay.name() << '\'';
	    else
	    {
		FileMultiString fms( lay.name() );
		fms += lay.content().name();
		strm << '\'' << fms << '\'';
	    }
	    strm << od_tab << toString(lay.thickness());
	    for ( int iprop=1; iprop<nrprops; iprop++ )
	    {
		if ( !mathpreserve || !lay.isMath(iprop) )
		    strm << od_tab << toString(lay.value(iprop));
		else
		{
		    const LayerValue& lv = *lay.getLayerValue( iprop );
		    strm << "\t'" << lv.dumpStr() << '\'';
		}
	    }
	    strm << od_endl;
	}
    }
    return strm.isOK();
}


void Strat::LayerModel::setElasticPropSel( const ElasticPropSelection& elp )
{
    elasticpropsel_ = elp;
}


//------ LayerModelSuite ------

Strat::LayerModelSuite::LayerModelSuite()
    : curChanged(this)
    , editingChanged(this)
{
    addModel( "", uiString::empty() );
}


void Strat::LayerModelSuite::setEmpty()
{
    baseModel().setEmpty();
    clearEditedData();
}


void Strat::LayerModelSuite::setBaseModel( LayerModel* newmod )
{
    baseModel().setEmpty();
    if ( newmod )
	mdls_.replace( 0, newmod );
    clearEditedData();
}


void Strat::LayerModelSuite::setCurIdx( idx_type idx )
{
    if ( mdls_.validIdx(idx) && idx != curidx_ )
    {
	curidx_ = idx;
	curChanged.trigger();
    }
}


void Strat::LayerModelSuite::setDesc( int imdl, const char* dsc,
				      const uiString& uidsc )
{
    if ( mdls_.validIdx(imdl) )
    {
	descs_.get( imdl ) = dsc;
	uidescs_.get( imdl ) = uidsc;
    }
}


void Strat::LayerModelSuite::addModel( const char* dsc, const uiString& uidsc )
{
    mdls_ += new LayerModel;
    descs_.add( dsc );
    uidescs_.add( uidsc );
}


void Strat::LayerModelSuite::removeModel( idx_type idx )
{
    if ( idx < 1 )
	{ pErrMsg("attempt to remove base model"); return; }

    const bool haded = hasEditedData();

    mdls_.removeSingle( idx );
    descs_.removeSingle( idx );
    uidescs_.removeSingle( idx );

    const bool removediscur = idx == curidx_;
    if ( removediscur )
	curidx_--;

    if ( hasEditedData() != haded )
	editingChanged.trigger( haded );

    if ( removediscur )
	curChanged.trigger();
}


bool Strat::LayerModelSuite::hasEditedData() const
{
    return size() > 1 && !mdls_.get(1)->isEmpty();
}


void Strat::LayerModelSuite::clearEditedData()
{
    if ( !hasEditedData() )
	return;

    for ( int imdl=1; imdl<size(); imdl++ )
	mdls_.get( imdl )->setEmpty();

    editingChanged.trigger( true );
}


void Strat::LayerModelSuite::prepareEditing()
{
    if ( size() < 2 )
	return;

    const bool haded = hasEditedData();

    for ( int imdl=1; imdl<size(); imdl++ )
	*mdls_.get( imdl ) = baseModel();

    editingChanged.trigger( haded );
}
