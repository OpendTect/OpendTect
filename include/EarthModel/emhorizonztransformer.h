#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		Jan 2010
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"

class ZAxisTransform;

namespace EM
{
class EMObjectIterator;
class Horizon;

/*!
\brief %Horizon z-axis transformer
*/

mExpClass(EarthModel) HorizonZTransformer : public Executor
{ mODTextTranslationClass(HorizonZTransformer);
public:
			HorizonZTransformer(const ZAxisTransform&,
					    const Horizon&,bool isforward);
    virtual		~HorizonZTransformer();

    od_int64		totalNr() const override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
						{ return tr("Positions done"); }

    void		setOutputHorizon(Horizon&);
    void		setReferenceZ(float z);

protected:

    int				nextStep() override;
    od_int64			nrdone_;
    od_int64			totalnr_;

    EMObjectIterator*		iter_;

    const Horizon&		tarhor_;
    const ZAxisTransform&	zat_;
    Horizon*			outputhor_;
    bool			isforward_;
    float			refz_;
};

} // namespace EM

