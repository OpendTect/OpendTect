#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "ranges.h"
#include "binid.h"
#include "bufstring.h"
#include "geomid.h"
class Bin2D;
class BinnedValueSet;
class SurveyInfo;
namespace PosInfo { class LineCollData; }


namespace Seis
{

class PolySelData;
class RangeSelData;
class SelDataPosIter;
class TableSelData;


/*!\brief encapsulates subselection from a cube or lines.

  This class exists so that without knowing the form of the subselection,
  other classes can find out whether a trace is included or not.
  Use one of the isOK() functions for this.

  The function selRes() returns an integer which gives more information than
  just yes/no. If 0 is returned, the position is included. If non-zero,
  the inline or crossline number can be one of:

  0 - this number is OK by itself, but not the combination
  1 - this number is the 'party-pooper' but there are selected posns with it
  2 - No selected position has this number

  Especially (2) is very useful: an entire inl or crl can be skipped from input.
  The return value of selRes is inl_result + 256 * crl_result.

  If you're not interested in all that, just use isOK().

 */

mExpClass(Seis) SelData
{
public:

    mUseType( Pos,		GeomID );
    mUseType( Pos,		IdxPair );
    mUseType( PosInfo,		LineCollData );
    mUseType( Pos::IdxPair,	pos_type );
    typedef int			idx_type;
    typedef idx_type		size_type;
    typedef float		z_type;
    typedef SelType		Type;
    typedef SelDataPosIter	PosIter;
    typedef Interval<pos_type>	pos_rg_type;
    typedef StepInterval<z_type> z_steprg_type;
    typedef pos_type		trcnr_type;

    virtual		~SelData()			{}
    virtual SelData*	clone() const			= 0;
    void		copyFrom(const SelData&);
    static SelData*	get(Type);			//!< empty
    static SelData*	get(const IOPar&,const SurveyInfo* si=nullptr);
    static SelData*	get(const DBKey&);

    bool		operator ==(const SelData&) const;
    bool		operator !=(const SelData&) const;

    virtual Type	type() const			= 0;
    virtual bool	is2D() const			{ return false; }
    virtual bool	isAll() const			{ return false; }
    inline bool		isOK( const Bin2D& b2d ) const	{ return !selRes(b2d); }
    inline bool		isOK( const BinID& bid ) const	{ return !selRes(bid); }
    inline bool		isOK( GeomID gid, trcnr_type trcnr ) const
			{ return !selRes(gid,trcnr); }
    bool		isOK(const TrcKey&) const;
			//!< will work in trckey's domain
    bool		isOK(const IdxPair&) const;
			//!< will convert to either BinID or Bin2D

    virtual PosIter*	posIter() const			= 0;
    virtual pos_rg_type inlRange() const		= 0;
    virtual pos_rg_type crlRange() const		= 0;
    virtual pos_rg_type trcNrRange(idx_type i=0) const	{ return crlRange(); }
    virtual z_steprg_type zRange(idx_type i=0) const;

    uiString		usrSummary() const;
    virtual size_type	expectedNrTraces() const	= 0;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&,const SurveyInfo* si=nullptr);
    static void		removeFromPar(IOPar&,const char* subky=0);

    void		include(const SelData&);
    virtual void	setZRange(const z_steprg_type&,idx_type i=0) = 0;

    virtual size_type	nrGeomIDs() const		{ return 1; }
    virtual GeomID	geomID( idx_type i=0 ) const	{ return gtGeomID(i); }
    idx_type		indexOf(GeomID) const;
    bool		hasGeomID( GeomID g ) const	{ return indexOf(g)>=0;}

    bool		isRange() const		{ return type() == Range; }
    bool		isTable() const		{ return type() == Table; }
    bool		isPoly() const		{ return type() == Polygon; }
    RangeSelData*	asRange();
    const RangeSelData*	asRange() const;
    TableSelData*	asTable();
    const TableSelData*	asTable() const;
    PolySelData*	asPoly();
    const PolySelData*	asPoly() const;

    BinnedValueSet*	applyTo(const LineCollData&) const;

protected:

			SelData()			{}

    size_type		nrTrcsInSI() const;
    virtual GeomID	gtGeomID( idx_type ) const { return GeomID::get3D(); }

    virtual void	doCopyFrom(const SelData&)	= 0;
    virtual void	doFillPar(IOPar&) const		= 0;
    virtual void	doUsePar(const IOPar&,const SurveyInfo*) = 0;
    virtual uiString	gtUsrSummary() const		= 0;
    virtual int		selRes3D(const BinID&) const	= 0;
    virtual int		selRes2D(GeomID,trcnr_type) const;

    static const char*	sNrLinesKey();
    static int		cInvalidSelRes()		{ return 258; }

public:

    inline int		selRes( const BinID& bid ) const
			{ return selRes3D(bid); }
    inline int		selRes( GeomID gid, trcnr_type tnr ) const
			{ return selRes2D(gid,tnr); }
    int			selRes(const Bin2D&) const;

};


inline bool isAll( const SelData* sd )
{
    return !sd || sd->isAll();
}


/*!\brief needs a next() before a valid position is reached. */

mExpClass(Seis) SelDataPosIter
{
public:

    mUseType( SelData,	trcnr_type );
    mUseType( Pos,	GeomID );

    virtual		~SelDataPosIter()	{}
    virtual SelDataPosIter* clone() const	= 0;

    virtual bool	next()			= 0;
    virtual void	reset()			= 0;

    const SelData&	selData() const		{ return sd_; }
    virtual bool	is2D() const		{ return false; }

    virtual GeomID	geomID() const		{ return GeomID::get3D(); }
    virtual trcnr_type	trcNr() const;
    virtual BinID	binID() const		= 0;
    void		getTrcKey(TrcKey&) const;

protected:

			SelDataPosIter(const SelData&);
			SelDataPosIter(const SelDataPosIter&);

    const SelData&	sd_;

};



} // namespace
