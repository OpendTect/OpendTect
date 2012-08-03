#ifndef emhorizonztransformer_h
#define emhorizonztransformer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Jan 2010
 RCS:		$Id: emhorizonztransformer.h,v 1.4 2012-08-03 13:00:18 cvskris Exp $
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "position.h"

class ZAxisTransform;

namespace EM
{
class EMObjectIterator;
class Horizon;

mClass(EarthModel) HorizonZTransformer : public Executor
{
public:
			HorizonZTransformer(const ZAxisTransform&,
					    const Horizon&,bool isforward);
    virtual		~HorizonZTransformer();

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    const char*		nrDoneText() const	{ return "Positions done"; }

    void		setOutputHorizon(Horizon&);
    void		setReferenceZ(float z);

protected:

    int				nextStep();
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

#endif

