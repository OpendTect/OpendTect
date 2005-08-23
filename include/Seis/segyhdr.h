#ifndef segyhdr_h
#define segyhdr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: segyhdr.h,v 1.13 2005-08-23 16:49:37 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "seisinfo.h"
#include "segythdef.h"
#include "iosfwd"
class BufferString;

#define SegyTxtHeaderLength		3200
#define SegyBinHeaderLength		400
#define SegyBinHeaderUnassShorts	170
#define SegyTracHeaderLength		240


/*!\brief 3200 byte SEG-Y text header.

  On construction, the 'txt' buffer is filled with data for writing the header.
  If used for reading, fill the buffer yourself and use getFrom.
 
 */


class SegyTxtHeader
{
public:
    		SegyTxtHeader(bool rev1=true); //!< rev1 relevant when writing
 
    void	setUserInfo(const char*);
    void	setPosInfo(const SegyTraceheaderDef&);
    void	setStartPos(float);
    void	getText(BufferString&);

    void	putAt(int,int,int,const char*);
    void	getFrom(int,int,int,char*) const;
    void	print(std::ostream&) const;
 
    void        setAscii();
    void        setEbcdic();

    unsigned char txt[SegyTxtHeaderLength];

    static bool	info2d;

};


/*!\brief 400 byte SEG-Y binary header.

  On construction, the 'txt' buffer is filled with data for writing the header.
  If used for reading, fill the buffer yourself and use getFrom.
 
 */

class SegyBinHeader
{
public:

		SegyBinHeader(bool rev1=true); //!< rev1 relevant when writing
    void	getFrom(const void*);
    void	putTo(void*) const;
    void	print(std::ostream&) const;
    int		bytesPerSample() const
		{ return formatBytes( format ); }
    static int	formatBytes( int frmt )
		{ return frmt == 3 ? 2 : (frmt == 8 ? 1 : 4); }

    bool	needswap;

    int		jobid;	/* job identification number */
    int		lino;	/* line number (only one line per reel) */
    int		reno;	/* reel number */
    short	ntrpr;	/* number of data traces per record */
    short	nart;	/* number of auxiliary traces per record */
    short	hdt;	/* sample interval in micro secs for this reel */
    short	dto;	/* same for original field recording */
    short	hns;	/* number of samples per trace for this reel */
    short	nso;	/* same for original field recording */
    short	format;	/* data sample format code:
                            1 = floating point (4 bytes)
                            2 = fixed point (4 bytes)
                            3 = fixed point (2 bytes)
                            4 = fixed point w/gain code (4 bytes)
			       + SEG-Y rev 1:
                            5 = IEEE float (4 bytes)
                            8 = signed char (1 byte)
		    */
    short	fold, tsort, vscode, hsfs, hsfe, hslen, hstyp, schn, hstas,
		hstae, htatyp, hcorr, bgrcv, rcvm;

    short	mfeet;    /* measurement system code: 1 = meters 2 = feet */

    short	polyt, vpol;
    short	hunass[SegyBinHeaderUnassShorts];

    short	isrev1; //!< This must be considered final
    		// Rev 1 only
    short	fixdsz;	
    short	nrstzs;
};


class SegyTraceheader
{
public:

			SegyTraceheader( unsigned char* b, bool rev1,
					 SegyTraceheaderDef& hd )
			: buf(b)
			, hdef(hd)
			, isrev1(rev1)
			, needswap(false), seqnr(1)		{}

    void		print(std::ostream&) const;

    unsigned short	nrSamples() const;
    void		putSampling(SamplingData<float>,unsigned short);

    void		use(const SeisTrcInfo&);
    void		fill(SeisTrcInfo&,float) const;

    float		postScale(int numbfmt) const;
    Coord		getCoord(bool rcv,float extcoordsc);

    unsigned char*	buf;
    bool		needswap;
    SegyTraceheaderDef&	hdef;
    int			seqnr;
    bool		isrev1;

protected:

    double		getCoordScale(float extcoordsc) const;

};


#endif
