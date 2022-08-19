#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "bufstring.h"
#include "trckeysampling.h"

class TrcKeySampling;
class Array2DInterpol;

namespace EM
{
class Horizon2D;
class Horizon3D;
class Hor2DTo3DSectionData;

/*!
\brief Horizon2D to Horizon3D.
*/

mExpClass(EarthModel) Hor2DTo3D : public Executor
{ mODTextTranslationClass(Hor2DTo3D);
public:
				Hor2DTo3D(const Horizon2D&,Array2DInterpol*,
					  Horizon3D&,TaskRunner* =0);  
				// current 3d-hor content is removed
				// Interpol is taken over, if is 0, only copy 
				// grid 2D to 3D.
				// TaskRunner is only used in constructor
				~Hor2DTo3D();

    int				nextStep() override;
    uiString			uiMessage() const override	{ return msg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;

protected:

    const Horizon2D&		hor2d_;
    Horizon3D&			hor3d_;
    uiString			msg_;
    int				cursectnr_;

    ObjectSet<Hor2DTo3DSectionData>	sd_;
    Array2DInterpol*			curinterp_;

    void			addSections(const TrcKeySampling&);
    void			fillSections();

};

} // namespace EM
