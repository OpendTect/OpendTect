#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.20 2006-11-10 13:51:37 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "enums.h"
#include "ranges.h"
#include "position.h"
#include "datachar.h"
#include "samplingdata.h"
class SUsegy;
class SeisTrc;
class PosAuxInfo;
class IOPar;

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

					// Attr number (see below)
    SamplingData<float>	sampling;
    int			nr;		// 0
    float		pick;		// 1
    float		refpos;		// 2
    Coord		coord;		// 3 4
    BinID		binid;		// 5 6
    float		offset;		// 7
    float		azimuth;	// 8
    bool		new_packet;	// not stored

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;
    void		gettr(SUsegy&) const;
    void		puttr(const SUsegy&);

    // Attrs are invented so you can get a header value without knowing
    // what it is. Useful e.g to let user decide what to display
    static int		nrAttrs()		{ return 9; }
    static const char*	attrName( int idx )	{ return attrnames[idx]; }
    static const char**	attrNames()		{ return attrnames; }
    static int		attrNr(const char*);
    double		getAttrValue(int) const;
    int			defDispAttr(const SeisTrcInfo& next) const;

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
    static float	defaultSampleInterval(bool forcetime=false);

    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);

private:

    static const char*	attrnames[];

};


#endif
