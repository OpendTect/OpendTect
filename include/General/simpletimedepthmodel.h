#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "factory.h"
#include "tableascio.h"
#include "tabledef.h"
#include "timedepthmodel.h"
#include "transl.h"
#include "zaxistransform.h"


/*!
\brief Simple Time-Depth model in the form of a Time-Depth table.
*/

mExpClass(General) SimpleTimeDepthModel : public TimeDepthModel
{ mODTextTranslationClass(SimpleTimeDepthModel)
public:
				SimpleTimeDepthModel();
				SimpleTimeDepthModel(const MultiID&);
				~SimpleTimeDepthModel();

    void			setRawData(const TypeSet<float>& times,
					   const TypeSet<float>& depths);

    bool			isOK() const override;
    bool			save(const MultiID&) const;

    const TypeSet<float>&	getRawTimes() const	{ return rawtimes_; }
    const TypeSet<float>&	getRawDepths() const	{ return rawdepths_; }

    static const UnitOfMeasure* getTimeUnit();
    static const UnitOfMeasure* getDepthUnit();

private:

    bool			readFromFile(const char*);
    bool			writeToFile(const char*) const;

    TypeSet<float>		rawtimes_;
    TypeSet<float>		rawdepths_;
};

mDeclEmptyTranslatorBundle(General,SimpleTimeDepthModel,od,"tdm")

/*!
\brief ZAxisTransform that uses a SimpleTimeDepthModel.
*/

mExpClass(General) SimpleTimeDepthTransform : public ZAxisTransform
{ mODTextTranslationClass(SimpleTimeDepthTransform)
public:

    bool			isOK() const override;
    bool			setID(const MultiID&);

protected:
				SimpleTimeDepthTransform(const ZDomain::Def&,
							 const ZDomain::Def&);
				SimpleTimeDepthTransform(const ZDomain::Def&,
							 const ZDomain::Def&,
							 const MultiID&);
				~SimpleTimeDepthTransform();

private:

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    void			doTransform(const SamplingData<float>& sd,
					    const ZDomain::Info& sdzinfo,
					    int sz,float*) const;

    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    bool			needsVolumeOfInterest() const override
				{ return false; }

    ZSampling			getWorkZSampling(const ZSampling&,
				       const ZDomain::Info& from,
				       const ZDomain::Info& to) const override;

    SimpleTimeDepthModel*	tdmodel_ = nullptr;
};


mExpClass(General) SimpleT2DTransform : public SimpleTimeDepthTransform
{ mODTextTranslationClass(SimpleT2DTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, SimpleT2DTransform,
				  "SimpleT2D", toUiString(sFactoryKeyword()));

				SimpleT2DTransform();
				SimpleT2DTransform(const MultiID&);

protected:
				~SimpleT2DTransform();
};


mExpClass(General) SimpleD2TTransform : public SimpleTimeDepthTransform
{ mODTextTranslationClass(SimpleD2TTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, SimpleD2TTransform,
				  "SimpleD2T", toUiString(sFactoryKeyword()));

				SimpleD2TTransform();
				SimpleD2TTransform(const MultiID&);

protected:
				~SimpleD2TTransform();
};



mExpClass(General) SimpleTimeDepthAscIO : public Table::AscIO
{ mODTextTranslationClass(SimpleTimeDepthAscIO)
public:
				SimpleTimeDepthAscIO(const Table::FormatDesc&);
				~SimpleTimeDepthAscIO();

    static Table::FormatDesc*	getDesc(bool withunitfld);
    static void			updateDesc(Table::FormatDesc&,bool withunitfld);
    static void			createDescBody(Table::FormatDesc*,bool unitfld);

    bool			get(od_istream&,SimpleTimeDepthModel&) const;
};
