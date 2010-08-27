#ifndef basemap_h
#define basemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id: basemap.h,v 1.6 2010-08-27 02:45:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

namespace OD { class RGBImage; }

class Coord;
class Color;


/*!Object that can be painted in a basemap. */


mClass BaseMapObject : public NamedObject
{
public:
				BaseMapObject(const char* nm);

    virtual const char*		getType() const				= 0;

    virtual void		updateGeometry()			{}

    virtual void		setDepth(int val)	{ depth_ = val; }
    virtual int			getDepth() const	{ return depth_; }
    				/*!<Determines what should be painted ontop of
				    what */

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const; 
    				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */
    virtual char		connectPoints(int shapeidx) const;
    				/*!\retval cDontConnect() - don't connect
				   \retval cConnect() - connect
				   \retval cPolygon() - connect as polygon.
				   \retval cFilledPolygon() - connect as
				   		polygon and fill */
    virtual const Color*	getColor(int shapeidx) const;

    virtual const OD::RGBImage*	getImage(Coord& origin,Coord& p11) const;
    				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    virtual const OD::RGBImage*	getPreview(int approxdiagonal) const;
    				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */
    static char			cDontConnect()		{ return 0; }
    static char			cConnect()		{ return 1; }
    static char			cPolygon()		{ return 2; }
    static char			cFilledPolygon()	{ return 3; }

    Notifier<BaseMapObject>	changed;

protected:

    int				depth_;

};


/*!Base class for a Basemap. */
mClass BaseMap
{
public:
    virtual void		addObject(BaseMapObject*) 		= 0;
    				/*!<Object maintained by caller. Adding an
				    existing will trigger update */

    virtual void		removeObject(const BaseMapObject*) 	= 0;
};


#endif
