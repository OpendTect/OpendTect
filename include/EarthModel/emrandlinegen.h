#ifndef emrandlinegen_h
#define emrandlinegen_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: emrandlinegen.h,v 1.1 2007-11-15 16:53:48 cvsbert Exp $
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

class RandomLineSetGenerator
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

			RandomLineSetGenerator(const Horizon3D&,const Setup&);
    Setup&		setup()			{ return setup_; }

    void		createLines(Geometry::RandomLineSet&) const;

protected:

    const Horizon3D&	hor_;
    const Horizon3DGeometry& geom_;
    Setup		setup_;

};

} // namespace


#endif
