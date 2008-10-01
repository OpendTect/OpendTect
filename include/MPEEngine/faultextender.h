#ifndef faultextender_h
#define faultextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faultextender.h,v 1.2 2008-10-01 03:44:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"

#include "position.h"

namespace EM { class Fault3D; };

namespace MPE
{

class FaultExtender : public SectionExtender
{
public:
			FaultExtender(EM::Fault3D&,const EM::SectionID& =-1);

    const Coord3&	maxDistance() const;
    void		setMaxDistance(const Coord3&);

    virtual void	setDirection(const BinIDValue&);
    const BinIDValue*	getDirection() const { return &direction; }

    int			nextStep();

protected:
    BinIDValue		direction;
    EM::Fault3D&	fault;

    Coord3		maxdistance;
};

}; // namespace MPE

#endif

