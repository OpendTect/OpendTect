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
class SurveyInfo;

namespace OD { namespace JSON { class Object; }; };


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
    bool	sameOutputPosRange(const ZSubSelData&) const;
    bool	includes(z_type) const;
    bool	includes(const ZSubSelData&) const;

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
    z_steprg_type zRange() const		{ return outputZRange(); }
    size_type	inputSize() const		{ return inpzrg_.nrSteps()+1; }
    size_type	outputSize() const		{ return size(); }

    void	setInputZRange(const z_steprg_type&);
    void	setOutputZRange(z_type start,z_type stop,z_type stp);
    inline void	setOutputZRange( const z_steprg_type& rg )
		{ setOutputZRange( rg.start, rg.stop, rg.step ); }
    void	clearSubSel()	{ ArrRegSubSelData::clearSubSel(inputSize()); }

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
			    : ssdata_(rg)	{}
    explicit		ZSubSel(GeomID);
			mImplSimpleEqOpers1Memb(ZSubSel,ssdata_)
    bool		sameOutputPosRange( const ZSubSel& oth ) const
			{ return ssdata_.sameOutputPosRange(oth.ssdata_); }
    bool		includes( const ZSubSel& oth ) const
			{ return ssdata_.includes( oth.ssdata_ ); }

    const ZSubSelData&	zData() const	{ return ssdata_; }
    ZSubSelData&	zData()		{ return ssdata_; }

    // for convenience, we duplicate the ZSubSelData interface

    z_type	zStart() const		{ return ssdata_.zStart(); }
    z_type	zStop() const		{ return ssdata_.zStop(); }
    z_type	zStep() const		{ return ssdata_.zStep(); }
    z_steprg_type inputZRange() const	{ return ssdata_.inputZRange(); }
    z_steprg_type outputZRange() const	{ return ssdata_.outputZRange(); }
    z_steprg_type zRange() const	{ return ssdata_.zRange(); }
    size_type	inputSize() const	{ return ssdata_.inputSize(); }
    size_type	outputSize() const	{ return ssdata_.outputSize(); }

    bool	isAll() const		{ return ssdata_.isAll(); }
    bool	hasFullRange() const	{ return ssdata_.hasFullRange(); }
    void	limitTo( const ZSubSel& oth ) { ssdata_.limitTo(oth.ssdata_); }
    void	limitTo( const z_rg_type& zrg ) { ssdata_.limitTo(zrg); }
    void	widenTo( const ZSubSel& oth ) { ssdata_.widenTo(oth.ssdata_); }
    void	widen( const z_rg_type& zrg ) { ssdata_.widen(zrg); }
    void	merge( const ZSubSel& oth ) { ssdata_.widenTo(oth.ssdata_); }

    idx_type	idx4Z( z_type z ) const { return ssdata_.idx4Z( z ); }
    z_type	z4Idx( idx_type idx ) const { return ssdata_.z4Idx( idx ); }

    void	setInputZRange( const z_steprg_type& rg )
		{ ssdata_.setInputZRange( rg ); }
    void	setOutputZRange( const z_steprg_type& rg )
		{ ssdata_.setOutputZRange( rg ); }
    void	setOutputZRange( z_type start, z_type stop, z_type stp )
		{ ssdata_.setOutputZRange( start, stop, stp ); }
    void	clearSubSel()	    { ssdata_.clearSubSel(); }

    bool	usePar(const IOPar&);
    bool	useJSON(const OD::JSON::Object&);
    void	fillPar(IOPar&) const;
    void	fillJSON(OD::JSON::Object&) const;

    static const ZSubSel&   surv3D(const SurveyInfo* si=nullptr);
    static ZSubSel&	    dummy();

protected:

    ZSubSelData	ssdata_;

		mImplArrRegSubSelClone(ZSubSel)

    SSData&	gtSSData( idx_type ) const override
		{ return mSelf().ssdata_; }

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
    mUseType( Pos,		ZSubSel );
    mUseType( ZSubSel,		idx_type );
    mUseType( ZSubSel,		size_type );
    mUseType( Pos::ZSubSelData,	z_type );
    mUseType( Pos::ZSubSelData,	z_rg_type );
    mUseType( Pos::ZSubSelData,	z_steprg_type );

			FullZSubSel(const SurveyInfo* si=nullptr);
			FullZSubSel(GeomID,const SurveyInfo* si=nullptr);
			FullZSubSel(const GeomIDSet&,
				    const SurveyInfo* si=nullptr);
			FullZSubSel(const ZSubSel&);
			FullZSubSel(const z_steprg_type&);
			FullZSubSel(GeomID,const z_steprg_type&,
					const SurveyInfo* si=nullptr);
			FullZSubSel(const FullZSubSel&);
    FullZSubSel&	operator =(const FullZSubSel&);
    bool		operator ==(const FullZSubSel&) const;
			mImplSimpleIneqOper(FullZSubSel)
    const SurveyInfo&	survInfo() const;

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
    void		setFull(GeomID,const SurveyInfo* si=nullptr);
    void		set(const ZSubSel&);		//!< 3D
    void		set(GeomID,const ZSubSel&);
    void		setEmpty();
    void		remove(idx_type);
    void		remove(GeomID);

    void		merge(const FullZSubSel&);
    void		limitTo(const FullZSubSel&);

    void		fillPar(IOPar&) const;
    void		fillJSON(OD::JSON::Object&) const;
    void		usePar(const IOPar&,const SurveyInfo* si=nullptr);
    void		useJSON(const OD::JSON::Object&,
			        const SurveyInfo* si=nullptr);
    uiString		getUserSummary() const;

protected:

    TypeSet<GeomID>	geomids_;
    TypeSet<ZSubSel>	zsss_;
    const SurveyInfo*	si_			= nullptr;

};

} // namespace Survey
