#ifndef horizon2dextender_h
#define horizon2dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2006
 RCS:           $Id: horizon2dextender.h,v 1.6 2011-05-02 06:14:48 cvsumesh Exp $
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

    //static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    //static void         	initClass();

    void			setAngleThreshold(float radians);
    float			getAngleThreshold() const;

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction_; }

    int				nextStep();

protected:
    void		addNeighbor(bool upwards, const RowCol& sourcerc );
    virtual const float	getDepth(const RowCol& src,const RowCol& dest);
    virtual void	prepareDataIfRequired() { return; }

    float		anglethreshold_;
    bool		alldirs_;
    Coord		xydirection_;
    BinIDValue		direction_;
    EM::Horizon2D&	surface_;
};


mClass BaseHorizon2DExtender : public Horizon2DExtender
{
public:
    static void			initClass();
    static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    				BaseHorizon2DExtender(EM::Horizon2D&,
						      const EM::SectionID&);
};


}; // namespace MPE

#endif

