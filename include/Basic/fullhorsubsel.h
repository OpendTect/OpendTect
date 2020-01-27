#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "survsubsel.h"

class CubeSubSel;
class CubeHorSubSel;
class LineSubSel;
class LineHorSubSel;
class LineHorSubSelSet;
class LineSubSelSet;
class SurveyInfo;
class TrcKeySampling;
class TrcKeyZSampling;


namespace Survey
{

class FullSubSel;

/*!\brief subselection of the positions in a 3D geometry or a set of
	2D geometries */

mExpClass(Basic) FullHorSubSel
{
public:

    mUseType( Pos,			GeomID );
    mUseType( Pos::IdxPair,		pos_type );
    mUseType( GeomSubSel,		idx_type );
    mUseType( GeomSubSel,		size_type );
    typedef Interval<pos_type>		pos_rg_type;
    typedef StepInterval<pos_type>	pos_steprg_type;
    typedef pos_type			trcnr_type;

			FullHorSubSel(const SurveyInfo* si=nullptr);
			FullHorSubSel(GeomID,const SurveyInfo* si=nullptr);
			FullHorSubSel(const GeomIDSet&,
				      const SurveyInfo* si=nullptr);
			FullHorSubSel(const CubeSubSel&);
			FullHorSubSel(const LineSubSel&);
			FullHorSubSel(const GeomSubSel&);
			FullHorSubSel(const CubeHorSubSel&);
			FullHorSubSel(const LineHorSubSel&);
			FullHorSubSel(const LineHorSubSelSet&);
			FullHorSubSel(const LineSubSelSet&);
			FullHorSubSel(const FullSubSel&);
			FullHorSubSel(const BinID&);		//!< single pos
			FullHorSubSel(GeomID,trcnr_type);	//!< single pos
			FullHorSubSel(const TrcKey&);		//!< single pos
			FullHorSubSel(const FullHorSubSel&);
			FullHorSubSel(const IOPar&,
				      const SurveyInfo* si=nullptr);
			FullHorSubSel(const TrcKeySampling&);
			FullHorSubSel(const TrcKeyZSampling&);
    virtual		~FullHorSubSel();
    FullHorSubSel&	operator =(const FullHorSubSel&);
    bool		operator ==(const FullHorSubSel&) const;
			mImplSimpleIneqOper(FullHorSubSel);
    const SurveyInfo&	survInfo() const;

    bool		is2D() const	{ return !chss_; }
    bool		is3D() const	{ return chss_; }

    pos_steprg_type	inlRange() const;
    pos_steprg_type	crlRange() const;
    pos_steprg_type	trcNrRange(idx_type iln=0) const;
    size_type		nrGeomIDs() const;
    GeomID		geomID(idx_type) const;
    idx_type		indexOf(GeomID) const;
    bool		isPresent( GeomID gid ) const
			{ return indexOf(gid)>=0; }

    void		setInlRange(const pos_rg_type&);
    void		setCrlRange(const pos_rg_type&);
    void		setTrcNrRange(const pos_rg_type&,idx_type i=0);
    void		setTrcNrRange(GeomID,const pos_rg_type&);

    bool		isAll() const;
    HorSubSel&		horSubSel(idx_type i=0);
    const HorSubSel&	horSubSel(idx_type i=0) const;
    CubeHorSubSel&	cubeHorSubSel()		{ return *chss_;}
    const CubeHorSubSel& cubeHorSubSel() const	{ return *chss_;}
    LineHorSubSel&	lineHorSubSel(idx_type);
    const LineHorSubSel& lineHorSubSel(idx_type) const;
    CubeHorSubSel&	subSel3D()		{ return *chss_; }
    const CubeHorSubSel& subSel3D() const	{ return *chss_; }
    LineHorSubSelSet&	subSel2D()		{ return lhsss_; }
    const LineHorSubSelSet& subSel2D() const	{ return lhsss_; }
    const LineHorSubSel* findLineHorSubSel(GeomID) const;
    void		merge(const FullHorSubSel&);
    void		limitTo(const FullHorSubSel&);

    void		setEmpty();
    void		setToAll(bool for2d,const SurveyInfo* si=nullptr);
    void		setFull(GeomID,const SurveyInfo* si=nullptr);
    void		set(const CubeHorSubSel&);
    void		set(const CubeSubSel&);
    void		set(const LineHorSubSel&);
    void		set(const LineSubSel&);
    void		set(const LineHorSubSelSet&);
    void		set(const LineSubSelSet&);
    void		setGeomID(GeomID,const SurveyInfo* si=nullptr);
    void		addGeomID(GeomID);

    uiString		getUserSummary() const;
    size_type		expectedNrPositions() const;

    static const char*	sNrLinesKey();
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&,const SurveyInfo* si=nullptr);

protected:

    CubeHorSubSel*	chss_	= nullptr;
    LineHorSubSelSet&	lhsss_;

    void		set3D(bool,const SurveyInfo* si=nullptr);
    void		clearContents();

    friend class	SubSelPosIter;

public:

    int			selRes3D(const BinID&) const;
    int			selRes2D(GeomID,trcnr_type) const;

};


/*!\brief Iterator for BinID/TrcNr positions for FullHorSubSel */

mExpClass(Basic) SubSelPosIter
{
public:

    mUseType( FullHorSubSel,	idx_type );
    mUseType( FullHorSubSel,	GeomID );
    mUseType( FullHorSubSel,	trcnr_type );

			SubSelPosIter(const FullHorSubSel&);
			SubSelPosIter(const FullSubSel&);
			SubSelPosIter(const SubSelPosIter&);

    const FullHorSubSel& subSel() const
			{ return subsel_; }

    bool		next();
    void		reset()		{ lineidx_ = trcidx_ = -1; }
    bool		is2D() const	{ return subsel_.is2D(); }

    GeomID		geomID() const;
    trcnr_type		trcNr() const;
    BinID		binID() const;
    Bin2D		bin2D() const;

protected:

    const FullHorSubSel& subsel_;
    idx_type		lineidx_	    = -1;
    idx_type		trcidx_		    = -1;

};


} // namespace Survey
