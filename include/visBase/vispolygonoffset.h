#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		June 2006
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visnodestate.h"

namespace osg { class PolygonOffset; }

namespace visBase
{
/*!Class that manipulates the zbuffer. See coin for details. */

mExpClass(visBase) PolygonOffset : public NodeState
{
public:
				PolygonOffset();

    void			setFactor(float);
    float			getFactor() const;

    void			setUnits(float);
    float			getUnits() const;
    enum			Mode
				{ Off=0,On=1,Override=2,Protected=4,Inherit=8 };
    void			setMode(unsigned int);
				//!<coresponding to  osg::StateAttributesvalue
				//!<:GLMODE<OFF,ON,OVERRIDE,PROTECTED and
				//!<INHERIT or their combination.
    osg::PolygonOffset*		osgPolygonOffset() { return offset_; }
    unsigned int		getMode() {return mode_; }

protected:
				~PolygonOffset();

    osg::PolygonOffset*		offset_;
    unsigned int		mode_;

private:
    void			applyAttribute(osg::StateSet*,
					       osg::StateAttribute*) override;

};

};

