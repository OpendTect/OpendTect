#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "arrregsubsel.h"
#include "geomid.h"


namespace Pos
{

/*!\brief array subselection data for Z ranges */

mExpClass(Basic) ZSubSelData : public ArrRegSubSelData
{
public:

    typedef Z_Type			z_type;
    typedef Interval<z_type>		z_rg_type;
    typedef StepInterval<z_type>	z_steprg_type;
    static z_type			zEps()	    { return (z_type)1e-6; }

		ZSubSelData(const z_steprg_type&);
    bool	operator ==(const ZSubSelData&) const;
		mImplSimpleIneqOper(ZSubSelData)

    bool	isAll() const;
    bool	hasFullRange() const;

    idx_type	idx4Z(z_type) const;
    z_type	z4Idx(idx_type) const;

    z_type	zStart() const;
    z_type	zStop() const;
    z_type	zStep() const;

    z_steprg_type inputZRange() const		{ return inpzrg_; }
    z_steprg_type outputZRange() const
		{ return z_steprg_type( zStart(), zStop(), zStep() ); }

    void	setInputZRange(const z_steprg_type&);
    void	setOutputZRange(z_type start,z_type stop,z_type stp);
    inline void	setOutputZRange( const z_steprg_type& rg )
		{ setOutputZRange( rg.start, rg.stop, rg.step ); }

    void	limitTo(const ZSubSelData&);
    void	limitTo(const z_rg_type&);
    void	widenTo(const ZSubSelData&);
    void	widen(const z_rg_type&); // zrg is relative, will be added

protected:

    z_steprg_type	inpzrg_;

    void		ensureSizeOK();

};


/*!\brief Z range subselection directly usable for array subselection. */

mExpClass(Basic) ZSubSel : public ArrRegSubSel1D
{
public:

    mUseType( ZSubSelData,	z_type );
    mUseType( ZSubSelData,	z_rg_type );
    mUseType( ZSubSelData,	z_steprg_type );
    mUseType( Pos,		GeomID );

    explicit		ZSubSel( const z_steprg_type& rg )
			    : data_(rg)	{}
    explicit		ZSubSel(GeomID);
			mImplSimpleEqOpers1Memb(ZSubSel,data_)

    const ZSubSelData&	zData() const	{ return data_; }
    ZSubSelData&	zData()		{ return data_; }

    // for convenience, we duplicate the ZSubSelData interface

    z_type	zStart() const		{ return data_.zStart(); }
    z_type	zStop() const		{ return data_.zStop(); }
    z_type	zStep() const		{ return data_.zStep(); }
    z_steprg_type inputZRange() const	{ return data_.inputZRange(); }
    z_steprg_type outputZRange() const	{ return data_.outputZRange(); }

    bool	isAll() const		{ return data_.isAll(); }
    bool	hasFullRange() const	{ return data_.hasFullRange(); }
    void	limitTo( const ZSubSel& oth ) { data_.limitTo(oth.data_); }
    void	limitTo( const z_rg_type& zrg ) { data_.limitTo(zrg); }
    void	widenTo( const ZSubSel& oth ) { data_.widenTo(oth.data_); }
    void	widen( const z_rg_type& zrg ) { data_.widen(zrg); }
    void	merge( const ZSubSel& oth ) { data_.widenTo(oth.data_); }

    idx_type	idx4Z( z_type z ) const { return data_.idx4Z( z ); }
    z_type	z4Idx( idx_type idx ) const { return data_.z4Idx( idx ); }

    void	setInputZRange( const z_steprg_type& rg )
		{ data_.setInputZRange( rg ); }
    void	setOutputZRange( const z_steprg_type& rg )
		{ data_.setOutputZRange( rg ); }
    void	setOutputZRange( z_type start, z_type stop, z_type stp )
		{ data_.setOutputZRange( start, stop, stp ); }

    bool	usePar(const IOPar&);
    void	fillPar(IOPar&) const;

    static const ZSubSel&   surv3D();
    static ZSubSel&	    dummy();

protected:

    ZSubSelData	data_;

		mImplArrRegSubSelClone(ZSubSel)

    Data&	gtData( idx_type ) const override
		{ return mSelf().data_; }

};

} // namespace Pos


namespace Survey
{

/*!\brief subselection of the z range for a 3D geometry or a set of
  2D geometries */

mExpClass(Basic) FullZSubSel
{
public:

    mUseType( Pos,		GeomID );
    mUseType( Pos::ZSubSel,	ZSubSel );
    mUseType( ZSubSel,		idx_type );
    mUseType( ZSubSel,		size_type );
    mUseType( Pos::ZSubSelData,	z_type );
    mUseType( Pos::ZSubSelData,	z_rg_type );
    mUseType( Pos::ZSubSelData,	z_steprg_type );

			FullZSubSel();				//!< full 3D
			FullZSubSel(GeomID);			//!< full range
			FullZSubSel(const GeomIDSet&);		//!< full ranges
			FullZSubSel(const ZSubSel&);		//!< 3D
			FullZSubSel(const z_steprg_type&);	//!< 3D
			FullZSubSel(GeomID,const z_steprg_type&);
			FullZSubSel(const FullZSubSel&);
    FullZSubSel&	operator =(const FullZSubSel&);
    bool		operator ==(const FullZSubSel&) const;
			mImplSimpleIneqOper(FullZSubSel)

    bool		is2D() const;
    bool		is3D() const		{ return !is2D(); }
    bool		isEmpty() const		{ return zsss_.isEmpty(); }
    size_type		size() const		{ return zsss_.size(); }
    size_type		nrGeomIDs() const	{ return size(); }
    idx_type		indexOf( GeomID g ) const { return geomids_.indexOf(g);}
    bool		isPresent( GeomID g ) const { return indexOf(g)>=0; }
    GeomID		geomID( idx_type idx=0 ) const { return geomids_[idx]; }
    ZSubSel&		get( idx_type idx=0 )	{ return zsss_[idx]; }
    const ZSubSel&	get( idx_type idx=0 ) const { return zsss_[idx]; }
    ZSubSel&		getFor(GeomID);
    const ZSubSel&	getFor( GeomID g ) const { return mSelf().getFor(g); }
    ZSubSel&		first()			{ return zsss_.first(); }
    const ZSubSel&	first() const		{ return zsss_.first(); }
    GeomID		firstGeomID() const	{ return geomids_.first(); }

    bool		isAll() const;
    bool		hasFullRange() const;
    z_steprg_type	zRange( idx_type idx=0 ) const
			{ return get(idx).outputZRange(); }

			// setting the input (and output) range:
    void		setFull(GeomID);
    void		set(const ZSubSel&);		//!< 3D
    void		set(GeomID,const ZSubSel&);
    void		setToNone(bool is2d);
    void		remove(idx_type);
    void		remove(GeomID);

    void		merge(const FullZSubSel&);
    void		limitTo(const FullZSubSel&);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    uiString		getUserSummary() const;

protected:

    TypeSet<GeomID>	geomids_;
    TypeSet<ZSubSel>	zsss_;

};

} // namespace Survey
