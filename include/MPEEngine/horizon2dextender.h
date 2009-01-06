#ifndef horizon2dextender_h
#define horizon2dextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2006
 RCS:           $Id: horizon2dextender.h,v 1.4 2009-01-06 10:48:18 cvsranojay Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon2D; };

namespace MPE
{

mClass Horizon2DExtender : public SectionExtender
{
public:
    			Horizon2DExtender(EM::Horizon2D&,
					  const EM::SectionID&);

    void		setAngleThreshold(float radians);
    float		getAngleThreshold() const;

    void		setDirection(const BinIDValue&);
    const BinIDValue*	getDirection() const { return &direction_; }

    int			nextStep();

protected:
    void		addNeighbor(bool upwards, const RowCol& sourcerc );

    float		anglethreshold_;
    bool		alldirs_;
    Coord		xydirection_;
    BinIDValue		direction_;
    EM::Horizon2D&	surface_;
};


}; // namespace MPE

#endif

