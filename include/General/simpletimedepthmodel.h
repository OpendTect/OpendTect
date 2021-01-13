#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jan 2021
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "factory.h"
#include "tableascio.h"
#include "tabledef.h"
#include "transl.h"
#include "velocitycalc.h"
#include "zaxistransform.h"


/*!
\brief Simple Time-Depth model in the form of a Time-Depth table.
*/

mExpClass(General) SimpleTimeDepthModel : public TimeDepthModel
{ mODTextTranslationClass(SimpleTimeDepthModel)
public:
			SimpleTimeDepthModel();
			SimpleTimeDepthModel(const MultiID&);
    virtual		~SimpleTimeDepthModel()	{}

    void		setRawData(const TypeSet<float>& times,
	    			   const TypeSet<float>& depths);

    bool		save(const MultiID&) const;

    const TypeSet<float>&	getRawTimes() const	{ return rawtimes_; }
    const TypeSet<float>&	getRawDepths() const	{ return rawdepths_; }

    virtual bool	isOK() const;

protected:

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;

    TypeSet<float>	rawtimes_;
    TypeSet<float>	rawdepths_;

};

mDeclEmptyTranslatorBundle(General,SimpleTimeDepthModel,od,"tdm")

/*!
\brief ZAxisTransform that uses a SimpleTimeDepthModel.
*/

mExpClass(General) SimpleTimeDepthTransform : public ZAxisTransform
{ mODTextTranslationClass(SimpleTimeDepthTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, SimpleTimeDepthTransform,
				  "SimpleT2D", toUiString(sFactoryKeyword()));

				SimpleTimeDepthTransform();
				SimpleTimeDepthTransform(const MultiID&);

    bool			isOK() const;
    bool			setID(const MultiID&);
    void			transformTrc(const TrcKey&,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
	    				      const SamplingData<float>&,
					      int sz,float* res) const;
    bool			canTransformSurv(Pos::SurvID) const
				{ return true; }

    float			getGoodZStep() const;
    Interval<float>		getZInterval(bool time) const;
    bool			needsVolumeOfInterest() const { return false; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

				~SimpleTimeDepthTransform();

    SimpleTimeDepthModel*	tdmodel_;

    Interval<float>		getZRange(bool time) const;

    void			doTransform(const SamplingData<float>&,
					    int sz,float*,bool) const;
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


