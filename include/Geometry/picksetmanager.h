#ifndef picksetmanager_h
#define picksetmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "pickset.h"
#include "objectset.h"
#include "multiid.h"
#include "uistring.h"


namespace Pick
{
class Set;
class SetSaver;

/*!\brief Manages all stored Pick::Set's.


*/

mExpClass(General) SetManager : public Monitorable
{ mODTextTranslationClass(Pick::SetManager)
public:

    typedef ObjectSet<SetSaver>::size_type	size_type;
    typedef size_type				IdxType;

			// Use MonitorLock when iterating
    size_type		size() const;
    Set&		get(IdxType);
    const Set&		get(IdxType) const;
    MultiID		id(IdxType) const;

    IdxType		indexOf(const Set&) const;
    IdxType		indexOf(const MultiID&) const;
    Set*		get( const MultiID& i )		{ return find(i); }
    const Set*		get( const MultiID& i ) const	{ return find(i); }
    MultiID*		get( const Set& s ) const	{ return find(s); }

    void		add(const MultiID&,Set*,bool autosave=false,
						bool keepchgrecords=false);
    void		set(const MultiID&,Set*); //!< removes, then adds
    void		remove(const MultiID&);
    void		changeSetID(const MultiID& from,const MultiID& to);

    uiString		save(IdxType) const;

    CNotifier<SetManager,MultiID>	setToBeRemoved;
    CNotifier<SetManager,MultiID>	setAdded;
    CNotifier<SetManager,MultiID>	setDispChanged;
    CNotifier<SetManager,MultiID>	setContentChanged;

    mExpClass(General) ChangeEvent
    {
    public:
	enum Type	{ Create, Change, Delete };

			ChangeEvent( Type t, IdxType i, const Location& loc )
			    : type_(t), idx_(i), loc_(loc)	{}

	Type		type_;
	IdxType		idx_;
	Location	loc_; // location at (=before) change

	inline bool	operator==( const ChangeEvent& oth ) const
			{ return type_==oth.type_ && idx_==oth.idx_; }
    };
    mExpClass(General) ChangeRecord
    {
    public:
			ChangeRecord( const MultiID& id )
			    : setid_(id)		{}
			~ChangeRecord()		{ deepErase(events_); }

	MultiID		setid_;
	ObjectSet<ChangeEvent>	events_;
    };

    void		broadcastChanges(const MultiID&);
    void		broadcastChanges(const Set*);
    CNotifier<SetManager,ChangeRecord>	newChangeRecord;

protected:

				SetManager();
				~SetManager();

    ObjectSet<SetSaver>		savers_;
    ObjectSet<ChangeRecord>	records_;

    void		setEmpty();
    Set*		find(const MultiID&) const;
    MultiID*		find(const Set&) const;

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
