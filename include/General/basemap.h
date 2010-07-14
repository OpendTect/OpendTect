#ifndef basemap_h
#define basemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id: basemap.h,v 1.4 2010-07-14 06:06:59 cvsraman Exp $
________________________________________________________________________

-*/

#include "namedobj.h"


/*!Object that can be painted in a basemap. */


mClass BaseMapObject : public NamedObject
{
public:
				BaseMapObject(const char* nm)
				    : NamedObject(nm)
				{}

    virtual const char*		getType() const				= 0;

    virtual void		updateGeometry()			= 0;

    virtual void		setDepth(int val)	{ depth_ = val; }
    virtual int			getDepth() const	{ return depth_; }
    				/*!<Determines what should be painted ontop of
				    what */

//    virtual int			nrShapes() const;
//    virtual const char*		getShapeName(int shapeidx) const;
//    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const; 
    				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */
//    virtual char		connectPoints(int shapeidx) const;
    				/*!\retval 0 - dont connect
				   \retval 1 - connect
				   \retval 2 - connect as polygon.
				   \retval 3 - connect as polygon and fill */
//    virtual const Color*	getColor(int shapeidx) const;

//    virtual const OD::Image*	getImage(Coord& origin,Coord& p11) const;
    				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
//    virtual const OD::Image*	getPreview(int approxdiagonal) const;
    				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */
protected:

    int				depth_;

};


/*!Base class for a Basemap. */
mClass BaseMap
{
public:
    virtual void		addObject(BaseMapObject*) 		= 0;
    				/*!<Object maintained by caller */
    virtual void		removeObject(const BaseMapObject*) 	= 0;
};


#endif
