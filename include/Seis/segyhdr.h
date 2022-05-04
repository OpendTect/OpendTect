#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "segythdef.h"
#include "coord.h"
#include "samplingdata.h"
#include "datachar.h"
#include "coordsystem.h"

class SeisTrcInfo;
class od_ostream;


namespace SEGY
{

class Hdrdef;

#define SegyTxtHeaderLength		3200
#define SegyBinHeaderLength		400
#define SegyTrcHeaderLength		240

/*!\brief 3200 byte SEG-Y text header.

  On construction, the 'txt' buffer is filled with data for writing the header.
  If used for reading, fill the buffer yourself and use getFrom.

*/

mExpClass(Seis) TxtHeader
{
public:
		TxtHeader() : revision_(1) { clearText(); }
		TxtHeader(int rev);	//!< rev only relevant when writing
    void	clear()			{ clearText(); setLineStarts(); }

    int		setInfo(const char* datanm,const Coords::CoordSystem*,
			const TrcHeaderDef&);
    void	setUserInfo(int firstlinenr,const char*);
		//!< Optional, should be called last

    void	setGeomID(const Pos::GeomID&);

    mDeprecated("Use other setUserInfo")
    void	setUserInfo(const char*);
    mDeprecated("Use setInfo")
    void	setSurveySetupInfo(const Coords::CoordSystem*);
    mDeprecated("Use setInfo")
    void	setPosInfo(const TrcHeaderDef&);
    mDeprecated("Use setInfo")
    void	setStartPos(float);

    void	getText(BufferString&) const;
    void	setText(const char*);

    bool	isAscii() const;
    void	setAscii();
    void	setEbcdic();

    unsigned char txt_[SegyTxtHeaderLength];

    static bool& info2D();
    static bool& isPS();

    void	setLineStarts();
    void	dump(od_ostream&) const;

    static const char*	sKeySettingEBCDIC()
			{ return "SEGY.Text Header EBCDIC"; }

protected:

    int		revision_;

    void	putAt(int,int,int,const char*);
    void	getFrom(int,int,int,char*) const;

    void	clearText();

    int		setGeneralInfo(const char* datanm);
    int		setSurveySetupInfo(int firstlinenr,const Coords::CoordSystem*);
    int		setPosInfo(int firstlinenr,const TrcHeaderDef&);
};


/*!\brief 400 byte SEG-Y binary header  */

mExpClass(Seis) BinHeader
{
public:

		BinHeader();
    void	setInput(const void*,bool needswap=false);
    void	setForWrite();

    int		bytesPerSample() const
		{ return formatBytes( format() ); }
    static int	formatBytes( int frmt )
		{ return frmt == 3 ? 2 : (frmt == 8 ? 1 : 4); }
    static bool	isValidFormat( int f )
		{ return f==1 || f==2 || f==3 || f==5 || f==8; }
    static DataCharacteristics	getDataChar(int frmt,bool dataswapped);

    int		valueAt(int bytenr);
    void	setValueAt(int bytenr,int);
    inline int	entryVal( int idx ) const
		{ return hdrDef()[idx]->getValue(buf_,needswap_); }
    inline void	setEntryVal( int idx, int val )
		{ return hdrDef()[idx]->putValue(buf_,val); }

    short	format() const		{ return (short) entryVal(EntryFmt()); }
    int		nrSamples() const	{ return entryVal(EntryNs()); }
    int		rawSampleRate() const	{ return entryVal(EntryDt()); }
    float	sampleRate(bool isdpth) const;
    bool	isInFeet() const	{ return entryVal(EntryMFeet()) == 2; }
    int		revision() const;
    bool	isRev0() const		{ return revision() < 1; }
    int		skipRev1Stanzas(od_istream&); //!< returns number skipped

    void	setIsSwapped( bool yn )	{ needswap_ = yn; }
    bool	isSwapped() const	{ return needswap_; }
    void	guessIsSwapped();
    void	unSwap();

    void	setFormat( short i )	{ setEntryVal(EntryFmt(),i); }
    void	setNrSamples( int i )	{ setEntryVal(EntryNs(),i); }
    void	setSampleRate(float,bool isdpth);
    void	setInFeet( bool yn )	{ setEntryVal(EntryMFeet(),yn?2:1); }

    void	dump(od_ostream&) const;

    static const HdrDef&	hdrDef();
    unsigned char*		buf()			{ return buf_; }
    const unsigned char*	buf() const		{ return buf_; }

protected:

    unsigned char	buf_[SegyBinHeaderLength];
    bool		forwrite_;
    bool		needswap_;

public:

    // 'well-known' headers, not often abused
    static int		EntryJobID()		{ return 0; }
    static int		EntryLino()		{ return 1; }
    static int		EntryDt()		{ return 5; }
    static int		EntryNs()		{ return 7; }
    static int		EntryFmt()		{ return 9; }
    static int		EntryTsort()		{ return 11; }
    static int		EntryMFeet()		{ return 24; }
    static int		EntryRevCode()		{ return 147; }

};


mExpClass(Seis) TrcHeader
{
public:

			TrcHeader(unsigned char*,const TrcHeaderDef&,bool rev0,
				  bool manbuf=false);
			TrcHeader( const TrcHeader& oth )
			    : buf_(0), mybuf_(false), hdef_(oth.hdef_)
			{ *this = oth; }
    void		initRead(); //!< must call once before first usage
			~TrcHeader();
    TrcHeader&		operator =(const TrcHeader&);

    static const HdrDef& hdrDef();

    static void		fillRev1Def(TrcHeaderDef&);

    unsigned short	nrSamples() const;
    void		putSampling(SamplingData<float>,unsigned short);

    void		use(const SeisTrcInfo&);
    void		fill(SeisTrcInfo&,float) const;
    void		setNeedSwap( bool yn=true )	{ needswap_ = yn; }

    float		postScale(int numbfmt) const;
    Coord		getCoord(bool rcv,float extcoordsc) const;

    unsigned char*	buf_;
    const TrcHeaderDef&	hdef_;
    bool		isrev0_;

    bool		isusable; // trid < 2 ; mostly ignored but not always
    bool		nonrectcoords; // counit == 1, 2 or 3

    void		dump(od_ostream&) const;

protected:

    bool		mybuf_;
    bool		needswap_;
    int			previnl_;
    int			seqnr_;
    int			lineseqnr_;

    double		getCoordScale(float extcoordsc) const;

    void		putRev1Flds(const SeisTrcInfo&) const;
    void		getRev1Flds(SeisTrcInfo&) const;

public:

    // 'well-known' headers
    static int	EntryTracl()		{ return 0; }
    static int	EntryTracr()		{ return 1; }
    static int	EntryFldr()		{ return 2; }
    static int	EntryOldSP()		{ return 4; }
    static int	EntryCdp()		{ return 5; }
    static int	EntryTrid()		{ return 7; }
    static int	EntryDUse()		{ return 10; }
    static int	EntryOffset()		{ return 11; }
    static int	EntryScalel()		{ return 19; }
    static int	EntryScalco()		{ return 20; }
    static int	EntrySx()		{ return 21; }
    static int	EntrySy()		{ return 22; }
    static int	EntryGx()		{ return 23; }
    static int	EntryGy()		{ return 24; }
    static int	EntryCoUnit()		{ return 25; }
    static int	EntryLagA()		{ return 33; }
    static int	EntryLagB()		{ return 34; }
    static int	EntryDelRt()		{ return 35; }
    static int	EntryNs()		{ return 38; }
    static int	EntryDt()		{ return 39; }
    static int	EntryTrwf()		{ return 65; }
    static int	EntryXcdp()		{ return 71; }
    static int	EntryYcdp()		{ return 72; }
    static int	EntryInline()		{ return 73; }
    static int	EntryCrossline()	{ return 74; }
    static int	EntrySP()		{ return 75; }
    static int	EntrySPscale()		{ return 76; }

    inline int	entryVal( int idx ) const
		{ return hdrDef()[idx]->getValue(buf_,needswap_); }
    inline void	setEntryVal( int idx, int val ) const
		{ return hdrDef()[idx]->putValue(buf_,val); }

};

} // namespace SEGY
