#ifndef seisinfo_h
#define seisinfo_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisinfo.h,v 1.10 2002-09-03 08:35:00 bert Exp $
________________________________________________________________________

Seismic Packet and trace information. Simple, accessible information.

@$*/
 
#include <samplingdata.h>
#include <position.h>
#include <seistype.h>
#include <enums.h>
#include <datachar.h>
class SUsegy;
class BinIDSampler;
class SeisTrc;

/*!\brief Information for a packet of seismics, AKA tape header info */


class SeisPacketInfo
{
public:
			SeisPacketInfo();
			SeisPacketInfo(const SeisPacketInfo&);
    SeisPacketInfo&	operator=(const SeisPacketInfo&);
    virtual		~SeisPacketInfo();

    BufferString	usrinfo;
    BufferString	stdinfo;
    int			nr;
    BinIDSampler&	binidsampling;

    void		clear();

    static const char*	sBinIDs;

    static BufferString	defaultusrinfo;

};


/*!\brief Information for a seismic trace, AKA trace header info */

class SeisTrcInfo
{
public:
			SeisTrcInfo()
			: sampling(0,.004), nr(0)
			, pick(mUndefValue), refpos(mUndefValue)
			, offset(0), azimuth(0)
			, new_packet(NO), stack_count(1)
			, mute_pos(mUndefValue), taper_length(0)
			{}

	    // persistent
    SamplingData<float>	sampling;
    int			nr;		// 0
    float		pick;		// 1
    float		refpos;		// 2
    Coord		coord;		// 3 4
    BinID		binid;		// 5 6
    float		offset;		// 7
    float		azimuth;	// 8

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
    static int		nrAttrs()		{ return 9; }
    static int		attrNr(const char*);
    static const char*	attrnames[];
    double		getAttr(int) const;

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
};


inline float mathAng2DegFromNorth( float rads )
{
    float deg = 90 - 57.2957795131 * rads;
    return deg < 0 ? deg + 360 : deg;
}


#endif
