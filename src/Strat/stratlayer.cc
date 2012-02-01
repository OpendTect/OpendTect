/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID = "$Id: stratlayer.cc,v 1.33 2012-02-01 13:54:40 cvsbert Exp $";

#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "property.h"
#include "separstr.h"
#include "ascstream.h"
#include "keystrs.h"
#include "elasticpropsel.h"

static const char* sKeyLayModFileType = "Layer Model";


const PropertyRef& Strat::Layer::thicknessRef()
{
    return PropertyRef::thickness();
}


Strat::Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
{
    setValue( 0, 0 ); setValue( 1, 0 );
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


int Strat::LayerSequence::indexOf( const Strat::Level& lvl, int startat ) const
{
    bool unseen = false;
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
	for ( int iseq=0; iseq<oth.seqs_.size(); iseq++ )
	{
	    LayerSequence* newseq = new LayerSequence( *oth.seqs_[iseq] );
	    newseq->propertyRefs() = props_;
	    seqs_ += newseq;
	}
	props_ = oth.props_;
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


bool Strat::LayerModel::readHeader( std::istream& strm, IOPar& pars )
{
    pars.setEmpty();
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyLayModFileType) )
	return false;

    pars.set( sKey::Date, astrm.timeStamp() );
    pars.getFrom( astrm );
    return strm.good();
}


bool Strat::LayerModel::read( std::istream& strm, IOPar& pars )
{
    if ( !readHeader(strm,pars) )
	return false;

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    BufferStringSet nms; TypeSet<float> vals;
    for ( int idx=0; ; idx++ )
    {
	BufferString ky( sKey::Value ); ky.add( "." ).add( idx );
	const char* res = iop.find( ky );
	if ( res && *res )
	    { nms.add( res ); vals += 0; }
    }
    if ( nms.isEmpty() )
	return false;

    // TODO create props_ from nms
    // read data
    return false;
}


bool Strat::LayerModel::write( std::ostream& strm, const IOPar& pars ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyLayModFileType) )
	return false;

    pars.putTo( astrm );

    const int nrseqs = seqs_.size();
    const int nrprops = props_.size();
    IOPar iop; iop.set( sKeyNrSeqs(), nrseqs );
    for ( int idx=0; idx<nrprops; idx++ )
    {
	const PropertyRef& pref = *props_[idx];
	BufferString ky( sKey::Value ); ky.add( "." ).add( idx );
	FileMultiString fms( pref.name() );
	fms += pref.disp_.color_.getStdStr();
	fms += pref.disp_.unit_;
	iop.set( ky, fms );
    }
    iop.putTo( astrm );

    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
    {
	const LayerSequence& seq = *seqs_[iseq];
	for ( int ilay=0; ilay<seq.size(); ilay++ )
	{
	    const Layer& lay = *seq.layers()[iseq];
	    for ( int iprop=0; iprop<nrprops; iprop++ )
		strm << '\t' << lay.value( iprop );
	strm << '\n';
	}
	astrm.newParagraph();
    }
    return true;
}


void Strat::LayerModel::addElasticPropSel( const ElasticPropSelection& elp )
{
    elasticpropsel_ = elp;
}
