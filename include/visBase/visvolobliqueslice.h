#ifndef visvolobliqueslice_h
#define visvolobliqueslice_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: visvolobliqueslice.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class SoObliqueSlice;

namespace visBase
{

/*!\brief
*/

mClass ObliqueSlice : public visBase::VisualObjectImpl
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
