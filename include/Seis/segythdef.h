#ifndef segythdef_h
#define segythdef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: segythdef.h,v 1.2 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

-*/
 
#include <gendefs.h>
class IOPar;


/*!\brief class stores offset info in trace header.

The actual buffer offset needs - 1.

*/

class SegyTraceheaderDef
{
public:
			SegyTraceheaderDef(
				unsigned char i=9,
				unsigned char c=21,
				unsigned char x=73,
				unsigned char y=77,
				unsigned char p=255,
				unsigned char ibs=4,
				unsigned char cbs=4 )
			: inl(i), crl(c), xcoord(x), ycoord(y), pick(p)
			, inlbytesz(ibs), crlbytesz(cbs)	{}

    unsigned char	inl, inlbytesz;
    unsigned char	crl, crlbytesz;
    unsigned char	xcoord, ycoord;
    unsigned char	pick;

    inline static bool	isReserved( unsigned char b, unsigned char sb )
                        { return sb < b+3 && sb > ((int)b)-3; }

    inline bool         isClashing( unsigned char b ) const
                        {
                            return isReserved( b, xcoord )
                                || isReserved( b, ycoord )
                                || isReserved( b, pick )
                                || isReserved( b, inl )
                                || isReserved( b, crl );
                        }

    void		usePar(const IOPar&);
    void		fromSettings();
    void		fillPar(IOPar&,const char* key="SEG-Y") const;

    static const char*	sXCoordByte;
    static const char*	sYCoordByte;
    static const char*	sInlByte;
    static const char*	sCrlByte;
    static const char*	sInlByteSz;
    static const char*	sCrlByteSz;
    static const char*	sPickByte;

};


#endif
