#ifndef stratunitref_h
#define stratunitref_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003 / Sep 2010
 RCS:		$Id$
________________________________________________________________________


-*/

#include "compoundkey.h"
#include "stratlevel.h"
#include "enums.h"
#include "iopar.h"


class IOPar;

namespace Strat
{

class RefTree;
class Lithology;
class NodeUnitRef;
class LeafUnitRef;

/*!\brief Reference data for a stratigraphic unit

  Every stratigraphy is a tree of units. A stratigraphy consists of reference
  units - every part of the subsurface can be attached to a reference unit.

 */

mClass UnitRef : public CallBacker
{
public:

    enum Type		{ NodeOnly, Leaved, Leaf };
    			DeclareEnumUtils(Type)

			UnitRef(NodeUnitRef*,const char* d=0);
    virtual		~UnitRef();

    virtual Type	type() const		= 0;
    virtual bool	isEmpty() const		{ return false; }
    virtual bool	hasChildren() const	= 0;
    bool		isLeaf() const		{ return type()==Leaf; }
    bool		isLeaved() const	{ return type()==Leaved; }
    CompoundKey		fullCode() const;

    virtual const BufferString&	code() const	= 0;
    virtual void	setCode(const char*)	{}
    const BufferString&	description() const	{ return desc_; }
    virtual void	setDescription( const char* d )	{ desc_ = d; }
    Color		color() const		{ return color_; }
    void		setColor(Color);
    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }

    NodeUnitRef*	upNode(int skip=0);
    const NodeUnitRef*	upNode( int skip=0 ) const
    			{ return ((UnitRef*)this)->upNode( skip ); }
    NodeUnitRef*	topNode();
    const NodeUnitRef*	topNode() const;
    RefTree&		refTree(); // is the topNode
    const RefTree&	refTree() const;
    virtual bool	isParentOf(const UnitRef&) const { return false; }
    virtual int		level() const			= 0;

    Notifier<UnitRef>	changed;
    Notifier<UnitRef>	toBeDeleted;

    virtual const LeafUnitRef*	firstLeaf() const	= 0;

protected:

    NodeUnitRef*	upnode_;

    BufferString    	desc_;
    Color		color_;
    IOPar		pars_;

    void		doFill(BufferString&,int) const;
    void		doUse(const char*,int*);
    void		notifChange(bool isrem=false);

    friend class	NodeUnitRef;

public:

    int			treeDepth() const;
    bool		isBelow(const UnitRef*) const;
    			//!< is given ref parent, grandparent, grandgrand... 
    bool		precedes(const UnitRef&) const;
    			//!< in terms of iterating through tree

    virtual void	fill( BufferString& bs ) const	{ doFill(bs,mUdf(int));}
    virtual void	use( const char* s )		{ doUse(s,0); }
    virtual void	getPropsFrom(const IOPar&);
    virtual void	putPropsTo(IOPar&) const;

    static const char*	sKeyPropsFor()		{ return "Properties for "; }
    static const char*	sKeyTreeProps()		{ return "entire tree"; }
    CompoundKey		parentCode() const;

};


/*!\brief UnitRef for units containing other units only */

