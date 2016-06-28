#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
________________________________________________________________________

-*/

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

    typedef IdxPair::IdxType IdxType;

			SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator =(const SeisTrcInfo&);

    TrcKey		trckey_;
    IdxType		nr_;
    Coord		coord_;
    SamplingData<float>	sampling_;
    float		offset_;
    float		azimuth_;
    float		refnr_;
    float		pick_;

    inline Pos::SurvID	survID() const		{ return trckey_.survID(); }
    inline const BinID&	binID() const		{ return trckey_.binID(); }
    inline IdxType	lineNr() const		{ return trckey_.lineNr(); }
    inline IdxType	trcNr() const		{ return trckey_.trcNr(); }
    inline SeisTrcInfo&	setBinID( const BinID& bid )
			{ trckey_.setBinID(bid); return *this; }
    inline SeisTrcInfo&	setLineNr( IdxType lnr )
			{ trckey_.setLineNr(lnr); return *this; }
    inline SeisTrcInfo&	setTrcNr( IdxType tnr )
			{ trckey_.setTrcNr(tnr); return *this; }

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling_.atIndex( idx ); }
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

    float		zref_;		// not stored
    bool		new_packet_;	// not stored

    mDeprecated IdxType& nr;
    mDeprecated Coord&	coord;
    mDeprecated SamplingData<float>& sampling;
    mDeprecated float&	offset;
    mDeprecated float&	azimuth;
    mDeprecated float&	refnr;
    mDeprecated float&	pick;
    /* mDeprecated */ BinID& binid;
			//!< in new code, use binID()/setBinID() instead

};


#endif
