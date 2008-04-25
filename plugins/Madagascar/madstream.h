#ifndef madstream_h
#define madstream_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
 * ID       : $Id: madstream.h,v 1.4 2008-04-25 11:10:40 cvsraman Exp $
-*/


#include "strmdata.h"

class BufferString;
class CBVSInfo;
class CubeSampling;
class IOPar;
class SeisTrcReader;
class SeisTrcWriter;

namespace ODMad
{

class MadStream
{
public:
    				MadStream(IOPar&);
				~MadStream();

    const IOPar*		getHeaderPars()		{ return headerpars_; }
    bool			getNextTrace(float*);
    int				getNrSamples() const;
    bool			putHeader(std::ostream&);
    bool			writeTraces();

    bool			isOK() const;
    const char*			errMsg() const;

protected:

    bool			iswrite_;
    bool			is2d_;
    bool			isps_;
    IOPar&			pars_;
    IOPar*			headerpars_;
    BufferString&		errmsg_;

    std::istream*		istrm_;
    std::ostream*		ostrm_;

    SeisTrcReader*		seisrdr_;
    SeisTrcWriter*		seiswrr_;

    void			initRead(IOPar*);
    void			initWrite(IOPar*);
    void			fillHeaderParsFromStream();
    void			fillHeaderParsFromSeis();
    bool			write2DTraces();
    BufferString		getPosFileName(bool forread=false) const;
};


} // namespace ODMad

#endif
