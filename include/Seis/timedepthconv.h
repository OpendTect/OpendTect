#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "zaxistransform.h"
#include "uistring.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "multidimstorage.h"
#include "veldesc.h"
#include "executor.h"


class IOObj;
class SeisTrc;
class SeisTrcReader;
class UnitOfMeasure;
template <class T> class Array3D;
template <class T> class ValueSeries;
namespace Vel { class Worker; }


/*!Base class for ZAxisstretchers that convert between time and depth using
   a 2D/3D velocity model on disk. */

mExpClass(Seis) VelocityStretcherNew : public ZAxisTransform
{ mODTextTranslationClass(VelocityStretcherNew);
public:

    bool		setVelData(const MultiID&);
    bool		isOK() const override;

    MultiID		getVelID() const;

    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const override;
    ZSampling		getZInterval(bool from,bool makenice) const;

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
			VelocityStretcherNew(const ZDomain::Def& from,
					  const ZDomain::Def& to,
					  const MultiID&);
			~VelocityStretcherNew();

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

    ZSampling		getZInterval(const ZSampling&,
				     const ZDomain::Info& from,
				     const ZDomain::Info& to,
				     bool makenice=true) const;
    ZSampling		getWorkZSampling(const ZSampling&,
					 const ZDomain::Info& from,
					 const ZDomain::Info& to) const;
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

mExpClass(Seis) Time2DepthStretcherNew : public VelocityStretcherNew
{ mODTextTranslationClass(Time2DepthStretcherNew);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Time2DepthStretcherNew,
				  "VelocityT2D",
				  toUiString(sFactoryKeyword()));

			Time2DepthStretcherNew();
			Time2DepthStretcherNew(const MultiID&);

protected:
			~Time2DepthStretcherNew();
};


/*!ZAxisstretcher that converts from depth to time (or back) using
   a 2D/3D velocity model on disk*/

mExpClass(Seis) Depth2TimeStretcherNew : public VelocityStretcherNew
{ mODTextTranslationClass(Depth2TimeStretcherNew);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Depth2TimeStretcherNew,
				  "VelocityD2T",
				  toUiString(sFactoryKeyword()));

			Depth2TimeStretcherNew();
			Depth2TimeStretcherNew(const MultiID&);

protected:
			~Depth2TimeStretcherNew();
};


/*! Scans a velocity model for minimum top/bottom average velocity. */

mExpClass(Seis) VelocityModelScannerNew : public Executor
{ mODTextTranslationClass(VelocityModelScanner);
public:
			VelocityModelScannerNew(const IOObj&,
						const VelocityDesc&);
			~VelocityModelScannerNew();

    uiString		uiMessage() const override	{ return msg_; }
    od_int64		totalNr() const override { return subsel_.totalNr(); }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Position scanned"); }

    const Interval<float>&	getTopVAvg() const	{ return startavgvel_; }
    const Interval<float>&	getBotVAvg() const	{ return stopavgvel_; }
    const UnitOfMeasure* velUnit() const;

private:

    bool		goImpl(od_ostream*,bool,bool,int);
    bool		doPrepare(od_ostream*);
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream*);

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


mExpClass(Seis) LinearVelTransformNew : public ZAxisTransform
{ mODTextTranslationClass(LinearVelTransformNew);
public:

    bool			isOK() const override;
    static const UnitOfMeasure* velUnit();

    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const override;
    ZSampling		getZInterval(bool from,bool makenice) const;

protected:
				LinearVelTransformNew(const ZDomain::Def& from,
						   const ZDomain::Def& to,
						   double v0,double k);
				~LinearVelTransformNew();

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

    ZSampling			getZInterval(const ZSampling&,
					     const ZDomain::Info& from,
					     const ZDomain::Info& to,
					     bool makenice=true) const;
    ZSampling			getWorkZSampling(const ZSampling&,
						 const ZDomain::Info& from,
						 const ZDomain::Info& to) const;

    double			v0_;
    double			k_;
    Vel::Worker*		worker_;

};


mExpClass(Seis) LinearT2DTransformNew : public LinearVelTransformNew
{ mODTextTranslationClass(LinearT2DTransformNew);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransformNew,
				  "LinearT2D", tr("Linear velocity") );

				LinearT2DTransformNew(double v0=2000.,
						      double k=0.);

protected:
				~LinearT2DTransformNew();
};


mExpClass(Seis) LinearD2TTransformNew : public LinearVelTransformNew
{ mODTextTranslationClass(LinearD2TTransformNew);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearD2TTransformNew,
				  "LinearD2T", tr("Linear velocity") );

				LinearD2TTransformNew(double v0=2000.,
						      double k=0.);

protected:
				~LinearD2TTransformNew();
};


// Old classes, kept for compatibility only
// These implementations are not loaded into the ZAxisTransform factory
mStartAllowDeprecatedSection

mExpClass(Seis) VelocityStretcher : public ZAxisTransform
{ mODTextTranslationClass(VelocityStretcher);
public:
    virtual bool		setVelData(const MultiID&)		= 0;

    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    static const char*		sKeyTopVavg()	{ return "Top Vavg"; }
    static const char*		sKeyBotVavg()	{ return "Bottom Vavg"; }

protected:
				VelocityStretcher(const ZDomain::Def& from,
						  const ZDomain::Def& to);
				~VelocityStretcher();
};


/*!ZAxisstretcher that converts from time to depth (or back) using a
   velocity model on disk. */

