#ifndef faultextender_h
#define faultextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faultextender.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"

#include "position.h"

namespace EM { class Fault; };

namespace MPE
{

class FaultExtender : public SectionExtender
{
public:
			FaultExtender(EM::Fault&,const EM::SectionID& =-1);

    const Coord3&	maxDistance() const;
    void		setMaxDistance(const Coord3&);

    virtual void	setDirection(const BinIDValue&);
    const BinIDValue*	getDirection() const { return &direction; }

    int			nextStep();

protected:
    BinIDValue		direction;
    EM::Fault&		fault;

    Coord3		maxdistance;
};

}; // namespace MPE

#endif

