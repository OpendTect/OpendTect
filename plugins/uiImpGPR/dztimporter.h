#ifndef dztimporter_h
#define dztimporter_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2010
 RCS:		$Id: dztimporter.h,v 1.1 2010-03-16 09:28:32 cvsbert Exp $
________________________________________________________________________

*/

#include "executor.h"
#include "datainterp.h"
#include "linekey.h"
class IOObj;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcWriter;
class StreamData;


namespace DZT
{

struct FileHeader
{
    struct Date
    {
	unsigned	sec2:5, min:6, hour:5, day:5, month:4, year:7;
    };

			FileHeader() : nsamp(0)	{}

    bool		getFrom(std::istream&,BufferString&);
    inline bool		isOK() const		{ return nsamp > 0; }

    inline int		traceNr( int trcidx ) const
			{ return 1 + trcidx * sps; }
    inline int		nrBytesPerSample() const
    			{ return bits / 8; }
    inline int		nrBytesPerTrace() const
			{ return nsamp * nrBytesPerSample(); }
    void		fillInfo(SeisTrcInfo&,int trcidx) const;

    unsigned short	tag, data, nsamp, bits;
    short		zero;
    float		sps, spm, mpm, position, range;
    Date		created, modified;
    unsigned short	npass, rgain, nrgain, text, ntext, proc, nproc, nchan;
    float		epsr, top, depth;
    char		dtype, antname[14];
    unsigned short	chanmask, chksum;

};


class Importer : public ::Executor
{
public:

			Importer(const char* fnm,const IOObj&,const LineKey&);
			~Importer();

    const char*		message() const		{ return msg_; }
    const char*		nrDoneText() const	{ return "Traces handled"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }

    int			nextStep();

protected:

    SeisTrc&		trc_;
    SeisTrcWriter*	wrr_;
    StreamData&		sd_;
    LineKey		lk_;

    FileHeader		fh_;
    char*		databuf_;
    DataInterpreter<float> di_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    BufferString	msg_;

    int			closeAll();

};

} // namespace DZT


#endif
