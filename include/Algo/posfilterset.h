#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "posfilter.h"




namespace Pos
{

/*!
\brief Set of Filters. Owns the Filters.
*/

mExpClass(Algo) FilterSet : public virtual Filter
{
public:

    virtual		~FilterSet();

    void		add(Filter*);
    void		add(const IOPar&);

    static const char*	typeStr();		//!< "Set"

    bool		initialize(TaskRunner*) override;
    void		reset() override;
    bool		includes(const Coord&,float) const override;
    float		adjustedZ(const Coord&,float) const override;
    bool		hasZAdjustment() const override;
    void		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;
    void		getSummary(BufferString&) const override;
    float		estRatio(const Provider&) const override;

    bool		isEmpty() const	{ return filts_.isEmpty(); }
    int			size() const	{ return filts_.size(); }

    ObjectSet<Filter>& filters()			{ return filts_; }
    const ObjectSet<Filter>& filters() const	{ return filts_; }

protected:

    ObjectSet<Filter>	filts_;

    void		copyFrom(const FilterSet&);

};


#define mSimpPosFilterSetDefFns(dim) \
			FilterSet##dim()	{} \
			FilterSet##dim( const FilterSet##dim& fs ) \
					{ *this = fs; } \
    FilterSet##dim&	operator =( const FilterSet##dim& fs ) \
					{ copyFrom(fs); return *this; } \
    Filter*	clone() const override	{ return new FilterSet##dim(*this); } \
    const char* type() const override	{ return typeStr(); } \
    const char* factoryKeyword() const override	{ return type(); } \
    bool	includes( const Coord& c, float z=1e30 ) const override \
			{ return FilterSet::includes(c,z); } \


/*!
\brief 3D FilterSet
*/

mExpClass(Algo) FilterSet3D : public FilterSet
		  , public Filter3D
{
public:

    bool	is2D() const override	{ return false; }
    bool	includes(const BinID&,float z=mUdf(float)) const override;

    mSimpPosFilterSetDefFns(3D)

};


/*!
\brief 2D FilterSet
*/

mExpClass(Algo) FilterSet2D : public FilterSet
		  , public Filter2D
{
public:

    bool	is2D() const override	{ return true; }
    bool	includes(int,float z=mUdf(float),int lidx=0) const override;

    mSimpPosFilterSetDefFns(2D)

};

} // namespace Pos
