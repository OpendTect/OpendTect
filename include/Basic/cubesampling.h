#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.3 2002-04-05 15:15:14 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <position.h>
class BinIDRange;
class BinIDSampler;


/*\brief Horizontal sampling in 3D surveys */

struct HorSampling
{
			HorSampling()			{ init(); }
			HorSampling(const BinIDRange&);
			HorSampling(const BinIDSampler&);
    HorSampling&	set(const BinIDRange&);
    HorSampling&	set(const BinIDSampler&);

    inline bool		includes( const BinID& bid ) const
			{ return inlOK(bid.inl) && crlOK(bid.crl); }

    inline bool		inlOK( int inl ) const
			{ return inl >= start.inl && inl <= stop.inl
			      && !( (inl-start.inl) % step.inl ); }
    inline bool		crlOK( int crl ) const
			{ return crl >= start.crl && crl <= stop.crl
			      && !( (crl-start.crl) % step.crl ); }

    inline void		include( const BinID& bid )
			{ includeInl(bid.inl); includeCrl(bid.crl); }
    inline void		includeInl( int inl )
			{ if ( inl < start.inl ) start.inl = inl;
			  if ( inl > stop.inl ) stop.inl = inl; }
    inline void		includeCrl( int crl )
			{ if ( crl < start.crl ) start.crl = crl;
			  if ( crl > stop.crl ) stop.crl = crl; }

    inline int		nrInl() const
			{ return (stop.inl-start.inl) / step.inl + 1; }
    inline int		nrCrl() const
			{ return (stop.crl-start.crl) / step.crl + 1; }
    inline int		totalNr() const
			{ return nrInl() * nrCrl(); }

    void		init();
    			//!< Sets to survey values
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    bool		getInterSection(const HorSampling&,HorSampling&) const;
    			//!< Returns false if intersection is empty

    BinID		start;
    BinID		stop;
    BinID		step;

};


/*\brief Hor+Vert sampling in 3D surveys */

struct CubeSampling
{
public:

    			CubeSampling()		{ init(); }

    void		init();
    			//!< Sets to survey values
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    HorSampling		hrg;
    StepInterval<float>	zrg;

    inline int		nrInl() const		{ return hrg.nrInl(); }
    inline int		nrCrl() const		{ return hrg.nrCrl(); }
    inline int		nrZ() const		{ return zrg.nrSteps() + 1; }
    inline int		totalNr() const		{ return hrg.totalNr() * nrZ();}
    bool		getInterSection(const CubeSampling&,
	    				CubeSampling&) const;
    			//!< Returns false if intersection is empty

};


#endif
