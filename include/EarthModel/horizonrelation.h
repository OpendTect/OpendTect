#ifndef horizonrelation_h
#define horizonrelation_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id$
________________________________________________________________________

-*/


#include "earthmodelmod.h"
#include "multiid.h"
#include "objectset.h"

class FileMultiString;
class IOPar;
template <class T> class TypeSet;

namespace EM
{

/*!
\ingroup EarthModel
\brief A Relation tree where a parent-child relationship means a top-bottom
relationship for the horizons.
*/

mClass(EarthModel) RelationTree
{
public:

    mClass(EarthModel) Node
    {
    public:
				Node(const MultiID&);

	MultiID			id_;
	ObjectSet<const Node>	children_;

	bool			hasChild(const Node*) const;

	void			fillPar(IOPar&) const;
	void			fillChildren(const FileMultiString&i,
						 const RelationTree&);

	static const char*	sKeyChildIDs();
	static const char*	sKeyLastModified();
    };


    				RelationTree(bool is2d,bool read=true);
				~RelationTree();

    const RelationTree::Node*	getNode(const MultiID&) const;
    void			getParents(int,TypeSet<int>&) const;
    void			removeNode(const MultiID&,bool write=true);
    void			addRelation(const MultiID& id1,
	    				    const MultiID& id2,bool write=true);
    				// id1 is above id2

    int				findRelation(const MultiID&,
	    				     const MultiID&) const;
    				/* 0 -> no relation
    				   1 -> first horizon is at the top
				   2 -> second horizon is at the top */
    bool			getSorted(const TypeSet<MultiID>& unsortedids,
	    				  TypeSet<MultiID>& sortedids ) const;

    bool			read();
    bool			write() const;

protected:
	
    ObjectSet<RelationTree::Node>	nodes_;
    bool			is2d_;

    int				findNode(const MultiID&) const;

    static const char*		sKeyHorizonRelations();
};

} // namespace EM

#endif

