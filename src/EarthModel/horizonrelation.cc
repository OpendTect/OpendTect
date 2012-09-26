/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "horizonrelation.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
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
{}


bool RelationTree::Node::hasChild( const RelationTree::Node* node ) const
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx] == node || children_[idx]->hasChild(node) )
	    return true;
    }

    return false;
}


void RelationTree::Node::fillPar( IOPar& par ) const
{
    BufferStringSet childids;
    for ( int idx=0; idx<children_.size(); idx++ )
	childids.addIfNew( children_[idx]->id_.buf() );

    par.set( sKey::ID(), id_ );
    par.set( sKeyChildIDs(), childids );
    PtrMan<IOObj> ioobj = IOM().get( id_ );
    if ( !ioobj )
	return;

    const char* fnm = ioobj->fullUserExpr( true );
    if ( !fnm || !*fnm || !File::exists(fnm) )
	return;

    par.set( sKeyLastModified(), File::timeLastModified(fnm) );
}


void RelationTree::Node::fillChildren( const FileMultiString& fms,
       				 const RelationTree& tree )
{
    children_.erase();
    for ( int idx=0; idx<fms.size(); idx++ )
    {
	MultiID id( fms[idx] );
	const RelationTree::Node* node = tree.getNode( id );
	if ( !node )
	    continue;

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
{ deepErase( nodes_ ); }


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
	if ( nodes_[idx]->children_.indexOf(node) >= 0 )
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
	parentnode->children_.remove( parentnode->children_.indexOf(node) );
	for ( int cdx=0; cdx<node->children_.size(); cdx++ )
	{
	    const RelationTree::Node* childnode = node->children_[cdx];
	    if ( !parentnode->hasChild(childnode) )
		parentnode->children_ += childnode;
	}
    }

    delete nodes_.remove( index );
    if ( dowrite )
	write();
}


void RelationTree::addRelation( const MultiID& id1, const MultiID& id2,
				bool dowrite )
{
    const int idx1 = findNode( id1 );
    RelationTree::Node* node1 = idx1 < 0 ? 0 : nodes_[idx1];
    if ( idx1 < 0 )
    {
	node1 = new RelationTree::Node( id1 );
	nodes_ += node1;
    }

    const int idx2 = findNode( id2 );
    RelationTree::Node* node2 = idx2 < 0 ? 0 : nodes_[idx2];
    if ( idx2 < 0 )
    {
	node2 = new RelationTree::Node( id2 );
	nodes_ += node2;
    }

    if ( !node1->hasChild(node2) )
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
	par.removeWithKey( is2d_ ? "Horizon2D" : "Horizon3D" );

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


static bool hasBeenModified( const MultiID& id, const char* datestamp )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return false;

    const char* fnm = ioobj->fullUserExpr( true );
    if ( !fnm || !*fnm || !File::exists(fnm) )
	return false;

    const char* moddate = File::timeLastModified( fnm );
    return Time::isEarlier( datestamp, moddate );
}

bool RelationTree::read()
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

    for ( int idx=0; idx<1024; idx++ )
    {
	MultiID id;
	PtrMan<IOPar> nodepar = subpar->subselect( idx );
	if ( !nodepar || !nodepar->get(sKey::ID(),id) )
	    break;

	RelationTree::Node* node = new RelationTree::Node( id );
	nodes_ += node;
    }

    TypeSet<MultiID> outdatednodes;
    for ( int idx=0; idx<nodes_.size(); idx++ )
    {
	FileMultiString fms;
	PtrMan<IOPar> nodepar = subpar->subselect( idx );
	if ( !nodepar || !nodepar->get(RelationTree::Node::sKeyChildIDs(),fms) )
	    continue;

	RelationTree::Node* node = nodes_[idx];
	node->fillChildren( fms, *this );
	BufferString datestamp;
	if ( !nodepar->get(RelationTree::Node::sKeyLastModified(),datestamp) )
	    continue;

	if ( hasBeenModified(node->id_,datestamp.buf()) )
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

} // namespace EM
