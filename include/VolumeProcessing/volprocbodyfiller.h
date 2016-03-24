#ifndef volprocbodyfiller_h
#define volprocbodyfiller_h

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
					tr("Body shape painter") );

				BodyFiller();
				~BodyFiller();
    virtual void		releaseData();

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

    static const char*		sKeyOldType();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual Task*		createTask();

    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const { return true; }
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		needsInput() const		{ return false;}
    virtual bool		isInputPrevStep() const		{ return true; }
    virtual bool		prefersBinIDWise() const	{ return true; }

protected:

    virtual bool		prepareComp(int nrthreads)	{ return true; }
    virtual bool		computeBinID(const BinID&,int);
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						const TrcKeySampling&,
						const StepInterval<int>&) const;
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

#endif
