#ifndef visbeachball_h
#define visbeachball_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
 RCS:           $Id$
________________________________________________________________________

-*/


#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

class SoBeachBall;
class Color;
class SoScale;
class UTMPosition;
class SoTranslation;
class SoDrawStyle;
class SoMaterial;
class SoMaterialBinding;
class SoComplexity;

namespace visBeachBall { class BallProperties; }
namespace visBase
{
class Transformation;
class DrawStyle;

/*! \brief 
Display a beachball-type object.
*/

mExpClass(visBase) BeachBall : public VisualObjectImpl
{
public:

    static BeachBall*		create()
    				mCreateDataObj(BeachBall);

    void			setBallProperties(const 
	    				visBeachBall::BallProperties&);
    visBeachBall::BallProperties getBallProperties() const;

    void             		setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setZScale(float zScale);
    float			getZScale() const;
    void			setCenterPosition(const Coord3&);
    Coord3			getCenterPosition() const;
    void			setRadius(float);
    float			getRadius() const;
    void			setColor1(Color);
    Color			getColor1() const;
    void                        setColor2(Color);
    Color                       getColor2() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    static const char*		radiusstr();
    static const char*		centerstr();
    static const char*		color1str();	
    static const char*		color2str();	

protected:
    				~BeachBall();

    SoBeachBall*		ball_;
    SoMaterial*			material_;
    SoMaterialBinding*		matbinding_;
    SoComplexity*		complexity_;
    SoTranslation*      	translation_;
    UTMPosition*        	xyTranslation_;
    SoScale*			scale_;
    const mVisTrans*		transformation_;

    float			radius_;
    float			zScale_;
};
};

#endif

