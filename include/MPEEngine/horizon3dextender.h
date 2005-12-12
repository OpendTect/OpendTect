#ifndef sectionextenderimpl_h
#define sectionextenderimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
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

protected:
    bool			addTargetNode(const BinID&,const EM::SubID&,
	    				      float depth);
    BinIDValue			direction;
    EM::Horizon&		surface;
};


}; // namespace MPE

#endif

