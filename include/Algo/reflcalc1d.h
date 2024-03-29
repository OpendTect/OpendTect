#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "factory.h"
#include "odcomplex.h"
#include "odcommonenums.h"
#include "paralleltask.h"
#include "reflectivitymodel.h"

class AILayer;
class ElasticLayer;
class ElasticModel;


/*!
\brief Acoustic and Elastic trace computation in 1D.
*/

mExpClass(Algo) ReflCalc1D : public ParallelTask
{ mODTextTranslationClass(ReflCalc1D);
public:
    mDefineFactoryInClass( ReflCalc1D, factory );

			~ReflCalc1D();

    mExpClass(Algo) Setup
    {
    public:
			Setup();
	virtual		~Setup();

	mDefSetupMemb(float,starttime);
	mDefSetupMemb(float,startdepth);
	mDefSetupMemb(ZDomain::DepthType,depthtype);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);

	bool		areDepthsInFeet() const;
    };

    virtual ReflCalc1D::Setup&	setup()			= 0;
    virtual const ReflCalc1D::Setup& setup() const	= 0;

    virtual bool	hasSameParams(const ReflCalc1D&) const;
    virtual bool	isOK() const			{ return true; }
    virtual bool	needsSwave() const		{ return true; }
    virtual bool	needFracRho() const		{ return false; }
    virtual bool	needFracAzi() const		{ return false; }

    bool		setModel(const ElasticModel&);
    const ElasticModel& getModel() const	{ return model_; }
			// model top depth must be TWT = 0ms
			/*!<Note, if either p-wave or s-wave are undef and
			  are undef and required, will fill them with Castagna
			  to compute the reflection coeffs <!*/

    void		setAngle(float thetaangle,Seis::OffsetType);
    void		setAngles(const TypeSet<float>& thetaangles,
				  Seis::OffsetType);
    void		getAngles(TypeSet<float>& thetaangles,
				  Seis::OffsetType=
				  Seis::OffsetType::AngleDegrees) const;
    bool		areDepthsInFeet() const;

    uiString		uiMessage() const override	{ return msg_; }

			//Available after execution
    ConstRefMan<AngleReflectivityModel> getRefModel() const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyReflPar()	   { return "Refl Parameter"; }
    static const char*	sKeyAngle()	   { return "Angle Range"; }
    static const char*	sKeyAngleInDegrees() { return "Angles in Degrees"; }

    static float	sDefAngle(Seis::OffsetType);
    static StepInterval<float> sDefAngleRange(Seis::OffsetType);

    static void		setIOParsToSingleAngle(IOPar&,float angle=0.f,
					   Seis::OffsetType=
					   Seis::OffsetType::AngleDegrees);
    static ReflCalc1D* createInstance(const IOPar&,uiString&,
				      const Setup* =nullptr);
    static ReflCalc1D* createInstance(const IOPar&,const ElasticModel*,
				      uiString&,const Setup* =nullptr);

protected:
			ReflCalc1D();

    od_int64		nrIterations() const override;
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    virtual void	compute(int threadidx,const ElasticLayer& top,
				const ElasticLayer& bottom,float thetaangle,
				float_complex&)			= 0;

			//Setup variables
    ElasticModel&	model_; // model top depth must be TWT = 0ms
    TypeSet<float>	thetaangles_;
    uiString		msg_;

			//Runtime variables
    float*		depths_ = nullptr;
    float*		twt_ = nullptr;
    float_complex**	reflectivities_ = nullptr;

				//Results
    RefMan<AngleReflectivityModel> refmodel_;

public:

    float		getDepth(int layer) const;
    float		getTime(int layer) const;

};


/*!
\brief Acoustic Impedance Calculator
*/

mExpClass(Algo) AICalc1D : public ReflCalc1D
{ mODTextTranslationClass(AICalc);
public:

    mDefaultFactoryInstantiation( ReflCalc1D, AICalc1D,
				  "AICalc",
				  tr("Acoustic Impedance Calculator") );

				AICalc1D();

    ReflCalc1D::Setup&		setup() override	{ return setup_; }
    const ReflCalc1D::Setup&	setup() const override	{ return setup_; }

    bool			needsSwave() const override { return false; }

    static void			computeAI(const AILayer&,
					  const AILayer&,float_complex&);

private:

    void			compute(int threadidx,const ElasticLayer&,
					const ElasticLayer&,float,
					float_complex&) override;

    ReflCalc1D::Setup		setup_;
};
