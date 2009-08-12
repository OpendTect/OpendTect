#ifndef visbeachball_h
#define visbeachball_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: visbeachball.h,v 1.1 2009-08-12 09:59:19 cvskarthika Exp $
________________________________________________________________________

-*/


#include "visobject.h"
#include "position.h"

class SoBeachBall;

namespace visBase
{
/*! \brief 
Display a beachball-type object.
*/

mClass BeachBall : public VisualObjectImpl
{
public:

    static BeachBall*		create()
    				mCreateDataObj(BeachBall);

    Coord3			position() const;
    void			setPosition(Coord3);
    float			radius() const;
    void			setRadius(float);

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    static const char*		radiusstr();

protected:
    				~BeachBall();

    SoBeachBall*		ball_;

};


};

#endif
