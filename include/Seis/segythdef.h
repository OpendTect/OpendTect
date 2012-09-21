#ifndef segythdef_h
#define segythdef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "segyhdrdef.h"
#include "bufstring.h"
class IOPar;
class SeisPacketInfo;

namespace SEGY
{

/*!\brief stores HdrEntry info of the needed fields from the trace header.  */

mClass(Seis) TrcHeaderDef
{
public:
			TrcHeaderDef();

    HdrEntry		inl_;
    HdrEntry		crl_;
    HdrEntry		xcoord_;
    HdrEntry		ycoord_;
    HdrEntry		trnr_;
    HdrEntry		offs_;
    HdrEntry		azim_;
    HdrEntry		pick_;
    HdrEntry		refnr_;

    inline bool         isClashing( unsigned char b ) const
                        {
                            return xcoord_.usesByte( b )
				|| ycoord_.usesByte( b )
				|| inl_.usesByte( b )
				|| crl_.usesByte( b )
				|| trnr_.usesByte( b )
				|| offs_.usesByte( b )
				|| azim_.usesByte( b )
				|| pick_.usesByte( b )
				|| refnr_.usesByte( b );
                        }

    void		usePar(const IOPar&);
    void		fromSettings();
    void		fillPar(IOPar&,const char* key="SEG-Y") const;

    static const char*	sXCoordByte();
    static const char*	sYCoordByte();
    static const char*	sInlByte();
    static const char*	sCrlByte();
    static const char*	sTrNrByte();
    static const char*	sOffsByte();
    static const char*	sAzimByte();
    static const char*	sRefNrByte();
    static const char*	sPickByte();

    BufferString	linename;
    SeisPacketInfo*	pinfo;
};


} // namespace

#endif

