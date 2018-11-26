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


namespace Pos
{

/*!\brief array subselection data for Z ranges */

mExpClass(Basic) ZSubSelData : public ArrRegSubSelData
{
public:

    typedef Z_Type			z_type;
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

protected:

    z_steprg_type	inpzrg_;

    void		ensureSizeOK();

};


/*!\brief Z range subselection directly usable for array subselection. */

mExpClass(Basic) ZSubSel : public ArrRegSubSel1D
{
public:

    mUseType( ZSubSelData,	z_type );
    mUseType( ZSubSelData,	z_steprg_type );

		ZSubSel( const z_steprg_type& rg )
		    : data_(rg)					{}
		mImplSimpleEqOpers1Memb(ZSubSel,data_)
		mImplArrRegSubSelClone(ZSubSel)

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

    idx_type	idx4Z( z_type z ) const { return data_.idx4Z( z ); }
    z_type	z4Idx( idx_type idx ) const { return data_.z4Idx( idx ); }

    void	setInputZRange( const z_steprg_type& rg )
		{ data_.setInputZRange( rg ); }
    void	setOutputZRange( const z_steprg_type& rg )
		{ data_.setOutputZRange( rg ); }
    void	setOutputZRange( z_type start, z_type stop, z_type stp )
		{ data_.setOutputZRange( start, stop, stp ); }

protected:

    ZSubSelData	data_;

    Data&	gtData( idx_type ) const override
		{ return mSelf().data_; }

};

} // namespace Pos
