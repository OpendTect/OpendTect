#ifndef seisinfo_h
#define seisinfo_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.5 2001-02-13 17:16:09 bert Exp $
________________________________________________________________________

Seismic Packet and trace information. Simple, accessible information.

@$*/
 
#include <samplingdata.h>
#include <position.h>
#include <seistype.h>
#include <enums.h>
#include <datachar.h>
class SUsegy;
class BinIDRange;
class SeisTrc;


class SeisPacketInfo
{
public:
			SeisPacketInfo();
			SeisPacketInfo(const SeisPacketInfo&);
    SeisPacketInfo&	operator=(const SeisPacketInfo&);
    virtual		~SeisPacketInfo();

    FixedString<32>	client;
    FixedString<32>	company;
    FixedString<180>	auxinfo;
    int			nr;
    BinIDRange&		range;

    void		clear();

    static const char*	sBinIDs;

    static FixedString<32>	defaultclient;
    static FixedString<32>	defaultcompany;
    static FixedString<180>	defaultauxinfo;

};


class SeisTrcInfo
{
public:
			SeisTrcInfo()
			: nr(0), sampling(0,.004)
			, refpos(mUndefValue), pick(mUndefValue)
			, new_packet(NO), stack_count(1)
			, mute_pos(mUndefValue), taper_length(0)
			, offset(0)
			{}

	    // persistent
    SamplingData<float>	sampling;
    int			nr;		// 0
    float		pick;		// 1
    float		refpos;		// 2
    Coord		coord;		// 3 4
    BinID		binid;		// 5 6
    float		offset;		// 7

	    // temporary
    bool		new_packet;
    int			stack_count;
    float		mute_pos;
    float		taper_length;

    int			nearestSample(float pos,int sampoffs=0) const;
    float		samplePos( int idx, int sampoffs=0 ) const
			{ return sampling.atIndex( idx + sampoffs ); }
    SampleGate		sampleGate(const Interval<float>&,int sampoffs=0) const;
    bool		dataPresent(float pos,int trcsize,int sampoffs=0) const;
    void		gettr(SUsegy&) const;
    void		puttr(const SUsegy&);

    static const char*	attrName( int idx )	{ return attrnames[idx]; }
    static int		nrAttrs()		{ return 8; }
    static int		attrNr(const char*);
    static const char*	attrnames[];
    double		getAttr(int) const;

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
};


#endif
