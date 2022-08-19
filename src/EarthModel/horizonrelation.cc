/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonrelation.h"
#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"
#include "timefun.h"

namespace EM
{

const char* RelationTree::Node::sKeyChildIDs()	{ return "Child IDs"; }
const char* RelationTree::Node::sKeyLastModified()
{ return "Last Modified"; }



RelationTree::Node::Node( const MultiID& id )
    : id_(id)
{
}


bool RelationTree::Node::hasChild( const RelationTree::Node* descendant ) const
{
    ObjectSet<const RelationTree::Node> nodes = children_;
    if ( nodes.isPresent(descendant) )
	return true;

    for ( int idx=0; idx<nodes.size(); idx++ )
    {
	for ( int idy=0; idy<nodes[idx]->children_.size(); idy++ )
	{
	    if ( nodes[idx]->children_[idy] == descendant )
		return true;

	    nodes.addIfNew( nodes[idx]->children_[idy] );
	}
    }

    return false;
}


void RelationTree::Node::fillPar( IOPar& par ) const
{
    TypeSet<MultiID> childids;
    for ( int idx=0; idx<children_.size(); idx++ )
	childids.addIfNew( children_[idx]->id_ );

    par.set( sKey::ID(), id_ );
    par.set( sKeyChildIDs(), childids );
    par.set( sKeyLastModified(), datestamp_ );
}


void RelationTree::Node::fillChildren( const FileMultiString& fms,
				 const RelationTree& tree )
{
    children_.erase();
    for ( int idx=0; idx<fms.size(); idx++ )
    {
	MultiID id( fms[idx].buf() );
	const RelationTree::Node* node = tree.getNode( id );
	if ( !node )
	    continue;

	if ( !node->hasChild(this) )
	    children_ += node;
    }
}



const char* RelationTree::sKeyHorizonRelations()
{ return "Horizon Relations"; }

RelationTree::RelationTree( bool is2d, bool doread )
    : is2d_(is2d)
{
    if ( doread )
	read();
}


RelationTree::~RelationTree()
{
    deepErase( nodes_ );
}


int RelationTree::findNode( const MultiID& id ) const
{
    for ( int idx=0; idx<nodes_.size(); idx++ )
    {
	if ( nodes_[idx]->id_ == id )
	    return idx;
    }

    return -1;
}


const RelationTree::Node* RelationTree::getNode( const MultiID& id ) const
{
    const int idx = findNode( id );
    return idx < 0 ? 0 : nodes_[idx];
}

void RelationTree::getParents( int index, TypeSet<int>& parents ) const
{
    parents.erase();
    if ( !nodes_.validIdx(index) )
	return;

    const RelationTree::Node* node = nodes_[index];
    for ( int idx=0; idx<nodes_.size(); idx++ )
    {
	if ( nodes_[idx]->children_.isPresent(node) )
	    parents += idx;
    }
}


int RelationTree::findRelation( const MultiID& id1, const MultiID& id2	) const
{
    const RelationTree::Node* node1 = getNode( id1 );
    const RelationTree::Node* node2 = getNode( id2 );
    if ( !node1 || !node2 )
	return 0;

    if ( node1->hasChild(node2) )
	return 1;
    if ( node2->hasChild(node1) )
	return 2;

    return 0;
}


void RelationTree::removeNode( const MultiID& id, bool dowrite )
{
    const int index = findNode( id );
    if ( index < 0 )
	return;

    RelationTree::Node* node = nodes_[index];
    TypeSet<int> parents;
    getParents( index, parents );
    for ( int idx=0; idx<parents.size(); idx++ )
    {
	RelationTree::Node* parentnode = nodes_[parents[idx]];
	parentnode->children_.removeSingle(
		parentnode->children_.indexOf(node) );
	for ( int cdx=0; cdx<node->children_.size(); cdx++ )
	{
	    const RelationTree::Node* childnode = node->children_[cdx];
	    if ( !parentnode->hasChild(childnode) &&
		 !childnode->hasChild(parentnode) )
		parentnode->children_ += childnode;
	}
    }

    delete nodes_.removeSingle( index );
    if ( dowrite )
	write();
}


static RelationTree::Node* createNewNode( const MultiID& id )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return 0;

    const char* fnm = ioobj->fullUserExpr( true );
    if ( !fnm || !*fnm || !File::exists(fnm) )
	return 0;

    RelationTree::Node* node = new RelationTree::Node( id );
    node->datestamp_ = File::timeLastModified( fnm );
    return node;
}


void RelationTree::addRelation( const MultiID& id1, const MultiID& id2,
				bool dowrite )
{
    const int idx1 = findNode( id1 );
    RelationTree::Node* node1 = idx1 < 0 ? 0 : nodes_[idx1];
    if ( idx1 < 0 )
    {
	node1 = createNewNode( id1 );
	if ( !node1 )
	    return;

	nodes_ += node1;
    }

    const int idx2 = findNode( id2 );
    RelationTree::Node* node2 = idx2 < 0 ? 0 : nodes_[idx2];
    if ( idx2 < 0 )
    {
	node2 = createNewNode( id2 );
	if ( !node2 )
	    return;

	nodes_ += node2;
    }

    if ( !node1->hasChild(node2) && !node2->hasChild(node1) )
    {
	node1->children_ += node2;
	if ( dowrite )
	    write();
    }
}


