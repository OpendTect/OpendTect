#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarmod.h"
#include "position.h"
#include "od_iostream.h"
#include "uistring.h"

class SeisTrcReader;
class SeisTrcWriter;
class SeisTrcBuf;
class SeisPSReader;
class SeisPSWriter;

namespace PosInfo { class CubeData; class CubeDataIterator; class Line2DData; }
namespace Seis { class SelData; }

namespace ODMad
{

mExpClass(Madagascar) MadStream
{ mODTextTranslationClass(MadStream);
public:
    				MadStream(IOPar&);
				~MadStream();

    const IOPar*		getHeaderPars()		{ return headerpars_; }
    bool			getNextTrace(float*);
    int				getNrSamples() const;
    bool			putHeader(od_ostream&);
    bool			writeTraces(bool writetofile=true);
    SeisTrcBuf*			getTrcBuf() const	{ return stortrcbuf_; }

    bool			isOK() const;
    uiString			errMsg() const;

    bool			isBinary() const	{ return isbinary_; }
    bool			is2D() const		{ return is2d_; }
    bool			isPS() const		{ return isps_; }
    void			setStorBufMine( bool yn )
				{ stortrcbufismine_ = yn; }

protected:

    static uiString		sNoPositionsInPosFile();
    static uiString		sPosFile();

    bool			iswrite_;
    bool			is2d_;
    bool			isps_;
    bool			isbinary_;
    IOPar&			pars_;
    IOPar*			headerpars_;
    uiString&			errmsg_;

    od_istream*			istrm_;
    od_ostream*			ostrm_;

    SeisTrcReader*		seisrdr_;
    SeisTrcWriter*		seiswrr_;
    SeisPSReader*		psrdr_;
    SeisPSWriter*		pswrr_;

    BinID			curbid_;
    SeisTrcBuf*			trcbuf_;
    SeisTrcBuf*			stortrcbuf_;
    bool			stortrcbufismine_;
    PosInfo::CubeDataIterator*	iter_;
    PosInfo::CubeData*		cubedata_;
    PosInfo::Line2DData*	l2ddata_;
    int				nroffsets_;
    int				curtrcidx_;		// For PS

    void			initRead(IOPar*);
    void			initWrite(IOPar*);
    void			fillHeaderParsFromStream();
    void			fillHeaderParsFromSeis();
    void			fillHeaderParsFromPS(const Seis::SelData*);
    bool			write2DTraces(bool);
    void			readRSFTrace(float*,int) const;
    bool			getNextPos(BinID&);
    BufferString		getPosFileName(bool forread=false) const;
};


} // namespace ODMad
