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


/*!
\brief Hor+Vert sampling in 3D surveys.

  When slices are to be taken from a TrcKeyZSampling, they should be ordered
  as follows:

  Dir |   Dim1  |  Dim2
  ----|---------|------
  Inl |   Crl   |  Z
  Crl |   Inl   |  Z
  Z   |   Inl   |  Crl

  See also the direction() and dimension() free functions.
*/

mExpClass(Basic) TrcKeyZSampling
{
public:

			TrcKeyZSampling();
			TrcKeyZSampling(const TrcKeyZSampling&);
			TrcKeyZSampling(bool settoSI);

    enum Dir		{ Inl=0, Crl=1, Z=2 };
			mDeclareEnumUtils(Dir)
    bool		is2D() const		{ return hsamp_.is2D(); }
    Dir			defaultDir() const;
			//!< 'flattest' direction, i.e. direction with
			//!< smallest size. If equal, prefer Inl then Crl then Z
    void		getDefaultNormal(Coord3&) const;
    bool		isFlat() const; //!< is one of directions size 1?

    void		init(bool settoSI=true);
			//!< Sets hrg_.init and zrg_ to survey values or zeros
    inline void		setEmpty()		{ init(false); }
    void		set2DDef();
			//!< Sets to survey zrange and
    void		normalize();
			//!< Makes sure start<stop and steps are non-zero
    mDeprecated("Use normalize()")
    void		normalise();

    TrcKeySampling	hsamp_;
    StepInterval<float> zsamp_;

    int			lineIdx(Pos::LineID) const;
    int			trcIdx(Pos::TraceID) const;
    int			zIdx(float z) const;
    int			nrLines() const;
    int			nrTrcs() const;
    int			nrZ() const;
    od_int64		totalNr() const;
    int			size(Dir d) const;
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
    void		fillPar(IOPar&) const;
    void		fillJSON(OD::JSON::Object&) const;
    bool		useJSON(const OD::JSON::Object&);
    static void		removeInfo(IOPar&);

//Legacy, don't use
    inline int		inlIdx( int inl ) const { return lineIdx(inl); }
    inline int		crlIdx( int crl ) const { return trcIdx(crl); }
    void		include(const BinID& bid,float z);

    inline int		nrInl() const		{ return nrLines(); }
    inline int		nrCrl() const		{ return nrTrcs(); }

    mDeprecated("Use hsamp_ instead")	TrcKeySampling&		hrg;
    mDeprecated("Use zsamp_ instead")	StepInterval<float>&	zrg;

};



mExpClass(Basic) TrcKeyZSamplingSet : public TypeSet<TrcKeyZSampling>
{

};


mGlobal(Basic) TrcKeyZSampling::Dir direction(TrcKeyZSampling::Dir,
					      int dimnr);
mGlobal(Basic) int dimension(TrcKeyZSampling::Dir,TrcKeyZSampling::Dir);


typedef TrcKeyZSampling CubeSampling;

namespace Pos
{

mGlobal(Basic) bool intersect(const StepInterval<int>&,
			      const StepInterval<int>&,
			      StepInterval<int>&);
mGlobal(Basic) bool intersectF(const StepInterval<float>&,
			       const StepInterval<float>&,
			       StepInterval<float>&);

}

