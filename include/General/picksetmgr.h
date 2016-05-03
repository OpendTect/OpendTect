#ifndef picksetmgr_h
#define picksetmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "pickset.h"
#include "multiid.h"
#include "undo.h"


namespace Pick
{

/*!\brief Utility to manage pick set lifecycles.
          Also supports change notifications.

 You can create your own set manager for your own special pick sets.
 There is a OD-wide Mgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.

 */

mExpClass(General) SetMgr : public NamedMonitorable
{
public:

			~SetMgr();
    int			size() const		{ return pss_.size(); }
    Set&		get( int idx )		{ return *pss_[idx]; }
    const Set&		get( int idx ) const	{ return *pss_[idx]; }
    const MultiID&	id( int idx ) const;

    int			indexOf(const char*) const;
    int			indexOf(const Set&) const;
    int			indexOf(const MultiID&) const;

    // Convenience. Check indexOf() if presence is not sure
    Set&		get( const MultiID& i )		{ return *find(i); }
    const Set&		get( const MultiID& i ) const	{ return *find(i); }
    const MultiID&	get( const Set& s ) const	{ return *find(s); }
    Set&		get( const char* s )		{ return *find(s); }
    const Set&		get( const char* s ) const	{ return *find(s); }

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
    void		removeCBs(CallBacker*);

    bool		isChanged( int idx ) const
			{ return idx < changed_.size()
				? (bool) changed_[idx] : false;}
    void		setUnChanged( int idx, bool yn=true )
			{ if ( changed_.validIdx(idx) ) changed_[idx] = !yn; }

    Undo&		undo()			{ return undo_; }
    const Undo&		undo() const		{ return undo_; }

    static SetMgr&	getMgr(const char*);
			SetMgr( const char* nm );
			//!< creates an unmanaged SetMgr
			//!< Normally you don't want that, use getMgr() instead

protected:

    Undo&		undo_;
    ObjectSet<Set>	pss_;
    TypeSet<MultiID>	ids_;
    BoolTypeSet		changed_;

    void		add(const MultiID&,Set*);
    Set*		find(const MultiID&) const;
    MultiID*		find(const Set&) const;
    Set*		find(const char*) const;

    void		survChg(CallBacker*);
    void		objRm(CallBacker*);
    void		removeAll();

    void		transfer(Set&,SetMgr&);
    friend class	Set;

};

inline SetMgr& Mgr();

} // namespace Pick


inline Pick::SetMgr& Pick::Mgr()
{
    return SetMgr::getMgr(0);
}


#endif
