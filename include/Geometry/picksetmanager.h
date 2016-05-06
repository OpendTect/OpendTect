#ifndef picksetmanager_h
#define picksetmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "pickset.h"
#include "objectset.h"
#include "multiid.h"
#include "uistring.h"


namespace Pick
{
class Set;
class SetSaver;

/*!\brief Manages all stored Pick::Set's.

 If a set is not yet loaded, then it will be loaded by fetch(). If that fails,
 the uiString error message is set to something non-empty.

*/

mExpClass(Geometry) SetManager : public Monitorable
{ mODTextTranslationClass(Pick::SetManager)
public:

    typedef MultiID	SetID;

			// Normal usage: get a set whether already loaded or not
    Set&		fetch(const SetID&,uiString&);
    uiString		save(const SetID&) const;
    MultiID		getID(const Set&) const;

    bool		add(const SetID&,Set*);
    bool		isLoaded(const SetID&) const;

			// Use MonitorLock when iterating
    int			size() const;
    Set&		get(int);
    const Set&		get(int) const;
    SetID		getID(int) const;

    CNotifier<SetManager,MultiID>	setToBeRemoved;
    CNotifier<SetManager,MultiID>	setAdded;
    CNotifier<SetManager,MultiID>	setDispChanged;
    CNotifier<SetManager,MultiID>	setContentChanged;

    mExpClass(Geometry) ChangeEvent
    {
    public:

	typedef Set::size_type	LocIdxType; // TODO not a safe key to a loc

	enum Type	{ Create, Change, Delete };

			ChangeEvent( Type t, LocIdxType i, const Location& loc )
			    : type_(t), idx_(i), loc_(loc)	{}

	Type		type_;
	LocIdxType	idx_;
	Location	loc_; // location at (=before) change

	inline bool	operator==( const ChangeEvent& oth ) const
			{ return type_==oth.type_ && idx_==oth.idx_; }
    };
    mExpClass(Geometry) ChangeRecord
    {
    public:
			ChangeRecord( const MultiID& id )
			    : setid_(id)		{}
			~ChangeRecord()			{ deepErase(events_); }

	const MultiID			setid_;
	mutable ObjectSet<ChangeEvent>	events_;
    };

    void		broadcastChanges(const MultiID&);
    void		broadcastChanges(const Set&);
    CNotifier<SetManager,ChangeRecord>	newChangeEvent;

protected:

				SetManager();
				~SetManager();

    ObjectSet<SetSaver>		savers_;
    ObjectSet<ChangeRecord>	records_;

    void		setEmpty();
    int			gtIdx(const SetID&) const;

    void		iomEntryRemovedCB(CallBacker*);
    void		survChgCB(CallBacker*);
    void		appExitCB(CallBacker*);
    void		setDelCB(CallBacker*);
    void		setChgCB(CallBacker*);

public:

    static SetManager&	getInstance();
    mDeclInstanceCreatedNotifierAccess(SetManager);

};

inline SetManager& SetMGR();

} // namespace Pick


inline Pick::SetManager& Pick::SetMGR()
{
    return SetManager::getInstance();
}


#endif
