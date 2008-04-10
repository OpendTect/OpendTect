#ifndef madstream_h
#define madstream_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
 * ID       : $Id: madstream.h,v 1.3 2008-04-10 04:00:33 cvsraman Exp $
-*/


#include "strmdata.h"

class BufferString;
class CubeSampling;
class IOPar;
class SeisTrcReader;
class SeisTrcWriter;

namespace ODMad
{

class MadStream
{
public:
    				MadStream(const IOPar&);
				~MadStream();

    bool			getNextTrace(float*);
    int				getNrSamples() const;
    bool			putHeader(std::ostream&);
    bool			writeTraces();

    bool			isOK() const;
    const char*			errMsg() const;

protected:

    bool			iswrite_;
    IOPar*			headerpars_;
    BufferString&		errmsg_;

    std::istream*		istrm_;
    std::ostream*		ostrm_;

    SeisTrcReader*		seisrdr_;
    SeisTrcWriter*		seiswrr_;

    void			initRead(IOPar*);
    void			initWrite(IOPar*);
    void			fillHeaderPars();
    void			fillHeaderPars(const CubeSampling&);
};


} // namespace ODMad

#endif
