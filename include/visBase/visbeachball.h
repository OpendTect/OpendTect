#ifndef visbeachball_h
#define visbeachball_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
 RCS:           $Id: visbeachball.h,v 1.2 2009-08-12 15:58:27 cvskarthika Exp $
________________________________________________________________________

-*/


#include "visobject.h"
#include "position.h"

class SoBeachBall;
class Color;
class SoShapeScale;
class UTMPosition;
class SoTranslation;

namespace visBase
{
class Transformation;

/*! \brief 
Display a beachball-type object.
*/

mClass BeachBall : public VisualObjectImpl
{
public:

    static BeachBall*		create()
    				mCreateDataObj(BeachBall);

    void			setCenterPosition(Coord3);
    Coord3			getCenterPosition() const;
    void			setRadius(float);
    float			getRadius() const;
    void			setColor1(Color);
    Color			getColor1() const;
    void                        setColor2(Color);
    Color                       getColor2() const;

    void             		setDisplayTransformation(Transformation*);
    Transformation*             getDisplayTransformation();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    static const char*		radiusstr();

protected:
    				~BeachBall();

    SoBeachBall*		ball_;
    SoNode*			shape_;
    SoTranslation*      	translation_;
    UTMPosition*        	xytranslation_;
    SoShapeScale*		scale_;
    Transformation*             transformation_;
};


};

#endif
