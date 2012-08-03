#ifndef visvolobliqueslice_h
#define visvolobliqueslice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: visvolobliqueslice.h,v 1.4 2012-08-03 13:01:27 cvskris Exp $
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

mClass(visBase) ObliqueSlice : public visBase::VisualObjectImpl
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

