#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"

#include "arrayndimpl.h"
#include "coord.h"
#include "multiid.h"
#include "trckeyzsampling.h"

namespace EM { class EMObject; class Body; class ImplicitBody; }
template <class T> class ODPolygon;

namespace VolProc
{

/*!\brief Body filler */

mExpClass(VolumeProcessing) BodyFiller : public Step
{ mODTextTranslationClass(BodyFiller);
public:
	mDefaultFactoryCreatorImpl( VolProc::Step, BodyFiller );
	mDefaultFactoryInstanciationBase( "BodyFiller",
					tr("Geobody shape painter") );

				BodyFiller();
				~BodyFiller();

    bool			needsInput() const override	{ return false;}
    bool			isInputPrevStep() const override
				{ return true; }

    bool			areSamplesIndependent() const override
				{ return true; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			releaseData() override;
    bool			canInputAndOutputBeSame() const override
				{ return true; }

    bool			needsFullVolume() const override
				{ return false;}

    enum ValueType		{ Constant, PrevStep, Undefined };

    void			setInsideValueType(ValueType);
    ValueType			getInsideValueType() const;
    void			setOutsideValueType(ValueType);
    ValueType			getOutsideValueType() const;

    void			setInsideValue(float);
    float			getInsideValue() const;
    void			setOutsideValue(float);
    float			getOutsideValue() const;

    bool			setSurface(const MultiID&);
    MultiID			getSurfaceID() { return mid_; }
    Task*			createTask() override;

    static const char*		sKeyOldType();

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    bool			prefersBinIDWise() const override
				{ return true; }
    bool			prepareComp(int nrthreads) override
				{ return true; }

    bool			computeBinID(const BinID&,int) override;
    bool			getFlatPlgZRange(const BinID&,
						 Interval<double>& result);

    EM::Body*			body_;
    EM::EMObject*		emobj_;
    EM::ImplicitBody*		implicitbody_;
    MultiID			mid_;

    ValueType			insidevaltype_;
    ValueType			outsidevaltype_;
    float			insideval_;
    float			outsideval_;

				//For flat body_ only, no implicitbody_.
    TrcKeyZSampling		flatpolygon_;
    TypeSet<Coord3>		plgknots_;
    TypeSet<Coord3>		plgbids_;
    char			plgdir_;
				/* inline=0; crosline=1; z=2; other=3 */
    double			epsilon_;
    ODPolygon<double>*		polygon_;

    static const char*		sKeyOldMultiID();
    static const char*		sKeyOldInsideOutsideValue();

    static const char*		sKeyMultiID();
    static const char*		sKeyInsideType();
    static const char*		sKeyOutsideType();
    static const char*		sKeyInsideValue();
    static const char*		sKeyOutsideValue();

};

} // namespace VolProc
