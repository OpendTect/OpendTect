#ifndef vispolygonoffset_h
#define vispolygonoffset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		June 2006
 RCS:		$Id: vispolygonoffset.h,v 1.3 2009-07-22 16:01:24 cvsbert Exp $
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
