#ifndef seisinfo_h
#define seisinfo_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.3 2000-03-03 09:28:59 bert Exp $
________________________________________________________________________

Seismic Packet and trace information. Simple, accessible information.

@$*/
 
#include <binidsel.h>
#include <ranges.h>
#include <fixstring.h>
#include <seistype.h>
#include <enums.h>
class SUsegy;


class SeisPacketInfo
{
public:
			SeisPacketInfo()
			: client(defaultclient)
			, company(defaultcompany)
			, auxinfo(defaultauxinfo)
			, wavetype(Seis::P), datatype(Seis::Ampl)
			, nr(0), ns(0), dt(0), starttime(1e-30)		{}

    FixedString<32>	client;
    FixedString<32>	company;
    FixedString<180>	auxinfo;

    Seis::WaveType	wavetype;
    Seis::DataType	datatype;

    int			nr;
    unsigned short	ns;
    unsigned int	dt;
    float		starttime;
    BinIDRange		range;

    void		fillEmpty(const SeisPacketInfo&);

    static const char*	sNrTrcs;
    static const char*	sBinIDs;

    static FixedString<32>	defaultclient;
    static FixedString<32>	defaultcompany;
    static FixedString<180>	defaultauxinfo;
};


class SeisTrcInfo
{
public:
			SeisTrcInfo()
			: nr(0), dt(4000)
			, starttime(0), reftime(mUndefValue)
			, pick(mUndefValue), pick2(mUndefValue)
			, new_packet(NO), stack_count(1)
			, mute_time(mUndefValue), taper_length(0)
			, offset(0)
			{}

	    // persistent
    int			nr;		// 0
    float		starttime;
    float		pick;		// 1
    float		pick2;
    float		reftime;	// 2
    unsigned int	dt;		// sample interval in micro-seconds
    Coord		coord;		// 3 4
    BinID		binid;		// 5 6
    BinID		binid2;
    float		offset;		// 7
    float		firstval;

	    // volatile
    bool		new_packet;
    int			stack_count;
    float		mute_time;
    float		taper_length;

    int			nearestSample(float t) const;
    SampleGate		sampleGate(const TimeGate&) const;
    float		sampleTime(int) const;
    bool		dataPresent(float t,int trcsize) const;
    void		gettr(SUsegy&) const;
    void		puttr(SUsegy&);

    static const char*	attrName( int idx )	{ return attrnames[idx]; }
    static int		nrAttrs()		{ return 8; }
    static int		attrNr(const char*);
    static const char*	attrnames[];
    double		getAttr(int) const;

    static const char*	sSampIntv;
    static const char*	sStartTime;
    static const char*	sNrSamples;
};


#endif
