#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "integerid.h"
#include "bufstring.h"


/*!\brief A unique identifier for an object consisting of a group and an ID
	within the group.

In many places there is a need for an ID consisting of not only an object ID
but also a group ID. If the groups were closed, an enum would be sufficient.
But if the number of groups is not fixed (e.g. because they are in a factory)
then both group ID and the object ID need to be integer numbers.

Usually there are a lot less groups than possible objects, so the group ID can
be represented by a short int.

The most standard GroupedID class (used for o.a. DataPack IDs) is a typedef for
IDWithGroup<od_int16,int>.

*/

template <class GroupNrT,class ObjNrT>
mClass(Basic) IDWithGroup
{
public:

    typedef GroupNrT			GroupNrType;
    typedef ObjNrT			ObjNrType;
    typedef IntegerID<GroupNrT>		GroupID;
    typedef IntegerID<ObjNrT>		ObjID;

			IDWithGroup()
			    : groupnr_(-1), objnr_(-1)	{}
			IDWithGroup( GroupID gid, ObjID oid )
			    : groupnr_(gid.asInt())
			    , objnr_(oid.asInt())	{}
    virtual		~IDWithGroup()			{}
    static inline IDWithGroup get( GroupNrT grpnr, ObjNrT objnr )
			{ return IDWithGroup(grpnr,objnr); }

    inline bool operator==( const IDWithGroup& oth ) const
			{ return groupnr_==oth.groupnr_ && objnr_==oth.objnr_; }
    inline bool operator!=( const IDWithGroup& oth ) const
			{ return !(*this==oth); }

    inline GroupID	groupID() const
			{ return GroupID::get(groupnr_); }
    inline ObjID	objID() const
			{ return ObjID::get(objnr_); }
    inline GroupNrT	groupNr() const			{ return groupnr_; }
    inline ObjNrT	objNr() const			{ return objnr_; }
    inline void		setGroupID( GroupID id )	{ groupnr_=id.asInt();}
    inline void		setObjID( ObjID id )		{ objnr_=id.asInt(); }
    inline void		setGroupNr( GroupNrT nr )	{ groupnr_ = nr; }
    inline void		setObjNr( ObjNrT nr )		{ objnr_ = nr; }

    virtual bool	isInvalid() const
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

			// serialization to string
    virtual BufferString toString() const;
    static bool		isValidString(const char*);
    virtual void	fromString(const char*);
    static IDWithGroup	getFromString(const char*);
			// serialization to int64
    virtual od_int64	toInt64() const;
    virtual void	fromInt64(od_int64);
    static IDWithGroup	getFromI64(od_int64);

protected:

    GroupNrT		groupnr_;
    ObjNrT		objnr_;

    inline		IDWithGroup( GroupNrT gnr, ObjNrT onr )
			    : groupnr_(gnr), objnr_(onr) { /*keep protected!*/ }

};


typedef IDWithGroup<od_int16,int> GroupedID;



		// these functions allow a trailer afteer a pipe symbol
		// example: 124.8|the_trailer
mGlobal(Basic) bool isValidGroupedIDString(const char*);
mGlobal(Basic) void getGroupedIDNumbers(const char*,od_int64&,od_int64&,
			    BufferString* auxpart=0,BufferString* survpart=0);


template <class GroupNrT,class ObjNrT> inline
BufferString IDWithGroup<GroupNrT,ObjNrT>::toString() const
{
    BufferString ret;
    ret.add( groupnr_ );
    ret.add( "." );
    ret.add( objnr_ );
    return ret;
}


template <class GroupNrT,class ObjNrT> inline
bool IDWithGroup<GroupNrT,ObjNrT>::isValidString( const char* str )
{
    return isValidGroupedIDString( str );
}


template <class GroupNrT,class ObjNrT> inline
void IDWithGroup<GroupNrT,ObjNrT>::fromString( const char* str )
{
    od_int64 gnr, onr;
    getGroupedIDNumbers( str, gnr, onr );
    groupnr_ = (GroupNrT)gnr;
    objnr_ = (ObjNrT)onr;
}


template <class GroupNrT,class ObjNrT> inline
IDWithGroup<GroupNrT,ObjNrT>
IDWithGroup<GroupNrT,ObjNrT>::getFromString( const char* str )
{
    IDWithGroup ret = getInvalid();
    ret.fromString( str );
    return ret;
}


template <class GroupNrT,class ObjNrT> inline
od_int64 IDWithGroup<GroupNrT,ObjNrT>::toInt64() const
{
    return (((od_uint64)groupnr_) << 32) + (((od_uint64)objnr_) & 0xFFFFFFFF);
}


template <class GroupNrT,class ObjNrT> inline
void IDWithGroup<GroupNrT,ObjNrT>::fromInt64( od_int64 i64 )
{
    groupnr_ = (GroupNrT)(i64 >> 32);
    objnr_ = (ObjNrT)(i64 & 0xFFFFFFFF);
}


template <class GroupNrT,class ObjNrT> inline
IDWithGroup<GroupNrT,ObjNrT>
IDWithGroup<GroupNrT,ObjNrT>::getFromI64( od_int64 i64 )
{
    IDWithGroup ret = getInvalid();
    ret.fromInt64( i64 );
    return ret;
}
