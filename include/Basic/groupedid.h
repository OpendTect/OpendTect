#ifndef groupedid_h
#define groupedid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "integerid.h"


/*!\brief 2 integer IDs combined to designate a Grouped Object.

to prevent automatic conversion between GroupID & ObjectID we need to define
different types for GroupID & ObjID

e.g
GroupedID <short int, int>

*/

template <class GroupNrType,class ObjNrType>
mClass(Basic) GroupedID
{
public:

    typedef IntegerID<GroupNrType>	    GroupIDType;
    typedef IntegerID<ObjNrType>	    ObjIDType;

    static inline GroupedID get( GroupNrType grpnr, ObjNrType objnr )
			{ return GroupedID(grpnr,objnr); }

    inline GroupIDType	getGroupID() const
			{ return GroupIDType::get(groupnr_); }
    inline ObjIDType	getObjID() const
			{ return ObjIDType::get(objnr_); }
    inline GroupNrType	getGroupNr() const		{ return groupnr_; }
    inline ObjNrType	getObjNr() const		{ return objnr_; }
    inline void		setGroupID( GroupIDType id )	{ groupnr_ = id.getI();}
    inline void		setObjID( ObjIDType id )	{ objnr_ = id.getI(); }
    inline void		setGroupNr( GroupNrType nr )	{ groupnr_ = nr; }
    inline void		setObjNr( ObjNrType nr )	{ objnr_ = nr; }

    inline bool		operator ==( const GroupedID& oth ) const
			{ return groupnr_ == oth.groupnr_ &&
				 objnr_ == oth.objnr_; }
    inline bool		operator !=( const GroupedID& oth ) const
			{ return groupnr_ != oth.groupnr_ ||
				 objnr_ != oth.objnr_; }

    inline bool		isInvalid() const
				{ return groupnr_<0 || objnr_<0; }
    inline bool		isValid() const			{ return !isInvalid(); }
    inline bool		hasValidGroup() const		{ return groupnr_>=0; }
    inline bool		hasValidObj() const		{ return objnr_>=0; }
    inline void		setInvalid()
			{ setInvalidGroup(); setInvalidObj(); }
    inline void		setInvalidGroup()		{ groupnr_=-1; }
    inline void		setInvalidObj()			{ objnr_ = -1; }
    static inline GroupedID getInvalid()
				{ return GroupedID(-1,-1); }

private:

    GroupNrType		groupnr_;
    ObjNrType		objnr_;

    inline		GroupedID( GroupNrType gnr=0, ObjNrType onr=0 )
			    : groupnr_(gnr), objnr_(onr) { /* keep private! */ }

};


#endif
