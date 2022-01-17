#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
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

    bool			needsInput() const		{ return false;}
    bool			isInputPrevStep() const		{ return true; }
    bool			areSamplesIndependent() const	{ return true; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			releaseData();
    bool			canInputAndOutputBeSame() const { return true; }
    bool			needsFullVolume() const		{ return false;}

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
    Task*			createTask();

    static const char*		sKeyOldType();

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int nrthreads)	{ return true; }
    bool			computeBinID(const BinID&,int);
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

