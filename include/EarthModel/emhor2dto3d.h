#ifndef emhor2dto3d_h
#define emhor2dto3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jan 2007
 RCS:		$Id: emhor2dto3d.h,v 1.5 2008-03-20 21:36:32 cvskris Exp $
________________________________________________________________________


-*/

#include "executor.h"
#include "bufstring.h"
#include "horsampling.h"

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

    class Setup
    {
    public:
				Setup(bool do_gridding);

	mDefSetupMemb(bool,dogrid);
	mDefSetupMemb(HorSampling,hs);	// default from SI()
	mDefSetupMemb(int,nrsteps);	// default 0 => unlimited
	mDefSetupMemb(float,srchrad);	// default 10 * inline distance
    };

				Hor2DTo3D(const Horizon2D&,const Setup&,
					  Horizon3D&);  
					  // current 3d-hor content is removed
				~Hor2DTo3D();

    int				nextStep();
    const char*			message() const		{ return msg_.buf(); }
    const char*			nrDoneText() const;
    int				nrDone() const;

protected:

    const Horizon2D&		hor2d_;
    Horizon3D&			hor3d_;
    BufferString		msg_;
    int				cursectnr_;
    Setup			setup_;

    ObjectSet<Hor2DTo3DSectionData>	sd_;
    Array2DInterpolator<float>*		curinterp_;

    void			addSections(const HorSampling&);
    void			fillSections();

};

} // namespace EM

#endif
