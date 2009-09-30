#ifndef basemap_h
#define basemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id: basemap.h,v 1.2 2009-09-30 07:15:49 cvskris Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

namespace OD { class Image; }

/*!Object that can be painted in a basemap. */


mClass BaseMapObject : public NamedObject
{
public:
				BaseMapObject()
				    : changeOccurred( this )
				{}

    virtual bool		isEnabled() const;

    virtual const char*		getType() const				= 0;

    virtual float		getDepth() const			= 0;
    				/*!<Determines what should be painted ontop of
				    what */

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const; 
    				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */
    virtual char		connectPoints(int shapeidx) const;
    				/*!\retval 0 - dont connect
				   \retval 1 - connect
				   \retval 2 - connect as polygon.
				   \retval 3 - connect as polygon and fill */
    virtual const Color*	getColor(int shapeidx) const;

    virtual const OD::Image*	getImage(Coord& origin,Coord& p11) const;
    				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    virtual const OD::Image*	getPreview(int approxdiagonal) const;
    				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */

    Notifier			changeOccurred;
};


/*!Base class for a Basemap. */
mClass BaseMap : public CallBacker
{
public:
    virtual void		addObject(BaseMapObject*) 		= 0;
    				/*!<Object maintained by caller */
    virtual void		removeObject(BaseMapObject*) 		= 0;

    static BaseMap*		getImpl() { return impl_; }
    static void			setImpl(BaseMap* i) { delete impl_; impl=i; }
protected:

    static BaseMap*		impl_;
};


#endif
