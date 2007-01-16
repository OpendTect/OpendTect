#ifndef sectionextenderimpl_h
#define sectionextenderimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.4 2007-01-16 14:26:56 cvsjaap Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon; };

namespace MPE
{

class HorizonExtender : public SectionExtender
{
public:
    				HorizonExtender(EM::Horizon& surface_,
						const EM::SectionID& sid);

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const CubeSampling&		getExtBoundary() const;

protected:
    BinIDValue			direction;
    EM::Horizon&		surface;
};


}; // namespace MPE

#endif

