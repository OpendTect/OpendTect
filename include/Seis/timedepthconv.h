#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datapack.h"
#include "multidimstorage.h"
#include "trckeyzsampling.h"
#include "uistring.h"
#include "zaxistransform.h"


class IOObj;
class SeisTrc;
class SeisTrcReader;
class UnitOfMeasure;
class VelocityDesc;
template <class T> class Array3D;
template <class T> class ValueSeries;
namespace Vel { class Worker; }


/*!Base class for ZAxisstretchers that convert between time and depth using
   a 2D/3D velocity model on disk. */

mExpClass(Seis) VelocityStretcher : public ZAxisTransform
{ mODTextTranslationClass(VelocityStretcher);
public:

    bool		setVelData(const MultiID&);
    bool		isOK() const override;

    MultiID		getVelID() const;

    static bool		getRange(const IOPar&,const VelocityDesc&,
				 bool top,Interval<float>&);
    static void		setRange(const Interval<float>&,
				 const VelocityDesc&,bool top,IOPar&);
    static ZSampling	getWorkZSampling(const ZSampling&,
					 const ZDomain::Info& from,
					 const ZDomain::Info& to,const IOPar&,
					 bool makenice=true);
    static ZSampling	getWorkZSampling(const ZSampling&,
					 const ZDomain::Info& from,
					 const ZDomain::Info& to,
					 const Interval<float>& topvavg,
					 const Interval<float>& botvavg,
					 const UnitOfMeasure* vavguom,
					 bool makenice=true);

    const UnitOfMeasure* velUnit() const;
    static Interval<float> getDefaultVAvg(const UnitOfMeasure*);

protected:
			VelocityStretcher(const ZDomain::Def& from,
					  const ZDomain::Def& to,
					  const MultiID&);
			~VelocityStretcher();

private:

    bool		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;

    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				     int,float*) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int,float*) const override;
    void		doTransform(const TrcKey&,const SamplingData<float>&,
				    const ZDomain::Info& sdzinfo,
				    int sz,float*) const;

    void		releaseData();
    int			addVolumeOfInterest(const TrcKeyZSampling&,
					    bool zistrans) override;
    void		setVolumeOfInterest(int,const TrcKeyZSampling&,
					    bool zistrans) override;
    void		removeVolumeOfInterest(int) override;
    bool		loadDataIfMissing(int,TaskRunner* =nullptr) override;

    Interval<float>	getInterval(const BinID&,int voiidx,
				    const ZDomain::Info&) const;

    bool		needsVolumeOfInterest() const override	{ return true; }
    bool		canTransformSurv(OD::GeomSystem) const override
			{ return true; }

    ZSampling		getWorkZSampling(const ZSampling&,
					const ZDomain::Info& from,
					const ZDomain::Info& to) const override;
    static ZSampling	getWorkZSampling(const ZSampling&,
					 const ZDomain::Def& from,
					 const ZDomain::Def& to,const IOPar&,
					 bool makenice=true)	= delete;
    static ZSampling	getWorkZSampling(const ZSampling&,
					 const ZDomain::Def& from,
					 const ZDomain::Def& to,
					 const Interval<float>& topvavg,
					 const Interval<float>& botvavg,
					 const UnitOfMeasure* vavguom,
					 bool makenice=true)	 = delete;

    Interval<float>	getDefaultVAvg() const;

    static const char*	sKeyTopVavg()	{ return "Top Vavg"; }
    static const char*	sKeyBotVavg()	{ return "Bottom Vavg"; }

    ObjectSet<Array3D<float> >		voidata_;
    TypeSet<TrcKeyZSampling>		voivols_;
    ObjectSet<const ZDomain::Info>	voizinfos_;
    ObjectSet<const ZDomain::Info>	voirevzinfos_;
    TypeSet<int>			voiids_;

    SeisTrcReader*			velreader_ = nullptr;
    VelocityDesc&			veldesc_;
    const ZDomain::Info*		velzinfo_ = nullptr;

    Interval<float>	topvavg_; //Used to compute ranges
    Interval<float>	botvavg_; //Used to compute ranges

};


