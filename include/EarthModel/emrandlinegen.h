#ifndef emrandlinegen_h
#define emrandlinegen_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: emrandlinegen.h,v 1.2 2007-12-24 16:51:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "ranges.h"
class RandomLineSet;
template <class T> class ODPolygon;
namespace Geometry { class RandomLineSet; }


namespace EM
{
class Horizon3D;
class Horizon3DGeometry;


/*!\brief Creates random lines along the contours of a surface */

class RandomLineSetByContourGenerator
{ 
public:

    struct Setup
    {
			Setup(bool linezrgisrelative=true);

	mDefSetupMemb(StepInterval<float>,contzrg) //!< default SI().zRange()
	mDefSetupMemb(Interval<float>,linezrg)	//!< default 30 samples
	mDefSetupMemb(bool,isrel)		//!< default true
	mDefSetupMemb(int,sectionnr)		//!< default -1: all sections
	mDefSetupMemb(const ODPolygon<float>*,selpoly)
						//!< default null: entire survey
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

class RandomLineByShiftGenerator
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

};


} // namespace


#endif
