#ifndef madstream_h
#define madstream_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
 * ID       : $Id: madstream.h,v 1.1 2008-03-31 11:20:50 cvsraman Exp $
-*/


#include "strmdata.h"

class CubeSampling;
class SeisTrcReader;
class SeisTrcWriter;
class IOPar;

namespace ODMad
{

class MadStream
{
public:
    				MadStream(const IOPar&,bool isread=true);
				~MadStream();

    bool			getNextTrace(float*);
    int				getNrSamples() const;
    bool			putHeader(std::ostream&);
    bool			writeTraces();
    bool			isRead()		{ return isread_; }
protected:

    bool			isread_;
    IOPar*			headerpars_;

    std::istream*		istrm_;
    std::ostream*		ostrm_;

    SeisTrcReader*		seisrdr_;
    SeisTrcWriter*		seiswrr_;

    void			fillHeaderPars();
    void			fillHeaderPars(const CubeSampling&);
};


} // namespace ODMad

#endif
