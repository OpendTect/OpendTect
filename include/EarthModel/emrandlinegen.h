#ifndef emrandlinegen_h
#define emrandlinegen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: emrandlinegen.h,v 1.8 2009/07/22 16:01:15 cvsbert Exp $
________________________________________________________________________


-*/

#include "ranges.h"
#include "position.h"
class RandomLineSet;
template <class T> class ODPolygon;
namespace Geometry { class RandomLine; class RandomLineSet; }


namespace EM
{
class Horizon3D;
class Horizon3DGeometry;


/*!\brief Creates random lines along the contours of a surface */

mClass RandomLineSetByContourGenerator
{ 
public:

    mStruct Setup
    {
			Setup(bool linezrgisrelative=true);

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
    Setup&		setup()			{ return setup_; }

    void		createLines(Geometry::RandomLineSet&) const;

protected:

    const Horizon3D&	hor_;
    const Horizon3DGeometry& geom_;
    Setup		setup_;

};


/*!\brief Creates random line from another by shifting it */

mClass RandomLineByShiftGenerator
{ 
public:

			RandomLineByShiftGenerator(
				const Geometry::RandomLineSet& rls,
				float d=1, int s=0 )
			    : rls_(rls), dist_(d), side_(s)	{}

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


} // namespace


#endif
