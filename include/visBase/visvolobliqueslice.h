#ifndef visvolobliqueslice_h
#define visvolobliqueslice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

class SoObliqueSlice;

namespace visBase
{

/*!\brief
*/

mExpClass(visBase) ObliqueSlice : public visBase::VisualObjectImpl
{
public:
    static ObliqueSlice*	create()
				mCreateDataObj(ObliqueSlice);

    Coord3			getPosOnPlane() const;
    Coord3			getNormal() const;

    void			set( const Coord3& normal, const Coord3& pos );

protected:
				~ObliqueSlice();
    SoObliqueSlice*		slice_;
};


};
	
#endif

