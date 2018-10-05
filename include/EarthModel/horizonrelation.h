#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/


#include "emcommon.h"
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
				Node(const DBKey&);

	DBKey			id_;
	BufferString		datestamp_;
	ObjectSet<const Node>	children_;

	bool			hasChild(const Node* descendant) const;

	void			fillPar(IOPar&) const;
	void			fillChildren(const FileMultiString&,
						 const RelationTree&);

	static const char*	sKeyChildIDs();
	static const char*	sKeyLastModified();
    };


				RelationTree(bool is2d,bool read=true);
				~RelationTree();

    const RelationTree::Node*	getNode(const DBKey&) const;
    void			getParents(int,TypeSet<int>&) const;
    void			removeNode(const DBKey&,bool write=true);
    void			addRelation(const DBKey& id1,
					    const DBKey& id2,bool write=true);
				// id1 is above id2

    int				findRelation(const DBKey&,
					     const DBKey&) const;
				/* 0 -> no relation
				   1 -> first horizon is at the top
				   2 -> second horizon is at the top */
    bool			getSorted(const DBKeySet& unsortedids,
					  DBKeySet& sortedids ) const;

    static bool			clear(bool is2d,bool dowrite=true);
    static bool			sortHorizons(bool is2d,
				     const DBKeySet& unsortedids,
				     DBKeySet& sortedids);
				/*!< Much faster if you only need RelationTree
				     for sorting and unsortedids is a small
				     subset of all horizons in the survey. */
    static bool			getSorted(bool is2d,DBKeySet&);
    static bool			getSorted(bool is2d,BufferStringSet&);
    static bool			update(bool id2d,const DBKeySet& sortedids);

    bool			read()			{ return read(true); }
    bool			write() const;

protected:

    ObjectSet<RelationTree::Node>	nodes_;
    bool			is2d_;

    int				findNode(const DBKey&) const;
    bool			read(bool removeoutdated);

    static const char*		sKeyHorizonRelations();
};

} // namespace EM
