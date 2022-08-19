#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "paralleltask.h"

#include "arrayndalgo.h"
#include "enums.h"
#include "factory.h"
#include "odmemory.h"
#include "polygon.h"
#include "rowcol.h"

class TrcKeySampling;
template <class T> class Array2D;
namespace Stats { class CalcSetup; }

/*!
\brief Base class for two dimensional array interpolators.
*/

mExpClass(Algo) Array2DInterpol : public ParallelTask
{ mODTextTranslationClass(Array2DInterpol);
public:
    virtual			~Array2DInterpol();
				mDefineFactoryInClass(Array2DInterpol,factory);

    enum FillType		{ HolesOnly, ConvexHull, Full, Polygon };
				mDeclareEnumUtils(FillType);

    void			setFillType(FillType);
    FillType			getFillType() const;
    void			setRowStep(float r);
    void			setColStep(float r);
    void			setOrigin(const RowCol&);
    void			setSampling(const TrcKeySampling&);
				//!< Set both steps and the origin

    void			setMaxHoleSize(float);
    float			getMaxHoleSize() const;

    void			setClassification(bool);
    bool			isClassification() const;

    void			setMask(const Array2D<bool>*,
					OD::PtrPolicy = OD::UsePtr );
				/*!<If mask is set, interpolation will only
				    occur where mask has 'true' values. If array
				    is larger than mask, values are assumed to
				    be 'false' outside the mask.
				    The mask works together with the filltype in
				    an AND operations, so each position must
				    get a 'true' value both from the filltype
				    AND the mask. If no mask is given, the
				    filltype will be used alone. */

    virtual uiString		infoMsg() const
				{ return uiString::emptyString(); }

    mExpClass(Algo) ArrayAccess
    {
    public:
	virtual			~ArrayAccess()				{}
	virtual void		set(od_int64 target, const od_int64* sources,
				    const float* weights, int nrsrc,
				    bool isclassification)		= 0;
	virtual bool		isDefined(od_int64) const		= 0;
	virtual int		getSize(char dim) const			= 0;
    };

    virtual bool		nothingToFill() const	{ return false; }
    virtual bool		setArray(Array2D<float>&,TaskRunner* =0);
				//!<Set AFTER all settings
    virtual bool		canUseArrayAccess() const { return false; }
    virtual bool		setArray(ArrayAccess&,TaskRunner* =0);
				//!<Set AFTER all settings

				//!<Trend is active only when setTrendOrder
				//!<is called
    void			setTrendOrder(PolyTrend::Order ord);
    bool			trimArray(int step,Array2D<char>& edgesmask);

    void			doPolygonCrop();

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    uiString			uiMessage() const override
				{ return uiStrings::sGridding(); }

    static const char*		sKeyFillType();
    static const char*		sKeyRowStep();
    static const char*		sKeyColStep();
    static const char*		sKeyOrigin();
    static const char*		sKeyNrRows();
    static const char*		sKeyNrCols();
    static const char*		sKeyNrCells();
    static const char*		sKeyMaxHoleSz();
    static const char*		sKeyPolyNrofNodes();
    static const char*		sKeyPolyNode();
    static const char*		sKeyCropPolygon();

protected:
		Array2DInterpol();

    bool	doPrepare(int) override;
    void	getNodesToFill(const bool* isdef, bool* shouldinterpol,
			       TaskRunner*) const;
		/*!<Fills shouldinterpol with true or false depending on if a
		    certain node should be interpolated or not, based on
		    filltype and maxholesize. If isdef is zero, the information
		    will be extracted from the grid. Both isdef and
		    shouldinterpol arrays refers to positions on the grid by
		    row=idx/nrcols_,col=idx%nrcols_ */
    bool	isDefined(int idx) const;
		/*!<idx refers to positions on the grid by
		    row=idx/nrcols_,col=idx%nrcols_ */
    virtual void setFrom(od_int64 target, const od_int64* sources,
			const float* weights, int nrsrc);
		/*!<For convenience, inheriting obj may set arr_ directly. */
    void	floodFillArrFrom(int seed, const bool* isdef,
				 bool* shouldinterpol) const;
		/*!<Floodfills 'false' into shouldinterpol from position seed.
		    Floodfill will stop when bumping into defined values, as
		    provided in isdef. */
    void	excludeBigHoles( const bool* isdef, bool* shouldinterpol )const;
		/*!<Will find holes larger than maxholesize_ and exclude them
		    from shouldinterpol. */


    Array2D<float>*		arr_;
    ArrayAccess*		arrsetter_;
    float			rowstep_;
    float			colstep_;
    int				nrrows_;
    int				nrcols_;
    int				nrcells_;
    RowCol			origin_;

    FillType			filltype_;
    float			maxholesize_;

    const Array2D<bool>*	mask_;
    bool			maskismine_;
    bool			isclassification_;
    Stats::CalcSetup*		statsetup_;

    PolyTrend*			trend_;
    ODPolygon<double>*		poly_;
    bool			croppoly_;
};