mExpClass(Seis) Time2DepthStretcher : public VelocityStretcher
{ mODTextTranslationClass(Time2DepthStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Time2DepthStretcher,
				  "VelocityT2D",
				  toUiString(sFactoryKeyword()));

			mDeprecated("Use Time2DepthStretcherNew")
			Time2DepthStretcher();
    bool		setVelData(const MultiID&) override;
    bool		isOK() const override;

    bool		needsVolumeOfInterest() const override	{ return true; }
    int			addVolumeOfInterest(const TrcKeyZSampling&,
					    bool) override;
    void		setVolumeOfInterest(int,
					const TrcKeyZSampling&,bool) override;
    void		removeVolumeOfInterest(int) override;
    bool		loadDataIfMissing(int,TaskRunner* =0) override;
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				  int,float*) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int,float*) const override;
    Interval<float>	getZInterval(bool from) const override;
    float		getGoodZStep() const override;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    MultiID		getZDomainID() const;

    const Interval<float>& getVavgRg(bool start) const;
    mDeprecated("Use VelocityStretcherNew")
    static Interval<float> getDefaultVAvg();

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    friend		class TimeDepthDataLoader;
			~Time2DepthStretcher();
    void		releaseData();
    Interval<float>	getTimeInterval(const BinID&,int voiidx) const;
    Interval<float>	getDepthInterval(const BinID&,int voiidx) const;

    static void				udfFill(ValueSeries<float>&,int);

    ObjectSet<Array3D<float> >		voidata_;
    TypeSet<TrcKeyZSampling>		voivols_;
    BoolTypeSet				voiintime_;
    TypeSet<int>			voiids_;

    SeisTrcReader*			velreader_;
    VelocityDesc			veldesc_;
    bool				velintime_;

    Interval<float>			topvavg_; //Used to compute ranges
    Interval<float>			botvavg_; //Used to compute ranges
};


/*!ZAxisstretcher that converts from depth to time (or back). Uses
   an Time2Depth converter to do the job. */


mExpClass(Seis) Depth2TimeStretcher : public VelocityStretcher
{ mODTextTranslationClass(Depth2TimeStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Depth2TimeStretcher,
				  "VelocityD2T",
				  toUiString(sFactoryKeyword()));

			mDeprecated("Use Depth2TimeStretcherNew")
			Depth2TimeStretcher();
    bool		setVelData(const MultiID&) override;
    bool		isOK() const override;

    bool		needsVolumeOfInterest() const override;
    int			addVolumeOfInterest(const TrcKeyZSampling&,
					    bool) override;
    void		setVolumeOfInterest(int,
					const TrcKeyZSampling&,bool) override;
    void		removeVolumeOfInterest(int) override;
    bool		loadDataIfMissing(int,TaskRunner* =0) override;
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				     int sz,float*) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int sz,float*) const override;
    Interval<float>	getZInterval(bool from) const override;
    float		getGoodZStep() const override;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    MultiID		getZDomainID() const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			~Depth2TimeStretcher();

    RefMan<Time2DepthStretcher>		stretcher_;
};

/*! Scans a velocity model for minimum top/bottom average velocity. */

mExpClass(Seis) VelocityModelScanner : public SequentialTask
{ mODTextTranslationClass(VelocityModelScanner);
public:
			mDeprecated("Use VelocityModelScannerNew")
			VelocityModelScanner(const IOObj&,
				const VelocityDesc&);
			~VelocityModelScanner();

    uiString		uiMessage() const override	{ return msg_; }
    od_int64		totalNr() const override { return subsel_.totalNr(); }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Position scanned"); }

    const Interval<float>&	getTopVAvg() const	{ return startavgvel_; }
    const Interval<float>&	getBotVAvg() const	{ return stopavgvel_; }

    int				nextStep() override;

protected:

    uiString			msg_;
    TrcKeySampling		subsel_;
    TrcKeySamplingIterator	hsiter_;
    bool			definedv0_;
    bool			definedv1_;
    bool			zistime_;
    int				nrdone_;

    const IOObj&		obj_;
    const VelocityDesc&		vd_;

    SeisTrcReader*		reader_;

    Interval<float>		startavgvel_;
    Interval<float>		stopavgvel_;
};


mExpClass(Seis) LinearVelTransform : public ZAxisTransform
{ mODTextTranslationClass(LinearVelTransform);
public:
    bool			usePar(const IOPar&) override;
    void			fillPar(IOPar&) const override;

    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

protected:
				LinearVelTransform(const ZDomain::Def& from,
						   const ZDomain::Def& to,
						   float v0, float dv);
				~LinearVelTransform();
    void			transformT2D(const SamplingData<float>&,
					     int sz,float* res) const;
    void			transformD2T(const SamplingData<float>&,
					     int sz,float* res) const;

    float			startvel_;
    float			dv_;

};


mExpClass(Seis) LinearT2DTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearT2DTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransform,
				  "LinearT2D", tr("Linear velocity") );

				mDeprecated("Use LinearT2DTransformNew")
				LinearT2DTransform(float v0=0, float dv=0);
				~LinearT2DTransform();

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;

    Interval<float>		getZInterval(bool time) const override;
    float			getGoodZStep() const override;

    bool			needsVolumeOfInterest() const override
				{ return false; }
};


mExpClass(Seis) LinearD2TTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearD2TTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearD2TTransform,
				  "LinearD2T", tr("Linear velocity") );

				mDeprecated("Use LinearD2TTransformNew")
				LinearD2TTransform(float v0=0, float dv=0);
				~LinearD2TTransform();

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    Interval<float>		getZInterval(bool depth) const override;
    float			getGoodZStep() const override;

    bool			needsVolumeOfInterest() const override
				{ return false; }
};

mStopAllowDeprecatedSection