/*!ZAxisstretcher that converts from time to depth (or back) using
   a 2D/3D velocity model on disk. */

mExpClass(Seis) Time2DepthStretcher : public VelocityStretcher
{ mODTextTranslationClass(Time2DepthStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Time2DepthStretcher,
				  "VelocityT2D",
				  toUiString(sFactoryKeyword()));

			Time2DepthStretcher();
			Time2DepthStretcher(const MultiID&);

protected:
			~Time2DepthStretcher();
};


/*!ZAxisstretcher that converts from depth to time (or back) using
   a 2D/3D velocity model on disk*/

mExpClass(Seis) Depth2TimeStretcher : public VelocityStretcher
{ mODTextTranslationClass(Depth2TimeStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Depth2TimeStretcher,
				  "VelocityD2T",
				  toUiString(sFactoryKeyword()));

			Depth2TimeStretcher();
			Depth2TimeStretcher(const MultiID&);

protected:
			~Depth2TimeStretcher();
};


/*! Scans a velocity model for minimum top/bottom average velocity. */

mExpClass(Seis) VelocityModelScanner : public SequentialTask
{ mODTextTranslationClass(VelocityModelScanner);
public:
			VelocityModelScanner(const IOObj&,const VelocityDesc&);
			~VelocityModelScanner();

    uiString		uiMessage() const override	{ return msg_; }
    od_int64		totalNr() const override { return subsel_.totalNr(); }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Position scanned"); }

    const Interval<float>&	getTopVAvg() const	{ return startavgvel_; }
    const Interval<float>&	getBotVAvg() const	{ return stopavgvel_; }
    const UnitOfMeasure* velUnit() const;

private:

    bool		doPrepare(od_ostream*) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream*) override;

    uiString			msg_;
    TrcKeySampling		subsel_;
    TrcKeySamplingIterator	hsiter_;
    bool			definedv0_ = false;
    bool			definedv1_ = false;
    int				nrdone_ = 0;

    const IOObj&		obj_;
    SeisTrcReader*		reader_ = nullptr;
    double			srd_;
    const UnitOfMeasure*	srduom_;
    const VelocityDesc&		veldesc_;
    const ZDomain::Info*	velzinfo_ = nullptr;
    VelocityDesc&		vavgdesc_;

    Interval<float>		startavgvel_;
    Interval<float>		stopavgvel_;
};


mExpClass(Seis) LinearVelTransform : public ZAxisTransform
{ mODTextTranslationClass(LinearVelTransform);
public:

    bool			isOK() const override;
    static const UnitOfMeasure* velUnit();

protected:
				LinearVelTransform(const ZDomain::Def& from,
						   const ZDomain::Def& to,
						   double v0,double k);
				~LinearVelTransform();

private:

    bool			usePar(const IOPar&) override;
    void			fillPar(IOPar&) const override;

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    void			doTransform(const SamplingData<float>& sd,
					    const ZDomain::Info& sdzinfo,
					    int sz,float*) const;

    bool			needsVolumeOfInterest() const override
				{ return false; }
    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    ZSampling			getWorkZSampling(const ZSampling&,
				       const ZDomain::Info& from,
				       const ZDomain::Info& to) const override;

    double			v0_;
    double			k_;
    Vel::Worker*		worker_;

};


mExpClass(Seis) LinearT2DTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearT2DTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransform,
				  "LinearT2D", tr("Linear velocity") );

				LinearT2DTransform(double v0=2000.,double k=0.);

				mDeprecated("Use double FP")
				LinearT2DTransform(float v0,float k);

protected:
				~LinearT2DTransform();
};


mExpClass(Seis) LinearD2TTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearD2TTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearD2TTransform,
				  "LinearD2T", tr("Linear velocity") );

				LinearD2TTransform(double v0=2000.,double k=0.);

				mDeprecated("Use double FP")
				LinearD2TTransform(float v0,float k);

protected:
				~LinearD2TTransform();
};
