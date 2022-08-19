#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "multiid.h"
#include "objectset.h"

class FileMultiString;


namespace EM
{

/*!
\brief A Relation tree where a parent-child relationship means a top-bottom
relationship for the horizons.
*/

mExpClass(EarthModel) RelationTree
{
public:

    mExpClass(EarthModel) Node
    {
    public:
				Node(const MultiID&);

	MultiID			id_;
	ObjectSet<const Node>	children_;
	BufferString		datestamp_;

	bool			hasChild(const Node* descendant) const;

	void			fillPar(IOPar&) const;
	void			fillChildren(const FileMultiString&,
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

    bool			read()			{ return read(true); }
    bool			write() const;

protected:

    ObjectSet<RelationTree::Node>	nodes_;
    bool			is2d_;

    int				findNode(const MultiID&) const;

    static const char*		sKeyHorizonRelations();

    bool			read(bool removeoutdated);

public:
    static bool			clear(bool is2d,bool dowrite=true);
    static bool			sortHorizons(bool is2d,
					const TypeSet<MultiID>& unsortedids,
					TypeSet<MultiID>& sortedids);
				/*!< Much faster if you only need RelationTree
				  for sorting and unsortedids is a small
				  subset of all horizons in the survey. */

    static bool			getSorted(bool is2d,TypeSet<MultiID>&);
    static bool			getSorted(bool is2d,BufferStringSet&);
    static bool			update(bool id2d,
					const TypeSet<MultiID>& sortedids);
};

} // namespace EM
