#ifndef visvolobliqueslice_h
#define visvolobliqueslice_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: visvolobliqueslice.h,v 1.1 2007-01-05 21:57:25 cvskris Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class SoObliqueSlice;

namespace visBase
{

/*!\brief
*/

class ObliqueSlice : public visBase::VisualObjectImpl
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
