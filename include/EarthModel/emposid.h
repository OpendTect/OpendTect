#ifndef emposid_h
#define emposid_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emposid.h,v 1.4 2003-05-05 11:59:10 kristofer Exp $
________________________________________________________________________


-*/

#include "multiid.h"
#include "geomposidholder.h"

namespace EarthModel
{

typedef MultiID ObjectID;
typedef unsigned short PatchID;
typedef Geometry::PosID SubID;


/*!\brief
is an identifier for each position in the earthmodel. It has three patches,
- an ObjectID, wich identifies wich object is belongs to.
- a PatchID, wich identifies which patch of the object it belongs to.
- a SubID, wich identifies the position on the patch. 
*/

class PosID
{
public:
    				PosID( ObjectID emobj,
				       PatchID patch,
				       SubID subid );

    const ObjectID&		emObject() const;
    const PatchID&		patchID() const;
    const SubID&		subID() const;

    bool			operator==(const PosID& b) const;
    bool			operator!=(const PosID& b) const;

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:
    ObjectID			emobj;
    PatchID			patch;
    SubID			subid;

    static const char*		emobjstr;
    static const char* 		patchstr;
    static const char*		subidstr;
};


inline PosID::PosID( ObjectID emobj_,
		       PatchID patch_,
		       SubID subid_ )
    : emobj( emobj_ )
    , patch( patch_ )
    , subid( subid_ )
{}


inline bool PosID::operator==(const PosID& b) const
{ return emobj==b.emobj && patch==b.patch && subid==b.subid; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::emObject() const
{ return emobj; }


inline const PatchID& PosID::patchID() const
{ return patch; }


inline const SubID& PosID::subID() const
{ return subid; }



}; // Namespace


#endif
