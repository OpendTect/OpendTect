#ifndef sectionextenderimpl_h
#define sectionextenderimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.5 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon3D; };

namespace MPE
{

class Horizon3DExtender : public SectionExtender
{
public:
    				Horizon3DExtender(EM::Horizon3D& surface_,
						const EM::SectionID& sid);

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const CubeSampling&		getExtBoundary() const;

protected:
    BinIDValue			direction;
    EM::Horizon3D&		surface;
};


}; // namespace MPE

#endif

