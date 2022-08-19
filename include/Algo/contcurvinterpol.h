#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool		setArray(Array2D<float>&,TaskRunner*) override;
    bool		setArray(ArrayAccess&,TaskRunner*) override;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    virtual bool	initFromArray(TaskRunner*);
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    int			maxNrThreads() const override	{ return 1; }
    od_int64		nrIterations() const override	{ return 1; }
    od_int64		totalNr() const override	{ return totalnr_; }
    uiString		uiNrDoneText() const override
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
