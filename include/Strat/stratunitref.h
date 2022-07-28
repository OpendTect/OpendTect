#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003 / Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"
#include "compoundkey.h"
#include "stratlevel.h"
#include "enums.h"
#include "iopar.h"


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

mExpClass(Strat) UnitRef : public CallBacker
{
public:

    enum Type		{ NodeOnly, Leaved, Leaf };
			mDeclareEnumUtils(Type)

			UnitRef(NodeUnitRef*,const char* d=0);
    virtual		~UnitRef();

    virtual Type	type() const		= 0;
    virtual bool	isEmpty() const		{ return false; }
    virtual bool	isUndef() const		{ return false; }
    virtual bool	hasChildren() const	= 0;
    bool		isLeaf() const		{ return type()==Leaf; }
    bool		isLeaved() const	{ return type()==Leaved; }
    CompoundKey		fullCode() const;
    CompoundKey		parentCode() const;

    virtual const OD::String& code() const	= 0;
    virtual void	setCode(const char*)	{}
    const OD::String&	description() const	{ return desc_; }
    virtual void	setDescription( const char* d ) { desc_ = d; }
    OD::Color		color() const		{ return color_; }
    void		setColor(OD::Color);
    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }

    NodeUnitRef*	upNode(int skip=0);
    const NodeUnitRef*	upNode( int skip=0 ) const
			{ return ((UnitRef*)this)->upNode( skip ); }
    void		setUpNode(NodeUnitRef* newpar)
			{ upnode_ = newpar; }
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

    BufferString	desc_;
    OD::Color		color_;
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

};


/*!\brief UnitRef for units containing other units only */

mExpClass(Strat) NodeUnitRef : public UnitRef
{
public:

			NodeUnitRef(NodeUnitRef*,const char*,const char* d=0);
			~NodeUnitRef();
    void		setEmpty();

    bool		isEmpty() const override
			    { return refs_.isEmpty(); }
    bool		hasChildren() const override
			    { return !refs_.isEmpty(); }
    virtual bool	hasLeaves() const	= 0;

    const OD::String&	code() const override	{ return code_; }
    void		setCode( const char* c ) override { code_ = c; }

    virtual Interval<float> timeRange() const	{ return timerg_; }
    virtual void	setTimeRange(const Interval<float>&);
    void		incTimeRange(const Interval<float>&);

    int			nrRefs() const		{ return refs_.size(); }
    UnitRef&		ref( int idx )		{ return *refs_[idx]; }
    const UnitRef&	ref( int idx ) const	{ return *refs_[idx]; }
    int			indexOf( const UnitRef* ur ) const
			{ return refs_.indexOf((const NodeUnitRef*)ur); }
    bool		isParentOf(const UnitRef&) const override;

    UnitRef*		find( const char* urcode )	{ return fnd(urcode); }
    const UnitRef*	find( const char* urcode ) const{ return fnd(urcode); }

    virtual int		nrLeaves() const;
    int			level() const override
			    { return upnode_?upnode_->level()+1:0; }

protected:

    ObjectSet<UnitRef>	refs_;
    Interval<float>	timerg_;
    BufferString	code_;

    UnitRef*		fnd(const char*) const;
    void		takeChildrenFrom(NodeUnitRef*);
    friend class	RefTree;

public:

    virtual bool	add(UnitRef*,bool rev=false);
    virtual bool	insert(UnitRef*,int posidx);
    virtual UnitRef*	replace(int uridx,UnitRef*);
    void		swapChildren(int,int);
    void		remove( int uridx )
			{ delete refs_.removeSingle(uridx); }
    void		remove( const UnitRef* ur )
			{ remove( indexOf( ur ) ); }

    void		getPropsFrom(const IOPar&) override;
    void		putPropsTo(IOPar&) const override;

protected:
    void		changeTimeRange(float dtime);

};


/*!\brief UnitRef for units containing non-Leaf units only */

mExpClass(Strat) NodeOnlyUnitRef : public NodeUnitRef
{
public:
			NodeOnlyUnitRef( NodeUnitRef* up, const char* c,
				     const char* d=0 )
			: NodeUnitRef(up,c,d)	{}

    bool		hasLeaves() const override	{ return false; }
    Type		type() const override		{ return NodeOnly; }
    const LeafUnitRef*	firstLeaf() const override;

};


/*!\brief UnitRef for units containing Leaf units only */

mExpClass(Strat) LeavedUnitRef : public NodeUnitRef
{
public:
			LeavedUnitRef( NodeUnitRef* up, const char* c,
				     const char* d=0 )
			: NodeUnitRef(up,c,d)
			, levelid_(-1)			{}

    Type		type() const override		{ return Leaved; }
    bool		hasLeaves() const override	{ return true; }

    LevelID		levelID() const		{ return levelid_; }
    void		setLevelID(LevelID);

    int			nrLeaves() const override	{ return refs_.size(); }
    const LeafUnitRef*	firstLeaf() const override
			{ return refs_.isEmpty() ? 0 : refs_[0]->firstLeaf(); }

    LeafUnitRef*	getLeaf(int);
    const LeafUnitRef*	getLeaf( int i ) const
			{ return const_cast<LeavedUnitRef*>(this)->getLeaf(i); }
    LeafUnitRef*	getLeaf(const Lithology&);
    const LeafUnitRef*	getLeaf( const Lithology& l ) const
			{ return const_cast<LeavedUnitRef*>(this)->getLeaf(l); }

protected:

    LevelID		levelid_;

    void		fill( BufferString& bs ) const override
			    { doFill(bs,levelid_.asInt()); }
    void		use( const char* s ) override
			{
			    int lvlid = levelid_.asInt();
			    doUse( s, &lvlid );
			}
};


/*!\brief UnitRef for layers */

mExpClass(Strat) LeafUnitRef : public UnitRef
{
public:

			LeafUnitRef(NodeUnitRef*,int lithid=-1,
				    const char* desc=0);
    bool		isUndef() const override;

    Type		type() const override		{ return Leaf; }
    bool		hasChildren() const override	{ return false; }
    const OD::String&	code() const override;
    int			lithology() const	{ return lith_; }
    void		setLithology(int);

    const Lithology&	getLithology() const;
    OD::Color		dispColor(bool lith_else_upnode) const;
    int			level() const override
			    { return upnode_?upnode_->level()+1:0; }
    const LeafUnitRef*	firstLeaf() const override { return this; }

protected:

    int			lith_; // Lithology::ID

    void		fill( BufferString& bs ) const override
			    { doFill(bs,lith_); }

    void		use( const char* s ) override	{ doUse(s,&lith_); }


public:

    void		getPropsFrom(const IOPar&) override;

};


inline NodeUnitRef* UnitRef::topNode()
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }
inline const NodeUnitRef* UnitRef::topNode() const
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }


}; // namespace Strat

