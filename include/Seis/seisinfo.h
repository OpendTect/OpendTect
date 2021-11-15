#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
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

    typedef IdxPair::IdxType	IdxType;

			SeisTrcInfo();
			~SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator =(const SeisTrcInfo&);

    Coord		coord;
    SamplingData<float> sampling;
    float		offset		= 0.f;
    float		azimuth		= 0.f;
    float		refnr		= mUdf(float);
    float		pick		= mUdf(float);
    int			seqnr_		= 0;

    bool		is2D() const;
    bool		is3D() const;
    bool		isSynthetic() const;
    Pos::SurvID		geomSystem() const;
    BinID		binID() const;
    Pos::IdxPair	idxPair() const;
    IdxType		inl() const;
    IdxType		crl() const;
    IdxType		lineNr() const;
    IdxType		trcNr() const;
    Pos::GeomID		geomID() const;
    const TrcKey&	trcKey() const		{ return trckey_; }

    SeisTrcInfo&	setGeomSystem(Pos::SurvID);
    SeisTrcInfo&	setPos(const BinID&); //3D
    SeisTrcInfo&	setPos(Pos::GeomID,IdxType); //2D
    SeisTrcInfo&	setGeomID(Pos::GeomID);
    SeisTrcInfo&	setLineNr(IdxType);
    SeisTrcInfo&	setTrcNr(IdxType);
    SeisTrcInfo&	setTrcKey(const TrcKey&);
    SeisTrcInfo&	calcCoord(); //!< from current TrcKey position

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth, SeqNr, GeomID };
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

    float		zref		= 0.f; // not stored
    bool		new_packet	= false; // not stored

private:

    TrcKey&		trckey_;

public:

    mDeprecated("Use setPos")
    SeisTrcInfo&	setBinID( const BinID& bid )
						{ return setPos(bid); }
    mDeprecated("Use setLineNr")
    SeisTrcInfo&	setInl( IdxType inl )	{ return setLineNr(inl); }
    mDeprecated("Use setTrcNr")
    SeisTrcInfo&	setCrl( IdxType crl )	{ return setTrcNr(crl); }

    mDeprecated("Use setPos")
    BinID&		binid;
    mDeprecated("Use seqnr_ or setTrcNr")
    int&		nr;

};


