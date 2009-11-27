#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.28 2009-11-27 09:41:30 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "seisposkey.h"
#include "samplingdata.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
class IOPar;
class PosAuxInfo;
template <class T> class TypeSet;


/*!\brief Information for a seismic trace, AKA trace header info */

mClass SeisTrcInfo
{
public:
			SeisTrcInfo()
			: sampling(0,defaultSampleInterval()), nr(0)
			, pick(mUdf(float)), refpos(mUdf(float))
			, offset(0), azimuth(0), new_packet(false)
			{}

    SamplingData<float>	sampling;
    int			nr;
    BinID		binid;
    Coord		coord;
    float		offset;
    float		azimuth;
    float		pick;
    float		refpos;

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefPos,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth };
    			DeclareEnumUtils(Fld)
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

    bool		new_packet;	// not stored

    void		handlePossibleFeetConversion(bool conv_back=false,
						 bool other_domain=false) const;
    /*!<
	Use this when seismic data meets non-seismic data.
	The other_domain is for time traces in depth surveys and vv.
	Note that this function most often does nothing.
	The 'const' is cheating, but very convenient. You already have to
	know what you're doing to use this function.
     */

};


#endif
