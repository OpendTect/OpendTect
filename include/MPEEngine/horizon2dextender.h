#ifndef horizon2dextender_h
#define horizon2dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionextender.h"
#include "coord.h"
#include "posinfo2dsurv.h"
#include "binidvalue.h"

namespace EM { class Horizon2D; };

namespace MPE
{

/*!
\brief SectionExtender to extend EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DExtender : public SectionExtender
{
public:
				Horizon2DExtender(EM::Horizon2D&,
					  	  const EM::SectionID&);
    static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    static void         	initClass();

    void			setAngleThreshold(float radians);
    float			getAngleThreshold() const;

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction_; }
    void			setLine2DKey( const PosInfo::Line2DKey& key )
				{ l2dkey_ = key; }
    const PosInfo::Line2DKey&	line2DKey() const	{ return l2dkey_; }

    int				nextStep();

protected:

    void		addNeighbor(bool upwards, const EM::SubID& sourcesid );
    virtual float	getDepth(const EM::SubID& src,
	    			 const EM::SubID& dest) const;
    virtual void	prepareDataIfRequired()		{}

    float		anglethreshold_;
    bool		alldirs_;
    Coord		xydirection_;
    BinIDValue		direction_;
    EM::Horizon2D&	surface_;
    PosInfo::Line2DKey	l2dkey_;
};


}; // namespace MPE

#endif


