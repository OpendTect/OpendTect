#ifndef horizonrelation_h
#define horizonrelation_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id: horizonrelation.h,v 1.1 2010-08-26 06:41:45 cvsraman Exp $
________________________________________________________________________

-*/


#include "multiid.h"
#include "objectset.h"

class FileMultiString;
class IOPar;
template <class T> class TypeSet;


mClass RelationTree
{
public:

    mClass Node
    {
    public:
				Node(const MultiID&);

	MultiID			id_;
	ObjectSet<const Node>	children_;

	bool			hasChild(const Node*) const;

	void			fillPar(IOPar&) const;
	void			fillChildren(const FileMultiString&i,
						 const RelationTree&);

    };


    				RelationTree(bool is2d);
				~RelationTree();

    const RelationTree::Node*	getNode(const MultiID&) const;
    void			getParents(int,TypeSet<int>&) const;
    void			removeNode(const MultiID&);
    void			addRelation(const MultiID&,const MultiID&);
    				// former is above the latter

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
};


#endif
