#ifndef visvolobliqueslice_h
#define visvolobliqueslice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: visvolobliqueslice.h,v 1.3 2009/07/22 16:01:25 cvsbert Exp $
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
