#ifndef emposid_h
#define emposid_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emposid.h,v 1.3 2003-04-22 11:00:16 kristofer Exp $
________________________________________________________________________


-*/

#include "multiid.h"
#include "geomposidholder.h"

namespace EarthModel
{

typedef MultiID ObjectID;
typedef unsigned short PartID;
typedef Geometry::PosID SubID;


/*!\brief
is an identifier for each position in the earthmodel. It has three parts,
- an ObjectID, wich identifies wich object is belongs to.
- a PartID, wich identifies which part of the object it belongs to.
- a SubID, wich identifies the position on the part. 
*/

class PosID
{
public:
    				PosID( ObjectID emobj,
				       PartID part,
				       SubID subid );

    const ObjectID&		emObject() const;
    const PartID&		partID() const;
    const SubID&		subID() const;

    bool			operator==(const PosID& b) const;
    bool			operator!=(const PosID& b) const;

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:
    ObjectID			emobj;
    PartID			part;
    SubID			subid;

    static const char*		emobjstr;
    static const char* 		partstr;
    static const char*		subidstr;
};


inline PosID::PosID( ObjectID emobj_,
		       PartID part_,
		       SubID subid_ )
    : emobj( emobj_ )
    , part( part_ )
    , subid( subid_ )
{}


inline bool PosID::operator==(const PosID& b) const
{ return emobj==b.emobj && part==b.part && subid==b.subid; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::emObject() const
{ return emobj; }


inline const PartID& PosID::partID() const
{ return part; }


inline const SubID& PosID::subID() const
{ return subid; }



}; // Namespace


#endif
