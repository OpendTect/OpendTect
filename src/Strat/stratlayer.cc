/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "property.h"
#include "separstr.h"
#include "strmoper.h"
#include "keystrs.h"
#include "elasticpropsel.h"


const PropertyRef& Strat::Layer::thicknessRef()
{
    return PropertyRef::thickness();
}


Strat::Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
    , content_(0)
{
    setValue( 0, 0 ); // layers always have a thickness
}


BufferString Strat::Layer::name() const
{
    return BufferString( unitRef().fullCode().buf() );
}


Color Strat::Layer::dispColor( bool lith ) const
{
    return unitRef().dispColor( lith );
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
    return ival < vals_.size() ? vals_[ival] : mUdf(float);
}


void Strat::Layer::setValue( int ival, float val )
{
    while ( vals_.size() <= ival )
	vals_ += mUdf(float);
    vals_[ival] = val;
}


const Strat::Lithology& Strat::Layer::lithology() const
{
    return unitRef().getLithology();
}


const Strat::Content& Strat::Layer::content() const
{
    return content_ ? *content_ : Content::unspecified();
}


Strat::LayerSequence::LayerSequence( const PropertyRefSelection* prs )
    : z0_(0)
{
    if ( prs ) props_ = *prs;
}


Strat::LayerSequence::~LayerSequence()
{
    deepErase( layers_ );
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
    if ( !lvlunit ) return -1;

    for ( int ilay=startat; ilay<size(); ilay++ )
    {
	const LeafUnitRef& lur = layers_[ilay]->unitRef();
	if ( lur.upNode() == lvlunit )
	    return ilay;
    }
    return -1;
}


