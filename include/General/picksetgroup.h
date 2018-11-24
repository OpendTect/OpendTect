#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "pickset.h"


namespace Pick
{


/*!\brief Group of Pick::Set's. */

mExpClass(General) SetGroup : public SharedObject
{
public:

    typedef ObjectSet<Location>::size_type	size_type;
    typedef size_type				idx_type;
    typedef RefMan<Set>				SetRefMan;
    typedef ConstRefMan<Set>			CSetRefMan;
    mDefIntegerIDType(				SetID);

			SetGroup(const char* nm=0);
			mDeclMonitorableAssignment(SetGroup);
			mDeclInstanceCreatedNotifierAccess(SetGroup);

    SetRefMan		getSet(SetID);
    CSetRefMan		getSet(SetID) const;
    SetRefMan		getSetByName(const char*);
    CSetRefMan		getSetByName(const char*) const;
    SetRefMan		getSetByIdx(idx_type);
    CSetRefMan		getSetByIdx(idx_type) const;
    SetRefMan		firstSet();
    CSetRefMan		firstSet() const;

    size_type		size() const;
    od_int64		nrLocations() const;
    bool		validSetID(SetID) const;
    SetID		getID(idx_type) const;
    int			indexOf(SetID) const;
    int			indexOf(const char*) const;
    bool		validIdx(idx_type) const;
    bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();
    bool		hasLocations() const	{ return nrLocations() > 0; }

    SetID		add(Set*);
    SetRefMan		remove(SetID);
    SetRefMan		removeByName(const char*);

    bool		isPresent(const char*) const;
    void		getNames(BufferStringSet&) const;

    static ChangeType	cSetAdd()	    { return 2; }
    static ChangeType	cSetRemove()	    { return 3; }
    static ChangeType	cOrderChange()	    { return 4; }

protected:

			~SetGroup();

    ObjectSet<Set>	sets_;
    TypeSet<SetID>	setids_;
    mutable Threads::Atomic<idx_type> cursetidnr_;

    idx_type		gtIdx(SetID) const;
    Set*		gtSet(SetID) const;
    SetID		gtID(const Set*) const;
    idx_type		gtIdxByName(const char*) const;
    Set*		gtSetByName(const char*) const;
    Set*		gtSetByIdx(idx_type) const;
    Set*		doRemove(idx_type);
    void		doSetEmpty();

    friend class	SetGroupIter;

public:

    bool		swap(idx_type,idx_type);

};


/*!\brief const SetGroup iterator. */

mExpClass(General) SetGroupIter
		: public MonitorableIter4Read<SetGroup::idx_type>
{
public:

			SetGroupIter(const SetGroup&,bool start_at_end=false);
			SetGroupIter(const SetGroupIter&);
    const SetGroup&	setGroup() const;

    SetGroup::SetID	ID() const;
    const Set&		set() const;

    mDefNoAssignmentOper(SetGroupIter)

};


} // namespace Pick
