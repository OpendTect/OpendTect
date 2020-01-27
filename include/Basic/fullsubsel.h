#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "cubesubsel.h"
#include "fullhorsubsel.h"
#include "linesubsel.h"
#include "zsubsel.h"


namespace Survey
{


/*!\brief subselection of the 3D geometry or of a set of 2D geometries */

mExpClass(Basic) FullSubSel
{
public:

    mUseType(	Pos,			ZSubSel );
    mUseType(	ZSubSel,		idx_type );
    mUseType(	ZSubSel,		size_type );
    mUseType(	ZSubSel,		z_type );
    mUseType(	ZSubSel,		z_steprg_type );
    mUseType(	Pos,			GeomID );
    mUseType(	Survey,			GeomSubSel );
    mUseType(	Pos::IdxPair,		pos_type );
    typedef	Interval<z_type>	z_rg_type;
    typedef	Interval<pos_type>	pos_rg_type;
    typedef	StepInterval<pos_type>	pos_steprg_type;
    typedef	pos_type		trcnr_type;

			FullSubSel(const SurveyInfo* si=nullptr);
			FullSubSel(GeomID,const SurveyInfo* si=nullptr);
			FullSubSel(const GeomIDSet&,
				    const SurveyInfo* si=nullptr);
			FullSubSel(const CubeSubSel&);
			FullSubSel(const LineSubSel&);
			FullSubSel(const GeomSubSel&);
			FullSubSel(const FullHorSubSel&);
			FullSubSel(const FullHorSubSel&,const FullZSubSel&);
			FullSubSel(const FullZSubSel&);
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
    FullSubSel&		operator =(const FullSubSel&);
    bool		operator ==(const FullSubSel&) const;
			mImplSimpleIneqOper(FullSubSel);

    bool		is2D() const		{ return hss_.is2D(); }
    bool		is3D() const		{ return hss_.is3D(); }

    pos_steprg_type	inlRange() const	{ return hss_.inlRange(); }
    pos_steprg_type	crlRange() const	{ return hss_.crlRange(); }
    pos_steprg_type	trcNrRange( idx_type iln=0 ) const
			{ return hss_.trcNrRange( iln ); }
    z_steprg_type	zRange( idx_type iln=0 ) const
			{ return zss_.zRange( iln ); }
    size_type		nrGeomIDs() const
			{ return hss_.nrGeomIDs(); }
    GeomID		geomID( idx_type iln ) const
			{ return hss_.geomID( iln ); }
    idx_type		indexOf( GeomID gid ) const
			{ return hss_.indexOf( gid ); }
    bool		isPresent( GeomID gid ) const
			{ return hss_.isPresent( gid ); }

    void		setInlRange( const pos_rg_type& rg )
			{ hss_.setInlRange( rg ); }
    void		setCrlRange( const pos_rg_type& rg )
			{ hss_.setCrlRange( rg ); }
    void		setTrcNrRange( const pos_rg_type& rg, idx_type iln=0 )
			{ hss_.setTrcNrRange( rg, iln ); }
    void		setTrcNrRange( GeomID gid, const pos_rg_type& rg )
			{ hss_.setTrcNrRange( gid, rg ); }
    void		setZSubSel( const ZSubSel& zss, idx_type iln=0 )
			{ zss_.get(iln) = zss; }
    void		setZSubSel(GeomID,const ZSubSel&);
    void		setZRange( const z_steprg_type& zrg, idx_type iln=0 )
			{ zss_.get(iln).setOutputZRange( zrg ); }

    HorSubSel&		horSubSel( idx_type iln=0 )
			{ return hss_.horSubSel( iln ); }
    const HorSubSel&	horSubSel( idx_type iln=0 ) const
			{ return hss_.horSubSel( iln ); }
    ZSubSel&		zSubSel( idx_type iln=0 )
			{ return zss_.get( iln ); }
    const ZSubSel&	zSubSel( idx_type iln=0 ) const
			{ return zss_.get( iln ); }
    ZSubSel&		zSubSel( GeomID gid )
			{ return zss_.getFor( gid ); }
    const ZSubSel&	zSubSel( GeomID gid ) const
			{ return zss_.getFor( gid ); }

    CubeHorSubSel	cubeHorSubSel() const;
    CubeSubSel		cubeSubSel() const;
    LineHorSubSel	lineHorSubSel(idx_type iln=0) const;
    LineSubSel		lineSubSel(idx_type iln=0) const;
    LineHorSubSelSet	lineHorSubSelSet() const;
    LineSubSelSet	lineSubSelSet() const;
    GeomSubSel*		getGeomSubSel(idx_type iln=0) const;
    CubeSubSel		subSel3D() const	{ return cubeSubSel(); }
    LineSubSelSet	subSel2D() const	{ return lineSubSelSet(); }
    FullHorSubSel&	fullHorSubSel()		{ return hss_; }
    const FullHorSubSel& fullHorSubSel() const	{ return hss_; }
    FullZSubSel&	fullZSubSel()		{ return zss_; }
    const FullZSubSel&	fullZSubSel() const	{ return zss_; }

    bool		hasFullZRange() const	{ return zss_.hasFullRange(); }
    void		merge(const FullSubSel&);
    void		limitTo(const FullSubSel&);

    bool		isAll() const;
    bool		isFlat() const;
    bool		isZSlice() const;

    void		setEmpty();
    void		setToAll(bool for2d);
    void		setFull(GeomID);
    void		set(const CubeHorSubSel&);
    void		set(const LineHorSubSel&);
    void		set(const LineHorSubSelSet&);
    void		set(const CubeSubSel&);
    void		set(const LineSubSel&);
    void		set(const LineSubSelSet&);
    void		set(const GeomSubSel&);
    void		setGeomID(GeomID);
    void		addGeomID(GeomID);

    uiString		getUserSummary() const;
    size_type		expectedNrPositions() const
			{ return hss_.expectedNrPositions(); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&,const SurveyInfo* si=nullptr);

protected:

    FullHorSubSel	hss_;
    FullZSubSel		zss_;

    void		getLineHorSubSelSet(LineHorSubSelSet&) const;
    void		getLineSubSelSet(LineSubSelSet&) const;
    void		fillFullZSS();
    void		setFromZSS(const FullZSubSel&);
    void		setZSSIfNotPresent(GeomID);
    void		syncZSS();

};


} // namespace Survey
