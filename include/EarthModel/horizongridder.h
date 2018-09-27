#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Mar 2013
________________________________________________________________________

-*/


#include "emcommon.h"
#include "array2dinterpolimpl.h"
#include "contcurvinterpol.h"
#include "trckeysampling.h"
#include "dbkey.h"

class FaultTrcDataProvider;
class TaskRunnerProvider;

namespace EM { class Horizon3D; }

/*!\brief Base class for Horizon Gridders. */

mExpClass(EarthModel) HorizonGridder
{ mODTextTranslationClass(HorizonGridder);
public:

    mDefineFactoryInClass(HorizonGridder,factory);

    virtual		~HorizonGridder();
    uiString		infoMsg() const	{ return infomsg_; }

    void		setFaultIds(const DBKeySet&);
    void		getFaultIds(DBKeySet&) const;

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,const TaskRunnerProvider&);

    static const char*	sKeyMethod();
    static const char*	sKeyNrFaults();
    static const char*	sKeyFaultID();

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    static uiRetVal	executeGridding(HorizonGridder*,EM::Horizon3D*,
				const BinID& step,
				const TaskRunnerProvider&,
				const Interval<int>* polyinlrg = 0,
				const Interval<int>* polycrlrg = 0);

protected:

			HorizonGridder();

    FaultTrcDataProvider* fltdataprov_;

    uiString	        infomsg_;
    TrcKeySampling	hs_;
    DBKeySet	faultids_;

    bool		init(const TaskRunnerProvider&);
    bool		blockSrcPoints(const float*,const od_int64*,int,
				       ObjectSet< TypeSet<int> >&) const;
    bool		setFrom(float*,od_int64,const od_int64*,
				const float*,int nrsrc);
};


mExpClass(EarthModel) InvDistHor3DGridder
	: public InverseDistanceArray2DInterpol, public HorizonGridder
{ mODTextTranslationClass(InvDistHor3DGridder);
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				InvDistHor3DGridder,
				"InverseDistance",
				tr("Inverse distance") )

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,const TaskRunnerProvider&);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		initFromArray(const TaskRunnerProvider&);
    void		setFrom(od_int64 target, const od_int64* sources,
	                        const float* weights, int nrsrc);
};


mExpClass(EarthModel) TriangulationHor3DGridder
	: public TriangulationArray2DInterpol, public HorizonGridder
{ mODTextTranslationClass(TriangulationHor3DGridder);
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				TriangulationHor3DGridder, "Triangulation",
				 ::toUiString(sFactoryKeyword()))

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,const TaskRunnerProvider&);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		initFromArray(const TaskRunnerProvider&);
    void		setFrom(od_int64 target, const od_int64* sources,
	                        const float* weights, int nrsrc);
};


mExpClass(EarthModel) ExtensionHor3DGridder
	: public ExtensionArray2DInterpol, public HorizonGridder
{ mODTextTranslationClass(ExtensionHor3DGridder);
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				ExtensionHor3DGridder,
				 "Extension", ::toUiString(sFactoryKeyword()))

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,const TaskRunnerProvider&);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

};


mExpClass(EarthModel) ContinuousCurvatureHor3DGridder
    : public ContinuousCurvatureArray2DInterpol,public HorizonGridder
{mODTextTranslationClass(ContinuousCurvatureHor3DGridder);
public:

			 mDefaultFactoryInstantiation(HorizonGridder,
			     ContinuousCurvatureHor3DGridder,
			     "ContinuousCurvature",
			     tr("Continuous curvature") )

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,const TaskRunnerProvider&);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

};
