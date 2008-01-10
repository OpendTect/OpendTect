#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.22 2008-01-10 09:59:03 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "enums.h"
#include "ranges.h"
#include "position.h"
#include "datachar.h"
#include "samplingdata.h"
#include "seistype.h"
class SeisTrc;
class PosAuxInfo;
class IOPar;
template <class T> class TypeSet;

/*!\brief Information for a packet of seismics, AKA tape header info */


class SeisPacketInfo
{
public:

			SeisPacketInfo()	{ clear(); }

    BufferString	usrinfo;
    BufferString	stdinfo;
    int			nr;

    StepInterval<int>	inlrg;
    StepInterval<int>	crlrg;
    StepInterval<float>	zrg;
    bool		inlrev;
    bool		crlrev;

    void		clear();

    static const char*	sBinIDs;
    static const char*	sZRange;

    static BufferString	defaultusrinfo;

};


/*!\brief Information for a seismic trace, AKA trace header info */

class SeisTrcInfo
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

    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    bool		new_packet;	// not stored
};


#endif
