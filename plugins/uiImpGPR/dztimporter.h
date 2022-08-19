#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "executor.h"
#include "datainterp.h"
#include "linekey.h"
#include "samplingdata.h"
#include "position.h"
class IOObj;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcWriter;


namespace DZT
{

struct FileHeader
{
    struct Date
    {
	unsigned	sec2:5, min:6, hour:5, day:5, month:4, year:7;
    };

			FileHeader();

    mDeprecatedDef bool	getFrom(od_istream&,BufferString&);
    bool		getFrom(od_istream&,uiString&);
    inline bool		isOK() const		{ return nsamp > 0; }

    inline int		traceNr( int trcidx ) const
			{ return nrdef_.atIndex(trcidx); }
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

    // Could these be found? Not in all files for sure ...
    SamplingData<int>	nrdef_;
    Coord		cstart_;
    Coord		cstep_;

};


mClass(uiImpGPR) Importer : public ::Executor
{ mODTextTranslationClass(Importer);
public:

			Importer(const char* fnm,const IOObj&,const LineKey&);
			~Importer();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return tr("Traces handled"); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }

    int			nextStep();

    FileHeader		fh_;
    float		zfac_;

protected:

    SeisTrc&		trc_;
    SeisTrcWriter*	wrr_;
    od_istream&		istream_;
    LineKey		lk_;

    char*		databuf_;
    DataInterpreter<float> di_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    uiString	msg_;

    int			closeAll();

};

} // namespace DZT
