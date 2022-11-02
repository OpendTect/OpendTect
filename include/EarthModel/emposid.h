#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "integerid.h"
#include "rowcol.h"


namespace EM
{

mExpClass(EarthModel) ObjectID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline ObjectID	udf()		{ return ObjectID(); }

protected:
};


mExpClass(EarthModel) SectionID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline SectionID	def()		{ return SectionID(0); }
    static inline SectionID	udf()		{ return SectionID(); }

protected:
};


using FaultID	= ObjectID;
using SubID	= od_int64;

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
				PosID(const ObjectID& emobjid=ObjectID::udf(),
				    const SectionID& sectionid=SectionID::udf(),
				    SubID subid=0);
				PosID(const ObjectID&,const RowCol&);
				PosID(const ObjectID&,SubID);
				~PosID();

    static const PosID&		udf();
    void			setUdf();
    bool			isUdf() const;
    bool			isValid() const;

    const ObjectID&		objectID() const;
    const SectionID&		sectionID() const;
    SubID			subID() const;
    void			setObjectID(const ObjectID&);
    void			setSectionID(const SectionID&);
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


inline bool PosID::operator==(const PosID& b) const
{ return emobjid_==b.emobjid_ && sectionid_==b.sectionid_ && subid_==b.subid_; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::objectID() const
{ return emobjid_; }

inline const SectionID& PosID::sectionID() const
{ return sectionid_; }

inline SubID PosID::subID() const
{ return subid_; }

inline void PosID::setObjectID( const ObjectID& id )
{ emobjid_ = id; }

inline void PosID::setSectionID( const SectionID& id )
{ sectionid_ = id; }

inline void PosID::setSubID( SubID id )
{ subid_ = id; }

} // namespace EM
