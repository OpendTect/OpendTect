#ifndef emhor2dto3d_h
#define emhor2dto3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2007
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "bufstring.h"
#include "horsampling.h"

class HorSampling;
class Array2DInterpol;

namespace EM
{
class Horizon2D;
class Horizon3D;
class Hor2DTo3DSectionData;

/*!
\ingroup EarthModel
\brief Horizon2D to Horizon3D.
*/

mExpClass(EarthModel) Hor2DTo3D : public Executor
{
public:
				Hor2DTo3D(const Horizon2D&,Array2DInterpol*,
					  Horizon3D&,TaskRunner* =0);  
				// current 3d-hor content is removed
				// Interpol is taken over, if is 0, only copy 
				// grid 2D to 3D.
				// TaskRunner is only used in constructor
				~Hor2DTo3D();

    int				nextStep();
    const char*			message() const		{ return msg_.buf(); }
    const char*			nrDoneText() const;
    od_int64			nrDone() const;
    od_int64			totalNr() const;

protected:

    const Horizon2D&		hor2d_;
    Horizon3D&			hor3d_;
    BufferString		msg_;
    int				cursectnr_;

    ObjectSet<Hor2DTo3DSectionData>	sd_;
    Array2DInterpol*			curinterp_;

    void			addSections(const HorSampling&);
    void			fillSections();

};

} // namespace EM

#endif

