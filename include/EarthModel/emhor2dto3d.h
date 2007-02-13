#ifndef emhor2dto3d_h
#define emhor2dto3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jan 2007
 RCS:		$Id: emhor2dto3d.h,v 1.2 2007-02-13 13:14:36 cvsjaap Exp $
________________________________________________________________________


-*/

#include "executor.h"
#include "bufstring.h"

class HorSampling;
class Array2DInterpolatorPars;
template <class T> class Array2DInterpolator;

namespace EM
{
class Horizon;
class Horizon2D;
class Hor2DTo3DSectionData;

class Hor2DTo3D : public Executor
{
public:

				Hor2DTo3D(const EM::Horizon2D&,
					  const HorSampling&,
					  int nrsteps, // < 1 is unlimited
					  EM::Horizon&);  
					  // current 3d-hor content is removed
				~Hor2DTo3D();

    int				nextStep();
    const char*			message() const		{ return msg_; }
    const char*			nrDoneText() const;
    int				nrDone() const;

protected:

    const EM::Horizon2D&	hor2d_;
    EM::Horizon&		hor3d_;
    BufferString		msg_;
    int				cursectnr_;

    ObjectSet<Hor2DTo3DSectionData>	sd_;
    Array2DInterpolator<float>*		curinterp_;

    void			addSections(const HorSampling&);
    void			fillSections();

};

} // namespace

#endif
