#ifndef posfilterset_h
#define posfilterset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilterset.h,v 1.1 2008-02-22 15:02:45 cvsbert Exp $
________________________________________________________________________


-*/

#include "posfilter.h"


#define mDeclPosFiltSetFns(virtspec,pfx) \
    virtspec const char* pfx##type() const { return typeStr(); } \
    virtspec bool	pfx##initialize(); \
    virtspec Executor*	pfx##initializer() const; \
    virtspec void	pfx##reset(); \
    virtspec bool	pfx##includes(const Coord&,float) const; \
    virtspec void	pfx##adjustZ(const Coord&,float&) const; \
    virtspec bool	pfx##hasZAdjustment() const; \
    virtspec void	pfx##fillPar(IOPar&) const; \
    virtspec void	pfx##usePar(const IOPar&); \
    virtspec void	pfx##getSummary(BufferString&) const


namespace Pos
{

/*!\brief Set of Filters. Owns the Filters. */

class FilterSet : public ObjectSet<Pos::Filter>
{
public:

    virtual		~FilterSet();

    void		add(Filter*);		//!< safer than += operator
    void		add(const IOPar&);

    static const char*	typeStr();		//!< "Set"

protected:

    void		copyFrom(const FilterSet&);

    virtual bool	is2d() const		= 0;
    mDeclPosFiltSetFns(,IMPL_);

};


class FilterSet3D : public FilterSet
		  , public Filter3D
{
public:

			FilterSet3D()	{}
			FilterSet3D( const FilterSet3D& fs )
					{ *this = fs; }
    FilterSet3D&	operator =( const FilterSet3D& fs )
					{ copyFrom(fs); return *this; }
    virtual Filter*	clone() const	{ return new FilterSet3D(*this); }
    virtual bool	is2D() const	{ return false; }

    mDeclPosFiltSetFns(virtual,);

    virtual bool	includes(const BinID&,float z=mUdf(float)) const;

    Filter3D*		filt3D( int idx )
    			{ return (Pos::Filter3D*)((*this)[idx]); }
    const Filter3D*	filt3D( int idx ) const
    			{ return (const Pos::Filter3D*)((*this)[idx]); }

protected:

    virtual bool	is2d() const		{ return false; }

};


class FilterSet2D : public FilterSet
		  , public Filter2D
{
public:

			FilterSet2D()	{}
			FilterSet2D( const FilterSet2D& fs )	{ *this = fs; }
    FilterSet&		operator =( const FilterSet& fs )
					{ copyFrom(fs); return *this; }
    virtual Filter*	clone() const	{ return new FilterSet2D(*this); }
    virtual bool	is2D() const	{ return true; }

    mDeclPosFiltSetFns(virtual,);

    virtual bool	includes(int,float z=mUdf(float)) const;

    Filter2D*		filt2D( int idx )
    			{ return (Pos::Filter2D*)((*this)[idx]); }
    const Filter2D*	filt2D( int idx ) const
    			{ return (const Pos::Filter2D*)((*this)[idx]); }

protected:

    virtual bool	is2d() const		{ return true; }

};


} // namespace

#endif
