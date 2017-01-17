#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "dbkey.h"
#include "rowcol.h"


namespace EM
{

typedef od_int16 SectionID;
typedef od_int64 SubID;

/*!
\brief Is an identifier for each position in the earthmodel.

It has three parts,
- a DBKey, which identifies wich object is belongs to.
- a SectionID, which identifies which section of the object it belongs to.
- a SubID, which identifies the position on the section.
*/

mExpClass(EarthModel) PosID
{
public:
				PosID( DBKey objid=DBKey::getInvalid(),
				       SectionID sectionid=0,
				       SubID subid=0);

    static const PosID&		udf();
    bool			isUdf() const;

    const DBKey&		objectID() const;
    SectionID			sectionID() const;
    SubID			subID() const;
    void			setObjectID(const DBKey&);
    void			setSectionID(SectionID);
    void			setSubID(SubID);
    RowCol			getRowCol() const;

    bool			operator==(const PosID& b) const;
    bool			operator!=(const PosID& b) const;

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:

    DBKey			objid_;
    SectionID			sectionid_;
    SubID			subid_;

    static const char*		emobjStr();
    static const char*		sectionStr();
    static const char*		subidStr();
};


inline PosID::PosID( DBKey objid, SectionID section, SubID subid )
    : objid_(objid)
    , sectionid_(section)
    , subid_(subid)
{}


inline bool PosID::operator==(const PosID& b) const
{ return objid_==b.objid_ && sectionid_==b.sectionid_ && subid_==b.subid_; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const DBKey& PosID::objectID() const
{ return objid_; }

inline SectionID PosID::sectionID() const
{ return sectionid_; }

inline SubID PosID::subID() const
{ return subid_; }

inline void PosID::setObjectID( const DBKey& id )
{ objid_ = id; }

inline void PosID::setSectionID( SectionID id )
{ sectionid_ = id; }

inline void PosID::setSubID( SubID id )
{ subid_ = id; }


} // namespace EM
