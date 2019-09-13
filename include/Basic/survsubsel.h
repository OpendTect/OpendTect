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

class Bin2D;
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

    virtual			~SubSel()			{}
    bool			operator ==( const SubSel& oth ) const
				{ return equals( oth ); }
				mImplSimpleIneqOper(SubSel)
    virtual SubSel*		duplicate() const	= 0;
				    //!< clone() would clash with ArrRegSubSel's
    virtual bool		equals(const SubSel&) const	= 0;

    virtual GeomID		geomID() const			= 0;
    virtual bool		is2D() const			= 0;
    virtual totalsz_type	totalSize() const		= 0;
    virtual bool		isAll() const			= 0;
    virtual bool		hasFullRange() const		= 0;

    static bool			getInfo(const IOPar&,bool& is2d,GeomID&);

    bool			is3D() const		{ return !is2D(); }

protected:

    static void			fillParInfo(IOPar&,bool is2d,GeomID);

};


/*!\brief base class for the subselection of the horizontal part of
  2D or 3D geometries */

mExpClass(Basic) HorSubSel : public SubSel
{
public:

    typedef Index_Type		trcnr_type;

    bool			operator ==( const HorSubSel& oth ) const
				{ return equals( oth ); }
				mImplSimpleIneqOper(HorSubSel)
    SubSel*			duplicate() const override;

    LineHorSubSel*		asLineHorSubSel();
    const LineHorSubSel*	asLineHorSubSel() const;
    CubeHorSubSel*		asCubeHorSubSel();
    const CubeHorSubSel*	asCubeHorSubSel() const;

    bool			includes(const BinID&) const;
    bool			includes(const Bin2D&) const;

    static HorSubSel*		get(const TrcKeySampling&);
    static HorSubSel*		create(const IOPar&);
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    virtual trcnr_type		trcNrStart() const		= 0;
    virtual trcnr_type		trcNrStop() const		= 0;
    virtual trcnr_type		trcNrStep() const		= 0;

protected:

    virtual bool		doUsePar(const IOPar&)		= 0;
    virtual void		doFillPar(IOPar&) const		= 0;

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

    bool		operator ==( const GeomSubSel& oth ) const
			{ return equals( oth ); }
			mImplSimpleIneqOper(GeomSubSel)
    SubSel*		duplicate() const override;

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


} // namespace Survey
