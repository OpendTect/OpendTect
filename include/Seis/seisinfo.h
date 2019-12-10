#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		25-10-1996
________________________________________________________________________

-*/

#include "seisposkey.h"
#include "samplingdata.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
class SeisTrc;
class TrcKey;
class PosAuxInfo;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:

    mUseType( Pos,	IdxPair );
    mUseType( IdxPair,	pos_type );
    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );
    typedef int		idx_type;
    typedef idx_type	size_type;
    typedef Pos::Z_Type	z_type;

			SeisTrcInfo();
			~SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator =(const SeisTrcInfo&);

    Coord		coord_;
    SamplingData<float>	sampling_;
    float		offset_			= 0.f;
    float		azimuth_		= 0.f;
    float		refnr_			= mUdf(float);
    float		pick_			= mUdf(float);

    bool		is2D() const;
    bool		isSynthetic() const;
    GeomSystem		geomSystem() const;
    BinID		binID() const;
    Bin2D		bin2D() const;
    IdxPair		idxPair() const;
    pos_type		inl() const;
    pos_type		crl() const;
    pos_type		lineNr() const;
    pos_type		trcNr() const;
    GeomID		geomID() const;
    const TrcKey&	trcKey() const		{ return trcky_; }

    SeisTrcInfo&	setGeomSystem(GeomSystem);
    SeisTrcInfo&	setPos(const BinID&);
    SeisTrcInfo&	setPos(const Bin2D&);
    SeisTrcInfo&	setPos(GeomID,pos_type);
    SeisTrcInfo&	setGeomID(GeomID);
    SeisTrcInfo&	setTrcNr(pos_type);
    SeisTrcInfo&	setLineNr(pos_type);
    SeisTrcInfo&	setTrcKey(const TrcKey&);
    SeisTrcInfo&	calcCoord(); //!< from current BinID or Bin2D

    idx_type		nearestSample(z_type) const;
    z_type		samplePos( idx_type idx ) const
			{ return sampling_.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<z_type>&) const;
    bool		dataPresent(z_type pos,size_type trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth };
			mDeclareEnumUtils(Fld)
    double		getValue(Fld) const;
    static void		getAxisCandidates(Seis::GeomType,TypeSet<Fld>&);
    Fld			getDefaultAxisFld(Seis::GeomType,
					  const SeisTrcInfo* next) const;
    void		getInterestingFlds(Seis::GeomType,IOPar&) const;
    void		setPSFlds(const Coord& rcvpos,const Coord& srcpos,
				  bool setpos=false);

    static const char*	sKeySamplingInfo;
    static const char*	sKeyNrSamples;
    static float	defaultSampleInterval(bool forcetime=false);

    Seis::PosKey	posKey(Seis::GeomType) const;
    void		setPosKey(const Seis::PosKey&);
    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    float		zref_			= 0.f;		// not stored

protected:

    TrcKey&		trcky_;

};
