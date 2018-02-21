#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Jan 2010
________________________________________________________________________

-*/

#include "emcommon.h"
#include "executor.h"
#include "uistrings.h"

class ZAxisTransform;

namespace EM
{
class ObjectIterator;
class Horizon;

/*!\brief Horizon z-axis transformer */

mExpClass(EarthModel) HorizonZTransformer : public Executor
{ mODTextTranslationClass(HorizonZTransformer);
public:
			HorizonZTransformer(const ZAxisTransform&,
					    const Horizon&,bool isforward);
    virtual		~HorizonZTransformer();

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		nrDoneText() const
			{ return uiStrings::sPositionsDone(); }

    void		setOutputHorizon(Horizon&);
    void		setReferenceZ(float z);

protected:

    int				nextStep();
    od_int64			nrdone_;
    od_int64			totalnr_;

    ObjectIterator*		iter_;

    const Horizon&		tarhor_;
    const ZAxisTransform&	zat_;
    Horizon*			outputhor_;
    bool			isforward_;
    float			refz_;
};

} // namespace EM
