#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Mar 2013
________________________________________________________________________

-*/


#include "array2dinterpolimpl.h"
#include "contcurvinterpol.h"
#include "trckeysampling.h"
#include "multiid.h"
#include "emposid.h"
#include "earthmodelmod.h"

class FaultTrcDataProvider;
class TaskRunner;

namespace EM { class Horizon3D; }

/*!
\brief Base class for Horizon Gridders.
*/

mExpClass(EarthModel) HorizonGridder
{ mODTextTranslationClass(HorizonGridder);
public:
    mDefineFactoryInClass(HorizonGridder,factory);

    virtual		~HorizonGridder();
    uiString		infoMsg() const	{ return infomsg_; }

    void		setFaultIds(const TypeSet<MultiID>&);
    void		getFaultIds(TypeSet<MultiID>&) const;

    virtual void	setTrcKeySampling(const TrcKeySampling&);
    virtual bool	setArray2D(Array2D<float>&,TaskRunner* =0);

    static const char*	sKeyMethod();
    static const char*	sKeyNrFaults();
    static const char*	sKeyFaultID();

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    static uiRetVal	executeGridding(HorizonGridder*,EM::Horizon3D*,
					const BinID& step,
					TaskRunner* t=0);
    static uiRetVal	executeGridding(HorizonGridder*,EM::Horizon3D*,
				const BinID& step,
				const Interval<int>* polyinlrg,
				const Interval<int>* polycrlrg,
				TaskRunner* t = 0);

    static uiRetVal	executeGridding(HorizonGridder* g,EM::Horizon3D* h,
					const EM::SectionID&, const BinID& step,
					TaskRunner* t=0)
			{ return executeGridding(g,h,step,t); }
    static uiRetVal	executeGridding(HorizonGridder* g,EM::Horizon3D* h,
				const EM::SectionID&,const BinID& step,
				const Interval<int>* polyinlrg,
				const Interval<int>* polycrlrg,
				TaskRunner* t = 0)
			{ return executeGridding(g,h,step,polyinlrg,polycrlrg,
						 t); }


protected:

			HorizonGridder();

    FaultTrcDataProvider* fltdataprov_;

    uiString		infomsg_;
    TrcKeySampling	hs_;
    TypeSet<MultiID>	faultids_;

    bool		init(TaskRunner*);
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

    void		setTrcKeySampling(const TrcKeySampling&) override;
    bool		setArray2D(Array2D<float>&,TaskRunner* =0) override;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:

    bool		initFromArray(TaskRunner*) override;
    void		setFrom(od_int64 target, const od_int64* sources,
				const float* weights, int nrsrc) override;
};


mExpClass(EarthModel) TriangulationHor3DGridder
	: public TriangulationArray2DInterpol, public HorizonGridder
{ mODTextTranslationClass(TriangulationHor3DGridder);
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				TriangulationHor3DGridder, "Triangulation",
				 ::toUiString(sFactoryKeyword()))

    void		setTrcKeySampling(const TrcKeySampling&) override;
    bool		setArray2D(Array2D<float>&,TaskRunner* =0) override;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:

    bool		initFromArray(TaskRunner*) override;
    void		setFrom(od_int64 target, const od_int64* sources,
				const float* weights, int nrsrc) override;
};


mExpClass(EarthModel) ExtensionHor3DGridder
	: public ExtensionArray2DInterpol, public HorizonGridder
{ mODTextTranslationClass(ExtensionHor3DGridder);
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				ExtensionHor3DGridder,
				 "Extension", ::toUiString(sFactoryKeyword()))

    void		setTrcKeySampling(const TrcKeySampling&) override;
    bool		setArray2D(Array2D<float>&,TaskRunner* =0) override;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

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

    void		setTrcKeySampling(const TrcKeySampling&) override;
    bool		setArray2D(Array2D<float>&,TaskRunner* =0) override;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

};

