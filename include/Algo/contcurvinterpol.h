#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ding Zheng
 Date:          Apr 2015
 RCS:           $Id: array2dinterpolimpl.h 38585 2015-03-20 10:24:32Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#include "array2dinterpol.h"

mExpClass(Algo) ContinuousCurvatureArray2DInterpol : public Array2DInterpol
{ mODTextTranslationClass(ContinuousCurvatureArray2DInterpol);
public:
		    mDefaultFactoryInstantiation(Array2DInterpol,
			    ContinuousCurvatureArray2DInterpol,
			 "ContinuousCurvature",tr("Continuous curvature"))

			 ContinuousCurvatureArray2DInterpol();
			~ContinuousCurvatureArray2DInterpol();

    bool		setArray(Array2D<float>&,const TaskRunnerProvider&);
    bool		setArray(ArrayAccess&,const TaskRunnerProvider&);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    virtual bool	initFromArray(const TaskRunnerProvider&);
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    int			maxNrThreads() const	{ return 1; }
    od_int64		nrIterations() const	{ return 1; }
    od_int64		totalNr() const		{ return totalnr_; }
    uiString		nrDoneText() const
		       { return tr("Convergence iterations "); }

private:

    bool		fillInputData();
    bool		removePlanarTrend();
    void		recoverPlanarTrend();
    bool		rescaleZValues();
    int			calcGcdEuclid();
    int			calcPrimeFactors(int);
    int			verifyGridSize(int);

    int			getNextGridSize(int curgridsize);
    bool		setCoefficients();
    void		fillInForecast(int,int);
    void		findNearestPoint(int);
    int			doFiniteDifference(int);
    void		finalizeGrid();

    void		updateGridConditions(int);
    void		updateEdgeConditions(int);
    void		updateGridIndex(int);
    bool		updateArray2D();


    static const char*	sKeyConvergence();
    static const char*	sKeyTension();
    static const char*	sKeySearchRadius();

    friend class        HorizonDataComparer;
    friend class        GridInitializer;

    struct HorizonData
    {
	HorizonData(){};
	bool		operator==( const HorizonData& ) const;
	HorizonData&	operator=( const HorizonData& );
	float		x_;
	float		y_;
	float		z_;
	int		index_;
    };

    struct BriggsData
    {
	BriggsData(){};
	BriggsData(double dx,double dy, double z);
	BriggsData& operator=(const BriggsData& brgdata);
	bool	    operator==(const BriggsData& brgdata) const;
	double	    b0_;
	double	    b1_;
	double	    b2_;
	double	    b3_;
	double	    b4_;
	double	    b5_;
    };

    // below function will be used after get correct intersection line between
    //fault and horizon
    void		InterpolatingFault(const TypeSet<HorizonData>&,int);

    int			    totalnr_;
    bool*		    curdefined_;
    bool*		    nodestofill_;
    double		    tension_;
    float		    radius_;
    double		    planec0_;
    double		    planec1_;
    double		    planec2_;
    double		    convergelimit_;
    double		    zscale_;
    double		    zmean_;
    int			    nfact_;
    int			    nrdata_;

    int			    offset_[25][12];
    double		    coeff_[2][12];
    TypeSet<int>	    factors_;
    ArrPtrMan<HorizonData>  hordata_;
    ArrPtrMan<float>	    griddata_;
    ArrPtrMan<char>	    gridstatus_;
    ArrPtrMan<BriggsData>   briggs_;

};
