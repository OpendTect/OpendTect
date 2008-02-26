#ifndef posfilterstd_h
#define posfilterstd_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilterstd.h,v 1.1 2008-02-26 08:55:18 cvsbert Exp $
________________________________________________________________________


-*/

#include "posfilter.h"


namespace Pos
{

/*!\brief Passes a percentage of the positions */

class RandomFilter
{
public:

			RandomFilter()
			    : passratio_(0.01)	{}
			RandomFilter( const RandomFilter& rf )
			    : passratio_(rf.passratio_)	{}

    float		passratio_;

    static const char*	typeStr();
    static const char*	ratioStr();

protected:

    virtual void	doUsePar(const IOPar&);
    virtual void	doFillPar(IOPar&) const;
    virtual void	mkSummary(BufferString&) const;

    void		doReset()		{ initStats(); }
    void		initStats();
    bool		drawRes() const;
};


#define mStdFiltAddVirtFns \
    virtual const char*	type() const	{ return typeStr(); } \
    virtual bool	initialize()	{ reset(); return true; } \
    virtual void	reset()		{ doReset(); } \
    virtual void	usePar( const IOPar& iop ) \
					{ doUsePar(iop); } \
    virtual void	fillPar( IOPar& iop ) const \
					{ doFillPar(iop); } \
    virtual void	getSummary( BufferString& s ) const \
					{ mkSummary(s); } \
    virtual bool	includes( const Coord&, float z=mUdf(float) ) const \
					{ return drawRes(); }


/*!\brief Passes a percentage of the positions (3D) */

class RandomFilter3D : public Pos::Filter3D
		     , public Pos::RandomFilter
{
public:

    virtual Filter*	clone() const	{ return new RandomFilter3D(*this); }
    virtual bool	is2D() const	{ return false; }

    virtual bool	includes( const BinID&, float z=mUdf(float) ) const
			{ return drawRes(); }

    static void		initClass();
    static Filter3D*	create()	{ return new RandomFilter3D; }

			mStdFiltAddVirtFns

};


/*!\brief Passes a percentage of the positions (2D) */

class RandomFilter2D : public Pos::Filter2D
		     , public Pos::RandomFilter
{
public:

    virtual Filter*	clone() const	{ return new RandomFilter2D(*this); }
    virtual bool	is2D() const	{ return true; }

    virtual bool	includes( int, float z=mUdf(float) ) const
			{ return drawRes(); }

    static void		initClass();
    static Filter2D*	create()	{ return new RandomFilter2D; }


			mStdFiltAddVirtFns
};

/*!\brief Passes each nth position */

class SubsampFilter
{
public:

			SubsampFilter()
			    : each_(2), seqnr_(0)			{}
			SubsampFilter( const SubsampFilter& sf )
			    : each_(sf.each_), seqnr_(sf.seqnr_)	{}

    int			each_;

    static const char*	typeStr();
    static const char*	eachStr();

protected:

    mutable int		seqnr_;

    virtual void	doUsePar(const IOPar&);
    virtual void	doFillPar(IOPar&) const;
    virtual void	mkSummary(BufferString&) const;

    bool		drawRes() const;
    void		doReset()		{ seqnr_ = 0; }
};


/*!\brief Passes each nth position (3D) */

class SubsampFilter3D : public Pos::Filter3D
		      , public Pos::SubsampFilter
{
public:

    virtual Filter*	clone() const	{ return new SubsampFilter3D(*this); }
    virtual bool	is2D() const	{ return false; }

    virtual bool	includes( const BinID&, float z=mUdf(float) ) const
			{ return drawRes(); }

    static void		initClass();
    static Filter3D*	create()	{ return new SubsampFilter3D; }

			mStdFiltAddVirtFns

};


/*!\brief Passes each nth position (2D) */

class SubsampFilter2D : public Pos::Filter2D
		      , public Pos::SubsampFilter
{
public:

    virtual Filter*	clone() const	{ return new SubsampFilter2D(*this); }
    virtual bool	is2D() const	{ return true; }

    virtual bool	includes( int, float z=mUdf(float) ) const
			{ return drawRes(); }

    static void		initClass();
    static Filter2D*	create()	{ return new SubsampFilter2D; }

			mStdFiltAddVirtFns
};


} // namespace

#endif
