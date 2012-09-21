#ifndef rangeposprovider_h
#define rangeposprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________


-*/

#include "generalmod.h"
#include "posprovider.h"


namespace Pos
{

/*!\brief 3D provider based on CubeSampling */

mClass(General) RangeProvider3D : public Provider3D
{
public:

			RangeProvider3D();
			RangeProvider3D(const RangeProvider3D&);
			~RangeProvider3D();
    RangeProvider3D&	operator =(const RangeProvider3D&);
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider3D(*this); }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID& start,BinID& stop) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    CubeSampling&	sampling()		{ return cs_; }
    const CubeSampling&	sampling() const	{ return cs_; }

    virtual bool	includes( const Coord& c, float z=mUdf(float) ) const
			{ return Pos::Provider3D::includes(c,z); }

protected:

    CubeSampling&	cs_;
    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new RangeProvider3D; }

};


/*!\brief 2D provider based on StepInterval<int>.

Can only be used if Line2DData is filled.

 */

mClass(General) RangeProvider2D : public Provider2D
{
public:

			RangeProvider2D();
			RangeProvider2D(const RangeProvider2D&);
    RangeProvider2D&	operator =(const RangeProvider2D&);
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider2D(*this); }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual int		curNr() const;
    virtual float	curZ() const		{ return curz_; }
    virtual Coord	curCoord() const;
    virtual bool	includes(int,float z=mUdf(float),int lidx=0) const;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent( Interval<int>& rg, int lidx=-1 ) const;
    virtual void	getZRange( Interval<float>& rg, int lidx ) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    StepInterval<int>&		trcRange(int lidx)	{ return trcrgs_[lidx];}
    const StepInterval<int>&	trcRange(int lidx) const {return trcrgs_[lidx];}
    
    StepInterval<float>&	zRange() 		{ return zrg_; }
    const StepInterval<float>&	zRange() const		{ return zrg_; };

protected:

    TypeSet< StepInterval<int> > trcrgs_;
    StepInterval<float>	zrg_;
    int			curtrcidx_;
    int			curlineidx_;
    float		curz_;

public:

    static void		initClass();
    static Provider2D*	create()	{ return new RangeProvider2D; }

};


} // namespace

#endif

