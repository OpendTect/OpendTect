#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jan 2021
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
    virtual			~SimpleTimeDepthModel() {}

    void			setRawData(const TypeSet<float>& times,
				   const TypeSet<float>& depths);

    bool			save(const MultiID&) const;

    const TypeSet<float>&	getRawTimes() const	{ return rawtimes_; }
    const TypeSet<float>&	getRawDepths() const	{ return rawdepths_; }

    virtual bool		isOK() const;

protected:

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

				SimpleTimeDepthTransform(const ZDomain::Def&,
							 const ZDomain::Def&);
				SimpleTimeDepthTransform(const ZDomain::Def&,
							 const ZDomain::Def&,
							 const MultiID&);

    bool			isOK() const;
    bool			setID(const MultiID&);
    bool			canTransformSurv(OD::GeomSystem) const
				{ return true; }

    bool			needsVolumeOfInterest() const { return false; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

				~SimpleTimeDepthTransform();

    SimpleTimeDepthModel*	tdmodel_ = nullptr;

    void			doTransform(const SamplingData<float>&,
					    int sz,float*,bool) const;
};


mExpClass(General) SimpleT2DTransform : public SimpleTimeDepthTransform
{ mODTextTranslationClass(SimpleT2DTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, SimpleT2DTransform,
				  "SimpleT2D", toUiString(sFactoryKeyword()));

				SimpleT2DTransform();
				SimpleT2DTransform(const MultiID&);

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const;

    float			getGoodZStep() const;
    Interval<float>		getZInterval(bool time) const;

protected:

    Interval<float>		getZRange(bool time) const;
};


mExpClass(General) SimpleD2TTransform : public SimpleTimeDepthTransform
{ mODTextTranslationClass(SimpleD2TTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, SimpleD2TTransform,
				  "SimpleD2T", toUiString(sFactoryKeyword()));

				SimpleD2TTransform();
				SimpleD2TTransform(const MultiID&);

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const;

    float			getGoodZStep() const;
    Interval<float>		getZInterval(bool time) const;

protected:

    Interval<float>		getZRange(bool time) const;
};



mExpClass(General) SimpleTimeDepthAscIO : public Table::AscIO
{ mODTextTranslationClass(SimpleTimeDepthAscIO)
    public:
			SimpleTimeDepthAscIO( const Table::FormatDesc& fd )
				: Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc(bool withunitfld);
    static void			updateDesc(Table::FormatDesc&,bool withunitfld);
    static void			createDescBody(Table::FormatDesc*,bool unitfld);

    bool			get(od_istream&,SimpleTimeDepthModel&) const;
};


