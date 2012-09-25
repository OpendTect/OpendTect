/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID mUnusedVar = "$Id: stratlayer.cc 26361 2012-09-24 20:41:21Z helene.huck@dgbes.com $";

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
{
    setValue( 0, 0 ); setValue( 1, 0 );
}


BufferString Strat::Layer::name() const
{
    return BufferString( unitRef().fullCode().buf() );
}


const Strat::LeafUnitRef& Strat::Layer::unitRef() const
{
    return ref_ ? *ref_ : RT().undefLeaf();
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


int Strat::LayerSequence::layerIdxAtZ( float zreq, bool retszifafter ) const
{
    const ObjectSet<Layer>& lays = layers();
    const int nrlays = lays.size();
    if ( nrlays == 0 || zreq < lays[0]->zTop() )
	return -1;
    else if ( zreq > lays[nrlays-1]->zBot() )
	return retszifafter ? nrlays : -1;

    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	const Layer& lay( *lays[ilay] );
	if ( zreq < lay.zBot() )
	    return ilay;
    }
    return nrlays-1;
}


int Strat::LayerSequence::indexOf( const Strat::Level& lvl, int startat ) const
{
    const RefTree& rt = refTree();
    Strat::UnitRefIter it( rt, Strat::UnitRefIter::LeavedNodes );
    const Strat::LeavedUnitRef* lvlunit = 0;
    ObjectSet<const Strat::LeavedUnitRef> hits;
    while ( it.next() )
    {
	const Strat::LeavedUnitRef* un
	    	= static_cast<const Strat::LeavedUnitRef*>( it.unit() );
	if ( !lvlunit && un->levelID() == lvl.id() )
	    lvlunit = un;
	if ( lvlunit )
	    hits += un;
    }
    if ( !lvlunit ) return -1;

    for ( int ilay=startat; ilay<size(); ilay++ )
    {
	const LeafUnitRef& lur = layers_[ilay]->unitRef();
	if ( &lur == &rt.undefLeaf() )
	    continue;

	const Strat::LeavedUnitRef* leavedun
	    = static_cast<const Strat::LeavedUnitRef*>( lur.upNode() );
	if ( !leavedun )
	    continue;
	for ( int ihit=0; ihit<hits.size(); ihit++ )
	{
	    if ( leavedun == hits[ihit] )
		return ilay;
	}
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
    props_ += &Layer::thicknessRef();
}


Strat::LayerModel::~LayerModel()
{
    deepErase( seqs_ );
}


Strat::LayerModel& Strat::LayerModel::operator =( const Strat::LayerModel& oth )
{
    if ( this != &oth )
    {
	props_ = oth.props_;
	for ( int iseq=0; iseq<oth.seqs_.size(); iseq++ )
	{
	    LayerSequence* newseq = new LayerSequence( *oth.seqs_[iseq] );
	    newseq->propertyRefs() = props_;
	    seqs_ += newseq;
	}
    }
    return *this;
}


Strat::LayerSequence& Strat::LayerModel::addSequence()
{
    LayerSequence* newseq = new LayerSequence( &props_ );
    seqs_ += newseq;
    return *newseq;
}


void Strat::LayerModel::setEmpty()
{
    deepErase( seqs_ );
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

    PropertyRefSelection newprops;
    newprops += &PropertyRef::thickness(); // get current survey's thickness
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

    props_ = newprops;
    const RefTree& rt = RT();

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	StrmOper::wordFromLine( strm, buf, 256 ); // read away "#S"
	LayerSequence* seq = new LayerSequence( &props_ );
	int nrlays, seqnr;
	strm >> seqnr >> nrlays;
	if ( !strm.good() ) return false;

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    StrmOper::wordFromLine( strm, buf, 256 ); // read away "#L"
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
	}
	seqs_ += seq;
    }
    return true;
}


bool Strat::LayerModel::write( std::ostream& strm, int modnr ) const
{
    const int nrseqs = seqs_.size();
    const int nrprops = props_.size();
    strm << "#M" << modnr << '\t' << nrprops << '\t' << nrseqs << '\n';

    for ( int iprop=0; iprop<nrprops; iprop++ )
	strm << "#P" << iprop << '\t' << props_[iprop]->name() << '\n';

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const LayerSequence& seq = *seqs_[iseq];
	const int nrlays = seq.size();
	strm << "#S" << iseq << '\t' << nrlays << '\n';

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm << "#L" << ilay << '\t';
	    const Layer& lay = *seq.layers()[iseq];
	    if ( lay.content().isUnspecified() )
		strm << lay.name();
	    else
	    {
		FileMultiString fms( lay.name() );
		fms += lay.content().name();
		strm << fms;
	    }
	    strm << '\t' << lay.thickness();
	    for ( int iprop=1; iprop<nrprops; iprop++ )
		strm << '\t' << lay.value( iprop );
	    strm << std::endl;
	}
    }
    return strm.good();
}


void Strat::LayerModel::addElasticPropSel( const ElasticPropSelection& elp )
{
    elasticpropsel_ = elp;
}
