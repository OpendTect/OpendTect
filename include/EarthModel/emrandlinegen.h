#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "earthmodelmod.h"
#include "ranges.h"
#include "coord.h"
class RandomLineSet;
template <class T> class ODPolygon;
namespace Geometry { class RandomLine; class RandomLineSet; }


namespace EM
{
class Horizon3D;
class Horizon3DGeometry;

/*!
\brief Creates random lines along the contours of a surface.
*/

mExpClass(EarthModel) RandomLineSetByContourGenerator
{
public:

    mStruct(EarthModel) Setup
    {
			Setup(bool linezrgisrelative=true);
			~Setup();

	mDefSetupMemb(StepInterval<float>,contzrg) //!< default SI().zRange()
	mDefSetupMemb(Interval<float>,linezrg)	//!< default 30 samples
	mDefSetupMemb(bool,isrel)		//!< default true
	mDefSetupMemb(int,sectionnr)		//!< default -1: all sections
	mDefSetupMemb(const ODPolygon<float>*,selpoly)
						//!< default null: entire survey
	mDefSetupMemb(int,nrlargestonly)	//!< default -1: all contours
	mDefSetupMemb(int,minnrvertices)	//!< default 2
    };

			RandomLineSetByContourGenerator(const Horizon3D&,
							const Setup&);
			~RandomLineSetByContourGenerator();

    Setup&		setup()			{ return setup_; }

    void		createLines(Geometry::RandomLineSet&) const;

protected:

    const Horizon3D&	hor_;
    const Horizon3DGeometry& geom_;
    Setup		setup_;

};


/*!
\brief Creates random line from another by shifting it.
*/

mExpClass(EarthModel) RandomLineByShiftGenerator
{
public:
			RandomLineByShiftGenerator(
				const Geometry::RandomLineSet& rls,
				float dist=1,int side=0);
			~RandomLineByShiftGenerator();

    const Geometry::RandomLineSet& rls_;
    float		dist_;
    int			side_; //!, -1=left, 0=both, 1=right

    void		generate(Geometry::RandomLineSet&,
	    			 int linenr_in_inp_set=0) const;

protected:

    void		crLine(const Geometry::RandomLine&,bool,
				Geometry::RandomLineSet&) const;
    bool		getShifted(Coord,Coord,Coord&,Coord&,bool) const;
    bool		getIntersection(Coord,Coord,Coord,Coord,Coord&) const;

};

} // namespace EM
