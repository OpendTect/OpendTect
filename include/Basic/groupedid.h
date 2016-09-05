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


/*!\brief A unique identifier for an object consisting of a group and an ID
	within the group.

In many places there is a need for an ID consisting of not only an object ID but also a group ID. If the groups were closed, an enum would be sufficient. But
if the number of groups is not fixed (e.g. because they are in a factory) then
both group ID and the object ID need to be integer numbers.

Usually there are a lot less groups than possible objects, so the group ID can
be represented by a short int.

*/

template <class GroupNrT,class ObjNrT>
mClass(Basic) IDWithGroup
{
public:

    typedef GroupNrT		GroupNrType;
    typedef ObjNrT		ObjNrType;
    mDefIntegerIDType(GroupNrT,	GroupID);
    mDefIntegerIDType(ObjNrT,	ObjID);

			IDWithGroup( GroupID gid, ObjID oid )
			    : groupnr_(gid.getI())
			    , objnr_(oid.getI())	{}
    static inline IDWithGroup get( GroupNrT grpnr, ObjNrT objnr )
			{ return IDWithGroup(grpnr,objnr); }

    inline GroupID	groupID() const
			{ return GroupID::get(groupnr_); }
    inline ObjID	objID() const
			{ return ObjID::get(objnr_); }
    inline GroupNrT	groupNr() const			{ return groupnr_; }
    inline ObjNrT	objNr() const			{ return objnr_; }
    inline void		setGroupID( GroupID id )	{ groupnr_ = id.getI();}
    inline void		setObjID( ObjID id )		{ objnr_ = id.getI(); }
    inline void		setGroupNr( GroupNrT nr )	{ groupnr_ = nr; }
    inline void		setObjNr( ObjNrT nr )		{ objnr_ = nr; }

    inline bool		operator ==( const IDWithGroup& oth ) const
				{ return groupnr_ == oth.groupnr_ &&
					 objnr_ == oth.objnr_; }
    inline bool		operator !=( const IDWithGroup& oth ) const
				{ return groupnr_ != oth.groupnr_ ||
					 objnr_ != oth.objnr_; }

    inline bool		isInvalid() const
				{ return groupnr_<0 || objnr_<0; }
    inline bool		isValid() const			{ return !isInvalid(); }
    inline bool		hasValidGroupID() const		{ return groupnr_>=0; }
    inline bool		hasValidObjID() const		{ return objnr_>=0; }
    inline void		setInvalid()
			{ setInvalidGroup(); setInvalidObj(); }
    inline void		setInvalidGroup()		{ groupnr_=-1; }
    inline void		setInvalidObj()			{ objnr_ = -1; }
    static inline IDWithGroup getInvalid()
				{ return IDWithGroup(-1,-1); }

    od_int64		toInt64() const;
    static IDWithGroup	fromInt64(od_int64);

protected:

    GroupNrT		groupnr_;
    ObjNrT		objnr_;

    inline		IDWithGroup( GroupNrT gnr=0, ObjNrT onr=0 )
			    : groupnr_(gnr), objnr_(onr) { /* keep private! */ }

};


typedef IDWithGroup<short,int> GroupedID;


template <class GroupNrT,class ObjNrT>
inline od_int64 IDWithGroup<GroupNrT,ObjNrT>::toInt64() const
{
    return (((od_uint64)groupnr_) << 32) + (((od_uint64)objnr_) & 0xFFFFFFFF);
}


template <class GroupNrT,class ObjNrT>
inline IDWithGroup<GroupNrT,ObjNrT>
IDWithGroup<GroupNrT,ObjNrT>::fromInt64( od_int64 i64 )
{
    return IDWithGroup<GroupNrT,ObjNrT>( (GroupNrT)(i64 >> 32),
					 (ObjNrT)(i64 & 0xFFFFFFFF) );
}


#endif
