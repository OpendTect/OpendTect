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
class PosAuxInfo;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:

    mUseType( IdxPair,	pos_type );
    mUseType( OD,	GeomSystem );
    typedef int		idx_type;
    typedef idx_type	size_type;
    typedef Pos::Z_Type	z_type;

			SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator =(const SeisTrcInfo&);

    TrcKey		trckey_;
    Coord		coord_;
    SamplingData<float>	sampling_;
    float		offset_;
    float		azimuth_;
    float		refnr_;
    float		pick_;

    inline bool		is2D() const		{ return trckey_.is2D(); }
    inline bool		isSynthetic() const	{ return trckey_.isSynthetic();}
    GeomSystem		geomSystem() const	{ return trckey_.geomSystem(); }
    inline const BinID&	binID() const		{ return trckey_.binID(); }
    inline pos_type	lineNr() const		{ return trckey_.lineNr(); }
    inline pos_type	trcNr() const		{ return trckey_.trcNr(); }

    inline SeisTrcInfo&	setGeomSystem( GeomSystem gs )
			{ trckey_.setGeomSystem(gs); return *this; }
    inline SeisTrcInfo&	setBinID( const BinID& bid )
			{ trckey_.setBinID(bid); return *this; }
    inline SeisTrcInfo&	setLineNr( pos_type lnr )
			{ trckey_.setLineNr(lnr); return *this; }
    inline SeisTrcInfo&	setTrcNr( pos_type tnr )
			{ trckey_.setTrcNr(tnr); return *this; }

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

    float		zref_;		// not stored

};