float Strat::LayerSequence::depthOf( const Strat::Level& lvl ) const
{
    const int sz = size();
    if ( sz < 1 ) return 0;
    const int idx = indexOf( lvl, 0 );
    return idx < 0 ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


int Strat::LayerSequence::positionOf( const Strat::Level& lvl ) const
{
    const RefTree& rt = refTree();
    Strat::UnitRefIter it( rt, Strat::UnitRefIter::LeavedNodes );
    ObjectSet<const Strat::UnitRef> unlist; BoolTypeSet isabove;
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


float Strat::LayerSequence::depthPositionOf( const Strat::Level& lvl ) const
{
    const int sz = size();
    if ( sz < 1 ) return 0;
    const int idx = positionOf( lvl );
    if ( idx < 0 ) return 0;
    return idx >= sz ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


void Strat::LayerSequence::getLayersFor( const UnitRef* ur,
					 ObjectSet<const Layer>& lys ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    if ( !ur ) ur = &refTree();

    for ( int idx=0; idx<sz; idx++ )
    {
	const Layer* ly = layers_[idx];
	if ( ur == &ly->unitRef() || ur->isParentOf(ly->unitRef()) )
	    lys += ly;
    }
}


void Strat::LayerSequence::getSequencePart( const Interval<float>& depthrg,
					    bool cropfirstlast,
					    LayerSequence& seq ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    if ( depthrg.isUdf() ) return;

    Interval<int> laysidx( layerIdxAtZ( depthrg.start ), 
	    		   layerIdxAtZ( depthrg.stop ) );

    if ( laysidx.isUdf() ) return;
    if ( laysidx.start == -1 )
	laysidx.start = 0;

    if ( laysidx.stop == -1 )
	laysidx.stop = sz - 1;

    const Layer* firstlay = layers_[laysidx.start];
    if ( firstlay )
    {
	Layer* croppedlay = new Layer( *firstlay );
	const float ztop = firstlay->zTop() > depthrg.start
			 ? firstlay->zTop() : depthrg.start;
	if ( cropfirstlast && croppedlay )
	{
	    const float zbase = firstlay->zBot() < depthrg.stop
			      ? firstlay->zBot() : depthrg.stop;
	    croppedlay->setThickness( zbase - ztop );
	}
	seq.layers() += croppedlay;
	seq.z0_ = ztop;
    }

    for ( int ilay=laysidx.start+1; ilay<laysidx.stop; ilay++ )
    {
	Layer* lay = new Layer( *layers_[ilay] );
	if ( lay ) seq.layers() += lay;
    }
    if ( laysidx.stop == laysidx.start )
	return;

    const Layer* lastlay = layers_[laysidx.stop];
    if ( lastlay )
    {
	Layer* croppedlay = new Layer( *lastlay );
	if ( cropfirstlast && croppedlay )
	{
	    const float ztop = lastlay->zTop() > depthrg.start
			     ? lastlay->zTop() : depthrg.start;
	    const float zbase = lastlay->zBot() < depthrg.stop
			      ? lastlay->zBot() : depthrg.stop;
	    croppedlay->setThickness( zbase - ztop );
	}

	seq.layers() += croppedlay;
    }

    seq.prepareUse();
}


void Strat::LayerSequence::prepareUse() const
{
    float z = z0_;
    for ( int idx=0; idx<size(); idx++ )
    {
	Layer& ly = *const_cast<Layer*>( layers_[idx] );
	ly.setZTop( z );
	z += ly.thickness();
    }
}


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
    setEmpty();
    if ( this != &oth )
    {
	proprefs_ = oth.proprefs_;
	for ( int iseq=0; iseq<oth.seqs_.size(); iseq++ )
	{
	    LayerSequence* newseq = new LayerSequence( *oth.seqs_[iseq] );
	    newseq->propertyRefs() = proprefs_;
	    seqs_ += newseq;
	}
    }
    return *this;
}


Interval<float> Strat::LayerModel::zRange() const
{
    if ( isEmpty() )
	return Interval<float>( 0, 0 );

    Interval<float> ret( seqs_[0]->zRange() );
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

    const PropertyRefSelection& inpprops = inpls.propertyRefs();
    for ( int ilay=0; ilay<inpls.size(); ilay++ )
    {
	const Layer& inplay = *inpls.layers()[ilay];
	Layer* newlay = new Layer( inplay.unitRef() );
	newlay->setThickness( inplay.thickness() );
	for ( int iprop=1; iprop<proprefs_.size(); iprop++ )
	{
	    const int idxof = inpprops.indexOf( proprefs_[iprop] );
	    newlay->setValue( iprop,
		    	idxof < 0 ? mUdf(float) : inplay.value(idxof) );
	}
	newls->layers() += newlay;
    }

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
    for ( int idx=0; idx<seqs_.size(); idx++ )
	seqs_[idx]->prepareUse();
}


const Strat::RefTree& Strat::LayerModel::refTree() const
{
    return isEmpty() ? RT() : seqs_[0]->refTree();
}



bool Strat::LayerModel::read( std::istream& strm )
{
    deepErase( seqs_ );
    char buf[256];
    StrmOper::wordFromLine( strm, buf, 256 );
    if ( buf[0] != '#' || buf[1] != 'M' )
	return false;

    int nrseqs, nrprops;
    strm >> nrprops >> nrseqs;
    if ( nrprops < 1 ) return false;
    StrmOper::wordFromLine( strm, buf, 256 ); // read newline

    PropertyRefSelection newprops;
    for ( int iprop=0; iprop<nrprops; iprop++ )
    {
	StrmOper::wordFromLine( strm, buf, 256 ); // read "#P.."
	BufferString propnm;
	StrmOper::readLine( strm, &propnm );
	if ( iprop != 0 )
	{
	    const PropertyRef* p = PROPS().find( propnm.buf() );
	    if ( !p ) return false;
	    newprops += p;
	}
    }
    if ( !strm.good() ) return false;

    proprefs_ = newprops;
    const RefTree& rt = RT();

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	StrmOper::wordFromLine( strm, buf, 256 ); // read away "#S.."
	LayerSequence* seq = new LayerSequence( &proprefs_ );
	int nrlays; strm >> nrlays;
	StrmOper::wordFromLine( strm, buf, 256 ); // read newline
	if ( strm.bad() ) return false;

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    StrmOper::wordFromLine( strm, buf, 256 ); // read away "#L.."
	    StrmOper::wordFromLine( strm, buf, 256 );
	    FileMultiString fms( buf );
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
	    for ( int iprop=1; iprop<nrprops; iprop++ )
		{ strm >> val; newlay->setValue( iprop, val ); }
	    seq->layers() += newlay;
	    StrmOper::wordFromLine( strm, buf, 256 ); // read newline
	}
	seq->prepareUse();
	seqs_ += seq;
    }
    return true;
}


bool Strat::LayerModel::write( std::ostream& strm, int modnr ) const
{
    const int nrseqs = seqs_.size();
    const int nrprops = proprefs_.size();
    strm << "#M" << modnr << '\t' << nrprops << '\t' << nrseqs << '\n';

    for ( int iprop=0; iprop<nrprops; iprop++ )
	strm << "#P" << iprop << '\t' << proprefs_[iprop]->name() << '\n';

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const LayerSequence& seq = *seqs_[iseq];
	const int nrlays = seq.size();
	strm << "#S" << iseq << '\t' << nrlays << '\n';

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm << "#L" << ilay << '\t';
	    const Layer& lay = *seq.layers()[ilay];
	    if ( lay.content().isUnspecified() )
		strm << lay.name();
	    else
	    {
		FileMultiString fms( lay.name() );
		fms += lay.content().name();
		strm << fms;
	    }
	    strm << '\t' << toString(lay.thickness());
	    for ( int iprop=1; iprop<nrprops; iprop++ )
		strm << '\t' << toString(lay.value(iprop));
	    strm << std::endl;
	}
    }
    return strm.good();
}


void Strat::LayerModel::setElasticPropSel( const ElasticPropSelection& elp )
{
    elasticpropsel_ = elp;
}
