#ifndef stratunitref_h
#define stratunitref_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitref.h,v 1.3 2004-11-29 17:04:26 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "compoundkey.h"

namespace Strat
{

class Lithology;


/*!\brief Reference data for a stratigraphic unit */

class UnitRef
{
public:

			UnitRef( UnitRef* up, const char* code,
			      const char* descr=0 )
			: code_(code)
			, upnode_(up)
			, desc_(descr)			{}

    virtual bool	isLeaf() const			= 0;
    CompoundKey		fullCode() const;

    const BufferString&	code() const			{ return code_; }
    void		setCode( const char* c )	{ code_ = c; }
    const BufferString&	description() const		{ return desc_; }
    void		setDescription( const char* d )	{ desc_ = d; }

    UnitRef*		upNode(int skip=0);
    const UnitRef*	upNode( int skip=0 ) const
    			{ return ((UnitRef*)this)->upNode( skip ); }
    UnitRef*		topNode()
    			{ return upnode_ ? upnode_->topNode() : this; }
    const UnitRef*	topNode() const
    			{ return upnode_ ? upnode_->topNode() : this; }

    int			level() const
			{ return upnode_ ? upnode_->level() + 1 : 0; }
    bool		isBelow(const UnitRef*) const;
    			//!< Any number of levels

protected:

    UnitRef*		upnode_;
    BufferString	code_;
    BufferString	desc_;

};


/*!\brief UnitRef for units containing other units only */

class NodeUnitRef : public UnitRef
{
public:

			NodeUnitRef( UnitRef* up, const char* c,
				     const char* d=0 )
			: UnitRef(up,c,d)		{}

    virtual bool	isLeaf() const			{ return false; }

    int			nrRefs() const			{ return refs_.size(); }
    UnitRef&		ref( int idx )			{ return *refs_[idx]; }
    const UnitRef&	ref( int idx ) const		{ return *refs_[idx]; }

    UnitRef*		find( const char* code )	{ return fnd(code); }
    const UnitRef*	find( const char* code ) const	{ return fnd(code); }

protected:

    ObjectSet<UnitRef>	refs_;

    UnitRef*		fnd(const char*) const;

};


/*!\brief UnitRef for layers */

class LeafUnitRef : public UnitRef
{
public:

			LeafUnitRef( UnitRef* up, const char* c,
				     const Lithology& l, const char* d=0 )
			: UnitRef(up,c,d)
			, lith_(l)	{}

    virtual bool	isLeaf() const		{ return true; }

    const Lithology&	lithology() const	{ return lith_; }

protected:

    const Lithology&	lith_;

};


}; // namespace Strat

#endif
