#ifndef visrandompos2body_h
#define visrandompos2body_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id: visrandompos2body.h,v 1.5 2012-08-03 13:01:25 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

class SoShapeHints;

namespace visBase
{
class TriangleStripSet;
class Transformation;

/*!This class displays the triangulated body based on a set of given points. 
   For example

   visBase::RandomPos2Body* nb = visBase::RandomPos2Body::create();
   nb->ref();
   nb->setPoints( known_points );

   will do the display work after add nb to your scene! 
*/

mClass(visBase) RandomPos2Body : public VisualObjectImpl
{
public:
    static RandomPos2Body*	create()
    				mCreateDataObj(RandomPos2Body);
					
    bool			setPoints(const TypeSet<Coord3>& pts);
    const TypeSet<Coord3>&	getPoints() const { return picks_; }

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			renderOneSide(int side);
    				/*!< 0 = visisble from both sides.
				     1 = visisble from positive side
				     -1 = visisble from negative side. */

protected:
    				~RandomPos2Body();

    TypeSet<Coord3>		picks_;			
    TriangleStripSet*		triset_;
    const mVisTrans*		transformation_;
    SoShapeHints*		hints_;
};


};


#endif

