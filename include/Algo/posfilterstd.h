#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________


-*/

#include "algomod.h"

#include "posfilter.h"

namespace Stats { class RandomGenerator; }


namespace Pos
{

/*!
\brief Passes a percentage of the positions.
*/

mExpClass(Algo) RandomFilter : public virtual Filter
{
public:

			RandomFilter();
			RandomFilter(const RandomFilter&);
			~RandomFilter();

    RandomFilter&	operator =(const RandomFilter&);

    bool		initialize(TaskRunner* =nullptr) override;
    void		reset() override		{ initStats(); }

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;
    float		estRatio(const Provider&) const	override
			{ return passratio_; }

    float		passratio_ = 0.01f;

    static const char*	typeStr();
    static const char*	ratioStr();

protected:

    void		initStats();
    bool		drawRes() const;

private:

    Stats::RandomGenerator& gen_;
};


#define mSimpPosFilterDefFnsBase \
const char* factoryKeyword() const override { return type(); } \
const char* type() const override { return typeStr(); } \
bool includes(const Coord&,float z=1e30) const override { return drawRes(); } \
static void initClass()

#define mSimpPosFilterDefFns3D(clssnm) \
bool includes(const BinID&,float z=1e30) const override { return drawRes(); } \
bool is2D() const override	{ return false; } \
Filter*	clone() const override	{ return new clssnm##Filter3D(*this); } \
static Filter3D* create()	{ return new clssnm##Filter3D; } \
mSimpPosFilterDefFnsBase

#define mSimpPosFilterDefFns2D(clssnm) \
bool includes(int,float z=1e30,int nr=0) const override { return drawRes(); } \
bool is2D() const override	{ return false; } \
Filter*	clone() const override	{ return new clssnm##Filter2D(*this); } \
static Filter2D* create()	{ return new clssnm##Filter2D; } \
mSimpPosFilterDefFnsBase


/*!
\brief Passes a percentage of the positions (3D).
*/

mExpClass(Algo) RandomFilter3D : public Filter3D
		     , public RandomFilter
{
public:

    mSimpPosFilterDefFns3D(Random);

};


/*!
\brief Passes a percentage of the positions (2D).
*/

mExpClass(Algo) RandomFilter2D : public Filter2D
		     , public RandomFilter
{
public:

    mSimpPosFilterDefFns2D(Random);

};


/*!
\brief Passes each nth position.
*/

mExpClass(Algo) SubsampFilter : public virtual Filter
{
public:

			SubsampFilter()
			    : each_(2), seqnr_(0)			{}
			SubsampFilter( const SubsampFilter& sf )
			    : each_(sf.each_), seqnr_(sf.seqnr_)	{}

    void		reset() override	{ seqnr_ = 0; }

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;
    float		estRatio(const Provider&) const	override
			{ return 1.f/each_; }

    int			each_;

    static const char*	typeStr();
    static const char*	eachStr();

protected:

    mutable int		seqnr_;

    bool		drawRes() const;
};


/*!
\brief Passes each nth position (3D).
*/

mExpClass(Algo) SubsampFilter3D : public Filter3D
		      , public SubsampFilter
{
public:

    mSimpPosFilterDefFns3D(Subsamp);

};


/*!
\brief Passes each nth position (2D).
*/

mExpClass(Algo) SubsampFilter2D : public Filter2D
		      , public SubsampFilter
{
public:

    mSimpPosFilterDefFns2D(Subsamp);

};

} // namespace

