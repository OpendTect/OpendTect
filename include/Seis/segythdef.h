#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "segyhdrdef.h"
#include "bufstring.h"
class SeisPacketInfo;

namespace SEGY
{

/*!\brief stores HdrEntry info of the needed fields from the trace header.

Note: the HdrEntry values are in 'user' bytes (i.e. subtract 1 for byte
offsets).

 */

mExpClass(Seis) TrcHeaderDef
{
public:
			TrcHeaderDef();
			~TrcHeaderDef();

    HdrEntry		inl_;
    HdrEntry		crl_;
    HdrEntry		xcoord_;
    HdrEntry		ycoord_;
    HdrEntry		trnr_;
    HdrEntry		offs_;
    HdrEntry		azim_;
    HdrEntry		pick_;
    HdrEntry		refnr_;

    inline bool		isClashing( unsigned char b ) const
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

    SeisPacketInfo*	pinfo;
};

} // namespace SEGY
