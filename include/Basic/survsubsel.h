#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "geomid.h"
#include "zsubsel.h"

class CubeSubSel;
class CubeHorSubSel;
class LineSubSel;
class LineHorSubSel;
class LineSubSelSet;
class LineHorSubSelSet;
class TrcKeySampling;
class TrcKeyZSampling;


namespace Survey
{

class Geometry;
class Geometry3D;
class Geometry2D;

/*!\brief base class for the subselection of (parts of) 2D or 3D geometries */

mExpClass(Basic) SubSel
{
public:

    mUseType( Pos,		GeomID );
    mUseType( Survey,		Geometry );
    mUseType( Survey,		Geometry2D );
    mUseType( Survey,		Geometry3D );
    typedef od_int64		totalsz_type;

    virtual			~SubSel()		{}
    virtual SubSel*		getCopy() const	= 0;
				    //!< clone() would clash with ArrRegSubSel's

    virtual GeomID		geomID() const		= 0;
    virtual bool		is2D() const		= 0;
    virtual totalsz_type	totalSize() const	= 0;
    virtual bool		isAll() const		= 0;
    virtual bool		hasFullRange() const	= 0;

    static bool			getInfo(const IOPar&,bool& is2d,GeomID&);

protected:

    static void			fillParInfo(IOPar&,bool is2d,GeomID);

};


/*!\brief base class for the subselection of the horizontal part of
  2D or 3D geometries */

mExpClass(Basic) HorSubSel : public SubSel
{
public:

    LineHorSubSel*		asLineHorSubSel();
    const LineHorSubSel*	asLineHorSubSel() const;
    CubeHorSubSel*		asCubeHorSubSel();
    const CubeHorSubSel*	asCubeHorSubSel() const;

    SubSel*			getCopy() const override;
    static HorSubSel*		get(const TrcKeySampling&);
    static HorSubSel*		create(const IOPar&);
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    virtual bool		doUsePar(const IOPar&)	= 0;
    virtual void		doFillPar(IOPar&) const	= 0;

};


/*!\brief base class for the subselection of a 2D or 3D geometry */

mExpClass(Basic) GeomSubSel : public SubSel
{
public:

    mUseType( Pos,	ZSubSel );
    mUseType( Pos,	ZSubSelData );
    mUseType( ZSubSel,	idx_type );
    mUseType( ZSubSel,	size_type );
    mUseType( ZSubSel,	z_type );
    mUseType( ZSubSel,	z_steprg_type );

    SubSel*		getCopy() const override;
    bool		is2D() const override
			{ return gtHorSubSel().is2D(); }
    GeomID		geomID() const override
			{ return gtHorSubSel().geomID(); }
    totalsz_type	totalSize() const override
			{ return gtHorSubSel().totalSize() * zss_.size(); }
    bool		isAll() const override
			{ return gtHorSubSel().isAll() && zss_.isAll(); }
    bool		hasFullRange() const override
			{ return gtHorSubSel().hasFullRange()
			      && zss_.hasFullRange(); }

    LineSubSel*		asLineSubSel();
    const LineSubSel*	asLineSubSel() const;
    CubeSubSel*		asCubeSubSel();
    const CubeSubSel*	asCubeSubSel() const;

    HorSubSel&		horSubSel()		{ return gtHorSubSel();}
    const HorSubSel&	horSubSel() const	{ return gtHorSubSel();}
    const ZSubSel&	zSubSel() const		{ return zss_; }
    ZSubSel&		zSubSel()		{ return zss_; }
    const ZSubSelData&	zSubSelData() const	{ return zss_.zData(); }
    ZSubSelData&	zSubSelData()		{ return zss_.zData(); }

    size_type		nrZ() const
			{ return zss_.size(); }
    z_steprg_type	zRange() const
			{ return zss_.outputZRange(); }
    void		setZRange( const z_steprg_type& rg )
			{ zss_.setOutputZRange( rg ); }
    idx_type		idx4Z( z_type z ) const
			{ return zss_.idx4Z( z ); }
    z_type		z4Idx( idx_type idx ) const
			{ return zss_.z4Idx( idx ); }

    static GeomSubSel*	get(GeomID);
    static GeomSubSel*	get(const TrcKeyZSampling&);
    static GeomSubSel*	create(const IOPar&);
    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

			GeomSubSel(const z_steprg_type&);

    ZSubSel	zss_;

    virtual HorSubSel&	gtHorSubSel() const	= 0;

};


/*!\brief the subselection of the 3D geometry or of the 2D geometries */

mExpClass(Basic) FullSubSel
{
public:

    mUseType( Survey,	GeomSubSel );
    mUseType( Pos,	ZSubSel );
    mUseType( ZSubSel,	idx_type );
    mUseType( ZSubSel,	size_type );
    mUseType( ZSubSel,	z_type );
    mUseType( ZSubSel,	z_steprg_type );
    mUseType( Pos,	GeomID );
    mUseType( Pos::IdxPair,		pos_type );
    typedef Interval<z_type>		z_rg_type;
    typedef Interval<pos_type>		pos_rg_type;
    typedef StepInterval<pos_type>	pos_steprg_type;
    typedef pos_type			trcnr_type;

			FullSubSel();
			FullSubSel(GeomID);
			FullSubSel(const GeomIDSet&);
			FullSubSel(const CubeSubSel&);
			FullSubSel(const LineSubSel&);
			FullSubSel(const GeomSubSel&);
			FullSubSel(const CubeHorSubSel&);
			FullSubSel(const LineHorSubSel&);
			FullSubSel(const LineSubSelSet&);
			FullSubSel(const LineHorSubSelSet&);
			FullSubSel(const BinID&);
			FullSubSel(GeomID,trcnr_type);
			FullSubSel(const TrcKey&);
			FullSubSel(const TrcKeySampling&);
			FullSubSel(const TrcKeyZSampling&);
			FullSubSel(const FullSubSel&);
			FullSubSel(const IOPar&);
    virtual		~FullSubSel();
    void		clearContents();
    FullSubSel&		operator =(const FullSubSel&);

    bool		is2D() const	{ return !css_; }

    pos_steprg_type	inlRange() const;
    pos_steprg_type	crlRange() const;
    pos_steprg_type	trcNrRange(idx_type iln=0) const;
    z_steprg_type	zRange(idx_type i=0) const;
    size_type		nrGeomIDs() const;
    GeomID		geomID(idx_type) const;
    idx_type		indexOf(GeomID) const;

    void		setInlRange(const pos_rg_type&);
    void		setCrlRange(const pos_rg_type&);
    void		setGeomID(GeomID);
    void		addGeomID(GeomID);
    void		setTrcNrRange(const pos_rg_type&,idx_type i=0);
    void		setTrcNrRange(GeomID,const pos_rg_type&);
    void		setZRange(const z_rg_type&,int i=0);
    void		setToAll(bool for2d);

    bool		isAll() const;
    GeomSubSel&		geomSubSel(idx_type i=0);
    const GeomSubSel&	geomSubSel(idx_type i=0) const;
    HorSubSel&		horSubSel(idx_type i=0);
    const HorSubSel&	horSubSel(idx_type i=0) const;
    ZSubSel&		zSubSel(idx_type i=0);
    const ZSubSel&	zSubSel(idx_type i=0) const;
    CubeSubSel&		cubeSubSel()		{ return *css_;}
    const CubeSubSel&	cubeSubSel() const	{ return *css_;}
    LineSubSel&		lineSubSel(idx_type);
    const LineSubSel&	lineSubSel(idx_type) const;
    CubeSubSel&		subSel3D()		{ return *css_; }
    const CubeSubSel&	subSel3D() const	{ return *css_; }
    LineSubSelSet&	subSel2D()		{ return lsss_; }
    const LineSubSelSet& subSel2D() const	{ return lsss_; }
    bool		hasFullZRange() const;
    const LineSubSel*	findLineSubSel(GeomID) const;
    void		merge(const FullSubSel&);
    void		limitTo(const FullSubSel&);

    bool		isFlat() const;
    bool		isZSlice() const;

    void		set(const CubeSubSel&);
    void		set(const LineSubSel&);
    void		set(const LineSubSelSet&);

    uiString		getUserSummary() const;
    size_type		expectedNrTraces() const;
    static const char*	sNrLinesKey();
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    CubeSubSel*		css_	= 0;
    LineSubSelSet&	lsss_;

    void		set3D(bool);

    friend class	FullSubSelPosIter;

public:

    int			selRes3D(const BinID&) const;
    int			selRes2D(GeomID,trcnr_type) const;

};


/*!\brief Iterator for BinID/TrcNr positions for FullSubSel */

mExpClass(Basic) FullSubSelPosIter
{
public:

    mUseType( FullSubSel,	idx_type );
    mUseType( FullSubSel,	GeomID );
    mUseType( FullSubSel,	trcnr_type );

			FullSubSelPosIter(const FullSubSel&);
			FullSubSelPosIter(const FullSubSelPosIter&);

    const FullSubSel&	subSel() const
			{ return subsel_; }

    bool		next();
    void		reset()		{ lineidx_ = trcidx_ = -1; }
    bool		is2D() const	{ return subsel_.is2D(); }

    GeomID		geomID() const;
    trcnr_type		trcNr() const;
    BinID		binID() const;

protected:

    const FullSubSel&	subsel_;
    idx_type		lineidx_	    = -1;
    idx_type		trcidx_		    = -1;

};


} // namespace Survey
