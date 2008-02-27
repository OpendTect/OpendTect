#ifndef posfilterstd_h
#define posfilterstd_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilterstd.h,v 1.2 2008-02-27 14:36:36 cvsbert Exp $
________________________________________________________________________


-*/

#include "posfilter.h"


namespace Pos
{

/*!\brief Passes a percentage of the positions */

class RandomFilter : public virtual Pos::Filter
{
public:

			RandomFilter()
			    : passratio_(0.01)	{}
			RandomFilter( const RandomFilter& rf )
			    : passratio_(rf.passratio_)	{}

    bool		initialize()	{ reset(); return true; }
    void		reset()		{ initStats(); }

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    float		passratio_;

    static const char*	typeStr();
    static const char*	ratioStr();

protected:

    void		initStats();
    bool		drawRes() const;
};


#define mSimpPosFilterDefFnsBase \
virtual const char* type() const { return typeStr(); } \
virtual bool includes(const Coord&,float z=1e30) const { return drawRes(); } \
static void initClass()

#define mSimpPosFilterDefFns3D(clssnm) \
virtual bool includes(const BinID&,float z=1e30) const { return drawRes(); } \
virtual bool is2D() const	{ return false; } \
virtual Filter*	clone() const	{ return new clssnm##Filter3D(*this); } \
static Filter3D* create()	{ return new clssnm##Filter3D; } \
mSimpPosFilterDefFnsBase

#define mSimpPosFilterDefFns2D(clssnm) \
virtual bool includes(int,float z=1e30) const { return drawRes(); } \
virtual bool is2D() const	{ return false; } \
virtual Filter*	clone() const	{ return new clssnm##Filter2D(*this); } \
static Filter2D* create()	{ return new clssnm##Filter2D; } \
mSimpPosFilterDefFnsBase


/*!\brief Passes a percentage of the positions (3D) */

class RandomFilter3D : public Pos::Filter3D
		     , public Pos::RandomFilter
{
public:

    mSimpPosFilterDefFns3D(Random);

};


/*!\brief Passes a percentage of the positions (2D) */

class RandomFilter2D : public Pos::Filter2D
		     , public Pos::RandomFilter
{
public:

    mSimpPosFilterDefFns2D(Random);

};

/*!\brief Passes each nth position */

class SubsampFilter : public virtual Pos::Filter
{
public:

			SubsampFilter()
			    : each_(2), seqnr_(0)			{}
			SubsampFilter( const SubsampFilter& sf )
			    : each_(sf.each_), seqnr_(sf.seqnr_)	{}

    void		reset()		{ seqnr_ = 0; }

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    int			each_;

    static const char*	typeStr();
    static const char*	eachStr();

protected:

    mutable int		seqnr_;

    bool		drawRes() const;
};


/*!\brief Passes each nth position (3D) */

class SubsampFilter3D : public Pos::Filter3D
		      , public Pos::SubsampFilter
{
public:

    mSimpPosFilterDefFns3D(Subsamp);

};


/*!\brief Passes each nth position (2D) */

class SubsampFilter2D : public Pos::Filter2D
		      , public Pos::SubsampFilter
{
public:

    mSimpPosFilterDefFns2D(Subsamp);

};


} // namespace

#endif