mClass NodeUnitRef : public UnitRef
{
public:

			NodeUnitRef(NodeUnitRef*,const char*,const char* d=0);
			~NodeUnitRef();

    virtual bool	isEmpty() const		{ return refs_.isEmpty(); }
    virtual bool	hasChildren() const	{ return !refs_.isEmpty(); }
    virtual bool	hasLeaves() const	= 0;

    virtual const BufferString&	code() const	{ return code_; }
    virtual void	setCode( const char* c ) { code_ = c; }

    virtual Interval<float> timeRange() const	{ return timerg_; }
    virtual void	setTimeRange(const Interval<float>&);
    void		incTimeRange(const Interval<float>&);

    int			nrRefs() const		{ return refs_.size(); }
    UnitRef&		ref( int idx )		{ return *refs_[idx]; }
    const UnitRef&	ref( int idx ) const	{ return *refs_[idx]; }
    int			indexOf( const UnitRef* ur ) const
			{ return refs_.indexOf((const NodeUnitRef*)ur); }
    virtual bool	isParentOf(const UnitRef&) const;

    UnitRef*		find( const char* urcode )	{ return fnd(urcode); }
    const UnitRef*	find( const char* urcode ) const{ return fnd(urcode); }

    virtual int		nrLeaves() const;
    virtual int		level() const { return upnode_?upnode_->level()+1:0; }

protected:

    ObjectSet<UnitRef>	refs_;
    Interval<float>	timerg_;
    BufferString    	code_;

    UnitRef*		fnd(const char*) const;
    void		takeChildrenFrom(NodeUnitRef*);
    friend class	RefTree;

public:

    virtual bool	add(UnitRef*,bool rev=false);
    virtual bool	insert(UnitRef*,int posidx);
    virtual UnitRef*	replace(int uridx,UnitRef*);
    void		swapChildren(int,int);
    void		remove( int uridx ) 
    			{ delete refs_.remove(uridx); }
    void		remove( const UnitRef* ur )
    			{ remove( indexOf( ur ) ); }

    virtual void	getPropsFrom(const IOPar&);
    virtual void	putPropsTo(IOPar&) const;
    void                removeAllChildren()
			{ deepErase( refs_ ); }


};


/*!\brief UnitRef for units containing non-Leaf units only */

mClass NodeOnlyUnitRef : public NodeUnitRef
{
public:
			NodeOnlyUnitRef( NodeUnitRef* up, const char* c,
				     const char* d=0 )
			: NodeUnitRef(up,c,d)	{}

    virtual bool	hasLeaves() const	{ return false; }
    virtual Type	type() const		{ return NodeOnly; }
    virtual const LeafUnitRef*	firstLeaf() const;

};


/*!\brief UnitRef for units containing Leaf units only */

mClass LeavedUnitRef : public NodeUnitRef
{
public:
			LeavedUnitRef( NodeUnitRef* up, const char* c,
				     const char* d=0 )
			: NodeUnitRef(up,c,d)
			, levelid_(-1)			{}

    virtual Type	type() const		{ return Leaved; }
    virtual bool	hasLeaves() const	{ return true; }

    Level::ID		levelID() const		{ return levelid_; }
    void		setLevelID(Level::ID);

    virtual int		nrLeaves() const	{ return refs_.size(); }
    virtual const LeafUnitRef*	firstLeaf() const
			{ return refs_.isEmpty() ? 0 : refs_[0]->firstLeaf(); }

protected:

    Level::ID		levelid_;

    virtual void	fill( BufferString& bs ) const	{ doFill(bs,levelid_); }
    virtual void	use( const char* s )	{ doUse(s,&levelid_); }

};


/*!\brief UnitRef for layers */

mClass LeafUnitRef : public UnitRef
{
public:

			LeafUnitRef(NodeUnitRef*,int lithidx=-1,
				    const char* desc=0);

    virtual Type	type() const		{ return Leaf; }
    virtual bool	hasChildren() const	{ return false; }
    virtual const BufferString&	code() const;
    int			lithology() const	{ return lith_; }
    void		setLithology(int);

    const Lithology&	getLithology() const;
    Color		dispColor(bool lith_else_upnode) const;
    virtual int		level() const { return upnode_?upnode_->level()+1:0; }
    virtual const LeafUnitRef*	firstLeaf() const { return this; }

protected:

    int			lith_;

    virtual void	fill( BufferString& bs ) const	{ doFill(bs,lith_); }
    virtual void	use( const char* s )	{ doUse(s,&lith_); }


public:

    virtual void	getPropsFrom(const IOPar&);

};


inline NodeUnitRef* UnitRef::topNode()
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }
inline const NodeUnitRef* UnitRef::topNode() const
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }


}; // namespace Strat

#endif
