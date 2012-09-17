#ifndef vispolygonoffset_h
#define vispolygonoffset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		June 2006
 RCS:		$Id: vispolygonoffset.h,v 1.4 2011/04/28 07:00:12 cvsbert Exp $
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

protected:
    				~PolygonOffset();

    SoPolygonOffset*		offset_;

    virtual SoNode*		gtInvntrNode();

};

};

#endif
