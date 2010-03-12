#ifndef segythdef_h
#define segythdef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: segythdef.h,v 1.11 2010-03-12 14:58:23 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "bufstring.h"
class IOPar;
class SeisPacketInfo;

namespace SEGY
{


/*!\brief class stores offset info in trace header.

The actual buffer offset needs - 1.

*/

mClass TrcHeaderDef
{
public:
			TrcHeaderDef(
				unsigned char i=9,
				unsigned char c=21,
				unsigned char x=73,
				unsigned char y=77,
				unsigned char t=5,
				unsigned char o=37,
				unsigned char a=255,
				unsigned char pk=255,
				unsigned char rp=255,
				unsigned char ibs=4,
				unsigned char cbs=4,
				unsigned char obs=4,
				unsigned char abs=4,
				unsigned char tbs=4,
				unsigned char pkbs=4,
				unsigned char rpbs=4)
			: inl(i), crl(c), xcoord(x), ycoord(y), trnr(t)
			, offs(o), azim(a), pick(pk), refnr(rp)
			, inlbytesz(ibs), crlbytesz(cbs), trnrbytesz(tbs)
			, refnrbytesz(rpbs), pickbytesz(pkbs)
			, offsbytesz(obs), azimbytesz(abs), pinfo(0)	{}

    unsigned char	inl, inlbytesz;
    unsigned char	crl, crlbytesz;
    unsigned char	trnr, trnrbytesz;
    unsigned char	offs, offsbytesz;
    unsigned char	azim, azimbytesz;
    unsigned char	refnr, refnrbytesz;
    unsigned char	pick, pickbytesz;
    unsigned char	xcoord, ycoord;

    inline static bool	isReserved( unsigned char b, unsigned char sb )
                        { return sb < ((short)b)+3 && sb > ((short)b)-3; }

    inline bool         isClashing( unsigned char b ) const
                        {
                            return isReserved( b, xcoord )
				|| isReserved( b, ycoord )
				|| isReserved( b, pick )
				|| isReserved( b, refnr )
				|| isReserved( b, inl )
				|| isReserved( b, crl )
				|| isReserved( b, offs )
				|| isReserved( b, azim )
				|| isReserved( b, trnr );
                        }

    void		usePar(const IOPar&);
    void		fromSettings();
    void		fillPar(IOPar&,const char* key="SEG-Y") const;

    static const char*	sXCoordByte();
    static const char*	sYCoordByte();
    static const char*	sInlByte();
    static const char*	sInlByteSz();
    static const char*	sCrlByte();
    static const char*	sCrlByteSz();
    static const char*	sTrNrByte();
    static const char*	sTrNrByteSz();
    static const char*	sOffsByte();
    static const char*	sOffsByteSz();
    static const char*	sAzimByte();
    static const char*	sAzimByteSz();
    static const char*	sPickByte();
    static const char*	sPickByteSz();
    static const char*	sRefNrByte();
    static const char*	sRefNrByteSz();

    BufferString	linename;
    SeisPacketInfo*	pinfo;
};


} // namespace

#endif
