#ifndef vispolygonoffset_h
#define vispolygonoffset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		June 2006
 RCS:		$Id: vispolygonoffset.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoPolygonOffset;

namespace visBase
{
/*!Class that manipulates the zbuffer. See coin for details. */

mClass PolygonOffset : public DataObject
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
