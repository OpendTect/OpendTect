#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "basicmod.h"
#include "trckeysampling.h"
#include "enums.h"
class HorSampling;
class CubeSampling;
class CubeSubSel;
class LineSubSel;
namespace Survey { class GeomSubSel; class FullSubSel; }


/*!\brief Hor+Vert sampling in 2D and 3D geometries.

  Note that this class may result in subgeometries that are not consistent with
  available geometries. To correct this, we want to migrate to usage of
  HorSampling/CubeSampling (to define geometries) and LineSubSel/CubeSubSel (to
  subselect existing geometries).

  The amount of usage of TrcKeyZSampling is overwhelming, though.

 */

mExpClass(Basic) TrcKeyZSampling
{
public:

    mUseType( Pos,	GeomID );
    mUseType( OD,	GeomSystem );
    mUseType( OD,	SliceType );
    mUseType( TrcKey,	linenr_type );
    mUseType( TrcKey,	trcnr_type );
    mUseType( Survey,	HorSubSel );
    mUseType( Survey,	GeomSubSel );
    mUseType( Survey,	FullSubSel );
    mUseType( Survey,	Geometry );

			TrcKeyZSampling(OD::SurvLimitType slt=OD::FullSurvey);
			TrcKeyZSampling(GeomID);
			TrcKeyZSampling(const HorSubSel&);
			TrcKeyZSampling(const GeomSubSel&);
			TrcKeyZSampling(const CubeSubSel&);
			TrcKeyZSampling(const LineSubSel&);
			TrcKeyZSampling(const Geometry&);
			TrcKeyZSampling(const HorSampling&);
			TrcKeyZSampling(const CubeSampling&);
			TrcKeyZSampling(const FullSubSel&);
			TrcKeyZSampling(const TrcKeySampling&);
			TrcKeyZSampling(const TrcKeyZSampling&);
			TrcKeyZSampling(bool settoSI,
					OD::SurvLimitType slt=OD::FullSurvey);

    bool		is2D() const		{ return hsamp_.is2D(); }
    GeomID		getGeomID() const	{ return hsamp_.getGeomID(); }
    GeomSystem		geomSystem() const	{ return hsamp_.geomSystem(); }
    void		setGeomSystem( GeomSystem gs )
						{ hsamp_.setGeomSystem( gs ); }

    SliceType		defaultDir() const;
			//!< 'flattest' direction, i.e. direction with
			//!< smallest size. If equal, prefer Inl then Crl then Z
    void		getDefaultNormal(Coord3&) const;
    bool		isFlat() const; //!< is one of directions size 1?

    void		init(bool settoSI=true,
			     OD::SurvLimitType slt=OD::FullSurvey);
    void		setTo(GeomID);
    void		setTo(const Geometry&);
    inline void		setEmpty()		{ init(false); }
    void		set2DDef();
    void		normalise();
			//!< Makes sure start<stop and steps are non-zero

    TrcKeySampling	hsamp_;
    ZSampling		zsamp_;

    int			lineIdx(linenr_type) const;
    int			trcIdx(trcnr_type) const;
    int			zIdx(float z) const;
    int			nrLines() const;
    int			nrTrcs() const;
    int			nrZ() const;
    od_int64		totalNr() const;
    int			size(SliceType) const;
    float		zAtIndex( int idx ) const;
    bool		isEmpty() const;
    bool		isDefined() const;
    bool		includes(const TrcKeyZSampling&) const;
    bool		getIntersection(const TrcKeyZSampling&,
					TrcKeyZSampling&) const;
			//!< Returns false if intersection is empty
    void		include(const TrcKey&,float z);
    void		include(const TrcKeyZSampling&);
    void		limitTo(const TrcKeyZSampling&,bool ignoresteps=false);
    void		limitToWithUdf(const TrcKeyZSampling&);
			/*!< handles undef values + returns reference cube
			     nearest limit if the 2 cubes do not intersect */
    void		shrinkTo(const TrcKeyZSampling& innertkzs,
				 float releps=1e-4);
			/*!< shrinks "this" tkzs up to but not across innertkzs
			     boundaries. Result will be one step larger than
			     limitTo(.) in case of non-matching steps. */
    void		growTo(const TrcKeyZSampling& outertkzs,
			       float releps=1e-4);
			/*!< grows "this" tkzs up to but not across outertkzs
			     boundaries. Counterpart of shrinkTo(.) function. */
    bool		adjustTo(const TrcKeyZSampling& availabletkzs,
				 bool falsereturnsdummy=false);
			/*!< adjusts the non-flat dimensions of the desired
			     "this" tkzs to the availabletkzs for optimal
			     texture data display. */
    bool		makeCompatibleWith(const TrcKeyZSampling& othertkzs);
			/*!< makes "this" tkzs compatible to othertkzs. Only
			     keeps locations lying on the grid of othertkzs. */
    void		expand(int nrlines,int nrtrcs,int nrz);

    void		snapToSurvey();
			/*!< Checks if it is on valid bids and sample positions.
			     If not, it will expand until it is */

    bool		isEqual(const TrcKeyZSampling&,
				float zeps=mUdf(float)) const;

    bool		operator==(const TrcKeyZSampling&) const;
    bool		operator!=(const TrcKeyZSampling&) const;
    TrcKeyZSampling&	operator=(const TrcKeyZSampling&);

    bool		usePar(const IOPar&);
    bool		useJSON(const OD::JSON::Object&);
    void		fillPar(IOPar&) const;
    void		fillJSON(OD::JSON::Object&) const;
    static void		removeInfo(IOPar&);

//Legacy, don't use
    inline int		inlIdx( int inl ) const { return lineIdx(inl); }
    inline int		crlIdx( int crl ) const { return trcIdx(crl); }
    void		include(const BinID& bid,float z);

    inline int		nrInl() const		{ return nrLines(); }
    inline int		nrCrl() const		{ return nrTrcs(); }

};
