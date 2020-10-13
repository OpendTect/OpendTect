#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisposkey.h"
#include "samplingdata.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
class SeisTrc;
class PosAuxInfo;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:

    typedef Index_Type	IdxType;

			SeisTrcInfo()
			: sampling(0,defaultSampleInterval()), nr(0)
			, refnr(mUdf(float)), pick(mUdf(float))
			, offset(0), azimuth(0), zref(0), new_packet(false)
			{}

    SamplingData<float>	sampling;
    int			nr;
    BinID		binid;			/* mDeprecated */
    Coord		coord;
    float		offset;
    float		azimuth;
    float		refnr;
    float		pick;

			// New functions that will be used more and more.
			// Try to avoid using binid directly!
    inline const BinID& binID() const		{ return binid; }
    inline IdxType	inl() const		{ return binid.inl(); }
    inline IdxType	crl() const		{ return binid.crl(); }
    inline IdxType	lineNr() const		{ return inl(); }
    inline IdxType	trcNr() const		{ return crl(); }
    TrcKey		trcKey() const
			{ return TrcKey(lineNr(),trcNr()); }
    void		setTrcKey(const TrcKey&);

    inline SeisTrcInfo& setBinID( const BinID& bid )
			{ binid = bid; return *this; }
    inline SeisTrcInfo& setInl( IdxType inr )
			{ binid.inl() = inr; return *this; }
    inline SeisTrcInfo& setCrl( IdxType inr )
			{ binid.crl() = inr; return *this; }
    inline void		setLineNr( int inr )	{ binid.inl() = inr; }
    inline void		setTrcNr( int inr )	{ binid.crl() = nr = inr; }

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth };
			mDeclareEnumUtils(Fld)
    double		getValue(Fld) const;
    static void		getAxisCandidates(Seis::GeomType,TypeSet<Fld>&);
    int			getDefaultAxisFld(Seis::GeomType,
					  const SeisTrcInfo* next) const;
    void		getInterestingFlds(Seis::GeomType,IOPar&) const;
    void		setPSFlds(const Coord& rcvpos,const Coord& srcpos,
				  bool setpos=false);

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
    static float	defaultSampleInterval(bool forcetime=false);

    Seis::PosKey	posKey(Seis::GeomType) const;
    void		setPosKey(const Seis::PosKey&);
    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    float		zref;		// not stored
    bool		new_packet;	// not stored

};