bool RelationTree::write() const
{
    IOPar par;
    const FilePath fp( IOObjContext::getDataDirName(IOObjContext::Surf),
		       "horizonrelations.txt" );
    if ( par.read(fp.fullPath(),sKeyHorizonRelations()) )
	par.removeSubSelection( is2d_ ? "Horizon2D" : "Horizon3D" );

    IOPar subpar;
    for ( int idx=0; idx<nodes_.size(); idx++ )
    {
	IOPar nodepar;
	nodes_[idx]->fillPar( nodepar );
	subpar.mergeComp( nodepar, toString(idx) );
    }

    par.mergeComp( subpar, is2d_ ? "Horizon2D" : "Horizon3D" );
    return par.write( fp.fullPath(), sKeyHorizonRelations() );
}


static bool hasBeenModified( const IOObj& ioobj, const char* datestamp )
{
    const char* fnm = ioobj.fullUserExpr( true );
    if ( !fnm || !*fnm || !File::exists(fnm) )
	return false;

    const char* moddate = File::timeLastModified( fnm );
    return Time::isEarlier( datestamp, moddate );
}


bool RelationTree::read( bool removeoutdated )
{
    deepErase( nodes_ );
    IOPar par;
    const FilePath fp( IOObjContext::getDataDirName(IOObjContext::Surf),
		       "horizonrelations.txt" );
    if ( !par.read(fp.fullPath(),sKeyHorizonRelations()) )
	return false;

    PtrMan<IOPar> subpar = par.subselect( is2d_ ? "Horizon2D" : "Horizon3D" );
    if ( !subpar )
	return false;

    for ( int idx=0; idx<1000000; idx++ )
    {
	MultiID id;
	PtrMan<IOPar> nodepar = subpar->subselect( idx );
	if ( !nodepar || !nodepar->get(sKey::ID(),id) )
	    break;

	RelationTree::Node* node = new RelationTree::Node( id );
	nodes_ += node;
    }

    TypeSet<MultiID> outdatednodes;
    IOObjContext ctxt( EMAnyHorizonTranslatorGroup::ioContext() );
    const IODir surfiodir( ctxt.getSelKey() );
    for ( int idx=0; idx<nodes_.size(); idx++ )
    {
	FileMultiString fms;
	PtrMan<IOPar> nodepar = subpar->subselect( idx );
	if ( !nodepar || !nodepar->get(RelationTree::Node::sKeyChildIDs(),fms) )
	    continue;

	RelationTree::Node* node = nodes_[idx];
	node->fillChildren( fms, *this );
	if ( !nodepar->get(RelationTree::Node::sKeyLastModified(),
			   node->datestamp_) )
	    continue;

	const IOObj* ioobj = surfiodir.get( node->id_ );
	if ( removeoutdated &&
		( !ioobj || hasBeenModified(*ioobj,node->datestamp_.buf()) ) )
	    outdatednodes += node->id_;
    }

    for ( int idx=0; idx<outdatednodes.size(); idx++ )
	removeNode( outdatednodes[idx], false );

    return true;
}


bool RelationTree::getSorted( const TypeSet<MultiID>& unsortedids,
			      TypeSet<MultiID>& sortedids ) const
{
    for ( int idx=0; idx<unsortedids.size(); idx++ )
    {
	const MultiID& mid = unsortedids[idx];
	if ( !sortedids.size() )
	{
	    if ( getNode(mid) )
		sortedids += mid;

	    continue;
	}

	for ( int idy=0; idy<sortedids.size(); idy++ )
	{
	    const MultiID& sortedmid = sortedids[idy];
	    const int rel = findRelation( mid, sortedmid );
	    if ( !rel )
		break;
	    else if ( rel == 1 )
	    {
		sortedids.insert( idy, mid );
		break;
	    }
	    else if ( idy == sortedids.size() - 1 )
	    {
		sortedids += mid;
		break;
	    }
	}
    }

    return sortedids.size() > 1;
}


// static functions
bool RelationTree::clear( bool is2d, bool dowrite )
{
    RelationTree reltree( is2d );
    deepErase( reltree.nodes_ );
    return dowrite ? reltree.write() : reltree.nodes_.isEmpty();
}


bool RelationTree::sortHorizons( bool is2d, const TypeSet<MultiID>& unsortedids,
				 TypeSet<MultiID>& sortedids )
{
    RelationTree reltree( is2d, false );
    reltree.read( false );
    return reltree.getSorted( unsortedids, sortedids );
}


bool RelationTree::update( bool is2d, const TypeSet<MultiID>& ids )
{
    RelationTree reltree( is2d );
    for ( int idx=1; idx<ids.size(); idx++ )
	reltree.addRelation( ids[idx-1], ids[idx], false );

    return reltree.write();
}


bool RelationTree::getSorted( bool is2d, TypeSet<MultiID>& mids )
{
    RelationTree reltree( is2d, false );
    reltree.read( false );
    TypeSet<MultiID> allmids;
    for ( int idx=0; idx<reltree.nodes_.size(); idx++ )
	allmids += reltree.nodes_[idx]->id_;

    return sortHorizons( is2d, allmids, mids );
}


bool RelationTree::getSorted( bool is2d, BufferStringSet& nms )
{
    TypeSet<MultiID> mids;
    if ( !getSorted(is2d,mids) )
	return false;

    for ( int idx=0; idx<mids.size(); idx++ )
	nms.add( IOM().nameOf(mids[idx]) );

    return nms.size();
}

} // namespace EM
