#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "multiid.h"
#include "rowcol.h"


namespace EM
{

typedef od_int32 ObjectID;
typedef od_int16 SectionID;
typedef od_int64 SubID;

/*!
\brief Is an identifier for each position in the earthmodel.

It has three parts,
- an ObjectID, which identifies wich object is belongs to.
- a SectionID, which identifies which section of the object it belongs to.
- a SubID, which identifies the position on the section. 
*/

mExpClass(EarthModel) PosID
{
public:
    				PosID( ObjectID emobjid=0,
				       SectionID sectionid=0,
				       SubID subid=0);

    static const PosID&		udf();
    bool			isUdf() const;

    const ObjectID&		objectID() const;
    SectionID			sectionID() const;
    SubID			subID() const;
    void			setObjectID(const ObjectID&);
    void			setSectionID(SectionID);
    void			setSubID(SubID);
    RowCol			getRowCol() const;

    bool			operator==(const PosID& b) const;
    bool			operator!=(const PosID& b) const;

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:

    ObjectID			emobjid_;
    SectionID			sectionid_;
    SubID			subid_;

    static const char*		emobjStr();
    static const char* 		sectionStr();
    static const char*		subidStr();
};


inline PosID::PosID( ObjectID emobj, SectionID section, SubID subid )
    : emobjid_(emobj)
    , sectionid_(section)
    , subid_(subid)
{}


inline bool PosID::operator==(const PosID& b) const
{ return emobjid_==b.emobjid_ && sectionid_==b.sectionid_ && subid_==b.subid_; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::objectID() const
{ return emobjid_; }

inline SectionID PosID::sectionID() const
{ return sectionid_; }

inline SubID PosID::subID() const
{ return subid_; }

inline void PosID::setObjectID( const ObjectID& id )
{ emobjid_ = id; }

inline void PosID::setSectionID( SectionID id )
{ sectionid_ = id; }

inline void PosID::setSubID( SubID id )
{ subid_ = id; }


} // namespace EM


