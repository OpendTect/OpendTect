#ifndef horizon2dextender_h
#define horizon2dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2006
 RCS:           $Id: horizon2dextender.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
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

