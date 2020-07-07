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
#include "posidxsubsel.h"

class Bin2D;
class CubeSubSel;
class CubeHorSubSel;
class LineSubSel;
class LineHorSubSel;
class LineSubSelSet;
class LineHorSubSelSet;
class SurveyInfo;
class TrcKeySampling;
class TrcKeyZSampling;

namespace OD { namespace JSON { class Object; }; };

namespace Survey
{

class Geometry;
class Geometry3D;
class Geometry2D;

/*!\brief base class for the subselection of (parts of) 2D or 3D geometries */

mExpClass(Basic) SubSel
{
public:

    mUseType( ArrRegSubSel::SSData,	idx_type );
    mUseType( ArrRegSubSel::SSData,	size_type );
    mUseType( Pos,			GeomID );
    mUseType( Pos,			IdxSubSelData );
    mUseType( IdxSubSelData,		pos_type );
    mUseType( Survey,			Geometry );
    mUseType( Survey,			Geometry2D );
    mUseType( Survey,			Geometry3D );
    typedef od_int64			totalsz_type;

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
    virtual void		clearSubSel()			= 0;

    static bool			getInfo(const IOPar&,bool& is2d,GeomID&);
    static bool			getInfo(const OD::JSON::Object&,bool& is2d,
					GeomID&);

    bool			is3D() const		{ return !is2D(); }

protected:

    static void			fillParInfo(IOPar&,bool is2d,GeomID);
    static void			fillJSONInfo(OD::JSON::Object&,bool is2d,
					     GeomID);

};


/*!\brief base class for the subselection of the horizontal part of
  2D or 3D geometries */

mExpClass(Basic) HorSubSel : public SubSel
{
public:

    typedef pos_type		trcnr_type;
    typedef StepInterval<trcnr_type>	trcnr_steprg_type;

    bool			operator ==( const HorSubSel& oth ) const
				{ return equals( oth ); }
				mImplSimpleIneqOper(HorSubSel)
    HorSubSel*			duplicate() const override;
    const SurveyInfo&		survInfo() const;

    LineHorSubSel*		asLineHorSubSel();
    const LineHorSubSel*	asLineHorSubSel() const;
    CubeHorSubSel*		asCubeHorSubSel();
    const CubeHorSubSel*	asCubeHorSubSel() const;

    bool			includes(const BinID&) const;
    bool			includes(const Bin2D&) const;
    bool			includes(const TrcKey&) const;
    Pos::IdxPair		atGlobIdx(totalsz_type) const;

    static HorSubSel*		get(const TrcKeySampling&);
    static HorSubSel*		create(const IOPar&,
					const SurveyInfo* si=nullptr);
    bool			usePar(const IOPar&);
    bool			useJSON(const OD::JSON::Object&);
    void			fillPar(IOPar&) const;
    void			fillJSON(OD::JSON::Object&) const;

    IdxSubSelData&		trcNrSubSel()
				{ return gtTrcNrSubSel(); }
    const IdxSubSelData&	trcNrSubSel() const
				{ return gtTrcNrSubSel(); }
    trcnr_type			trcNrStart() const
				{ return gtTrcNrSubSel().posStart(); }
    trcnr_type			trcNrStop() const
				{ return gtTrcNrSubSel().posStop(); }
    trcnr_type			trcNrStep() const
				{ return gtTrcNrSubSel().posStep(); }
    trcnr_steprg_type		trcNrRange() const
				{ return gtTrcNrSubSel().outputPosRange(); }
    size_type			trcNrSize() const
				{ return gtTrcNrSubSel().size(); }
    virtual size_type		nrLines() const
				{ return 1; }
    virtual trcnr_steprg_type	lineNrRange() const
				{ const auto lnr = geomID().getI();
				    return trcnr_steprg_type(lnr,lnr,1); }

    pos_type			lineNr4Idx(idx_type) const;
    pos_type			trcNr4Idx(idx_type) const;
    idx_type			idx4LineNr(pos_type) const;
    idx_type			idx4TrcNr(pos_type) const;

protected:

    const SurveyInfo*		si_				= nullptr;

    virtual bool		doUsePar(const IOPar&)			= 0;
    virtual bool		doUseJSON(const OD::JSON::Object&)	= 0;
    virtual void		doFillPar(IOPar&) const			= 0;
    virtual void		doFillJSON(OD::JSON::Object&) const	= 0;

    virtual IdxSubSelData&	gtTrcNrSubSel() const			= 0;

};


mExpClass(Basic) HorSubSelIterator
{
public:

    typedef int		idx_type;

			HorSubSelIterator(HorSubSel&);
			HorSubSelIterator(const HorSubSel&);

    void		toStart()		{ lidx_ = 0; tidx_ = -1; }
    bool		next();

    bool		is2D() const		{ return hss_.is2D(); }
    HorSubSel&		horSubSel()		{ return hss_; }
    const HorSubSel&	horSubSel() const	{ return hss_; }

    BinID		binID() const;
    Bin2D		bin2D() const;
    void		getTrcKey(TrcKey&) const;

protected:

    HorSubSel&		hss_;
    idx_type		lidx_;
    idx_type		tidx_;
    const idx_type	nrtrcs_;

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
    typedef Pos::Distance_Type dist_type;

    bool		operator ==( const GeomSubSel& oth ) const
			{ return equals( oth ); }
			mImplSimpleIneqOper(GeomSubSel)
    bool		includes(const GeomSubSel&) const;
    bool		includes( const BinID& bid ) const
			{ return horSubSel().includes( bid ); }
    bool		includes( const Bin2D& b2d ) const
			{ return horSubSel().includes( b2d ); }
    bool		includes( const TrcKey& tk ) const
			{ return horSubSel().includes( tk ); }
    GeomSubSel*		duplicate() const override;

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
    virtual bool	useJSON(const OD::JSON::Object&);
    virtual void	fillPar(IOPar&) const;
    virtual void	fillJSON(OD::JSON::Object&) const;

    dist_type		trcDist(bool max_else_avg) const;
    void		limitTo(const GeomSubSel&);
    void		merge(const GeomSubSel&);

protected:

			GeomSubSel(const z_steprg_type&);

    ZSubSel	zss_;

    virtual HorSubSel&	gtHorSubSel() const	= 0;

};


} // namespace Survey
