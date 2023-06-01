#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"

#include "multiid.h"
#include "ptrman.h"
#include "undo.h"


namespace Pick
{

class Set;

/*!\brief Utility to manage pick set lifecycles.
          Also supports change notifications.

 You can create your own set manager for your own special pick sets.
 There is a OD-wide Mgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.

 */

mExpClass(General) SetMgr : public NamedCallBacker
{
public:
    static SetMgr&	getMgr(const char*);

    int			size() const		{ return pss_.size(); }
    RefMan<Set>		get( int idx )		{ return pss_[idx]; }
    ConstRefMan<Set>	get( int idx ) const	{ return pss_[idx]; }
    const MultiID&	id( int idx ) const;

    int			indexOf(const char*) const;
    int			indexOf(const Set&) const;
    int			indexOf(const MultiID&) const;

    // Convenience. Check indexOf() if presence is not sure
    const MultiID&	get( const Set& s ) const	{ return *find(s); }
    RefMan<Set>		get( const MultiID& mid )	{ return find( mid ); }
    ConstRefMan<Set>	get( const MultiID& mid ) const { return find( mid ); }
    RefMan<Set>		get( const char* nm )		{ return find( nm ); }
    ConstRefMan<Set>	get( const char* nm ) const	{ return find( nm ); }

    void		set(const MultiID&,Set*);
			//!< add, replace or remove (pass null Set ptr).
			//!< Set is already, or becomes *mine*
			//!< Note that replacement will trigger two callbacks
    void		setID(int idx,const MultiID&);

    struct ChangeData : public CallBacker
    {
	enum Ev		{ Added, Changed, ToBeRemoved };

			ChangeData( Ev e, const Set* s, int l )
			    : ev_(e), set_(s), loc_(l)		{}

	Ev		ev_;
	const Set*	set_;
	const int	loc_;
			//<refers to the idx in set_
    };

    void		reportChange(CallBacker* sender,const ChangeData&);
    void		reportChange(CallBacker* sender,const Set&);
    void		reportDispChange(CallBacker* sender,const Set&);

    Notifier<SetMgr>	locationChanged;//!< Passes ChangeData*
    Notifier<SetMgr>	setToBeRemoved;	//!< Passes Set*
    Notifier<SetMgr>	setAdded;	//!< passes Set*
    Notifier<SetMgr>	setChanged;	//!< passes Set*
    Notifier<SetMgr>	setDispChanged;	//!< passes Set*

    struct BulkChangeData : public CallBacker
    {
	enum Ev		{ Added, ToBeRemoved };

			BulkChangeData( Ev e, const Set* s,
					const TypeSet<int>& l )
			    : ev_(e), set_(s), locs_(l)		{}

	Ev		ev_;
	const Set*	set_;
	TypeSet<int>	locs_;
			//<refers to the indexes in set_ (sorted)
    };

    void		reportBulkChange(CallBacker* sender,
	    				 const BulkChangeData&);
    Notifier<SetMgr>	bulkLocationChanged;//!< Passes BulkChangeData*

    void		removeCBs(CallBacker*);

    bool		isChanged( int idx ) const
			{ return idx < changed_.size()
				? (bool) changed_[idx] : false;}
    void		setUnChanged( int idx, bool yn=true )
			{ if ( changed_.validIdx(idx) ) changed_[idx] = !yn; }

    Undo&		undo();
    const Undo&		undo() const;

    static BufferString getDispFileName(const MultiID&);
    bool		readDisplayPars(const MultiID&,IOPar&) const;
    bool		writeDisplayPars(const MultiID&,const IOPar&) const;

    static void		dumpMgrInfo(StringPairSet&);

			SetMgr( const char* nm );
			//!< creates an unmanaged SetMgr
			//!< Normally you don't want that, use getMgr() instead
			~SetMgr();

protected:

    Undo&		undo_;
    RefObjectSet<Set>	pss_;
    TypeSet<MultiID>	ids_;
    BoolTypeSet		changed_;

    void		add(const MultiID&,Set*);
    RefMan<Set>		find(const MultiID&);
    RefMan<Set>		find(const char*);
    ConstRefMan<Set>	find(const MultiID&) const;
    ConstRefMan<Set>	find(const char*) const;
    MultiID*		find(const Set&) const;

    void		survChg(CallBacker*);
    void		objRm(CallBacker*);
    void		removeAll();
};

inline SetMgr& Mgr();

} // namespace Pick


inline Pick::SetMgr& Pick::Mgr()
{
    return SetMgr::getMgr( nullptr );
}
