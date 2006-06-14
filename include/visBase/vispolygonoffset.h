#ifndef vispolygonoffset_h
#define vispolygonoffset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		June 2006
 RCS:		$Id: vispolygonoffset.h,v 1.1 2006-06-14 17:08:09 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoPolygonOffset;

namespace visBase
{
/*!Class that manipulates the zbuffer. See coin for details. */

class PolygonOffset : public DataObject
{
public:
    static PolygonOffset*	create()
				mCreateDataObj(PolygonOffset);

    enum Style			{ Filled, Lines, Points };
    void			setStyle(Style);
    Style			getStyle() const;

    void			setFactor(float);
    float			getFactor() const;

    void			setUnits(float);
    float			getUnits() const;

    SoNode*			getInventorNode();

protected:
    				~PolygonOffset();

    SoPolygonOffset*		offset_;
};

};

#endif
