#ifndef emhor2dto3d_h
#define emhor2dto3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jan 2007
 RCS:		$Id: emhor2dto3d.h,v 1.3 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________


-*/

#include "executor.h"
#include "bufstring.h"

class HorSampling;
class Array2DInterpolatorPars;
template <class T> class Array2DInterpolator;

namespace EM
{
class Horizon2D;
class Horizon3D;
class Hor2DTo3DSectionData;

class Hor2DTo3D : public Executor
{
public:

				Hor2DTo3D(const Horizon2D&,
					  const HorSampling&,
					  int nrsteps, // < 1 is unlimited
					  Horizon3D&);  
					  // current 3d-hor content is removed
				~Hor2DTo3D();

    int				nextStep();
    const char*			message() const		{ return msg_; }
    const char*			nrDoneText() const;
    int				nrDone() const;

protected:

    const Horizon2D&		hor2d_;
    Horizon3D&			hor3d_;
    BufferString		msg_;
    int				cursectnr_;

    ObjectSet<Hor2DTo3DSectionData>	sd_;
    Array2DInterpolator<float>*		curinterp_;

    void			addSections(const HorSampling&);
    void			fillSections();

};

} // namespace EM

#endif
