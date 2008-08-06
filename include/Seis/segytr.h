#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.20 2008-08-06 12:06:24 cvsbert Exp $
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include <segylike.h>
#include <segyhdr.h>
class LinScaler;


class SEGYSeisTrcTranslator : public SegylikeSeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();

    virtual int		dataBytes() const;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

    enum DumpMode	{ None, File, String };
    void		setTrcDumpMode( DumpMode dm )	{ dumpmode = dm; }
    int			nrTrcsDumped() const		{ return itrc; }
    void		setMaxTrcsDumped( int nr )	{ maxnrdump = nr; }
    void		closeTraceDump();
    const char*		dumpStr() const
    			{ return dumpstr.c_str(); }
    const char*		dumpFileName() const;

    virtual const char*	defExtension() const		{ return "sgy"; }

    static const char*	sNumberFormat;
    static const char*	sExternalNrSamples;
    static const char*	sExternalCoordScaling;
    static const char*	sExternalTimeShift;
    static const char*	sExternalSampleRate;
    static const char*	sUseOffset;
    static const char*	sForceRev0;

    bool		isRev1() const		{ return trhead.isrev1; }
    int			numbfmt;

protected:

    SegyTraceheader	trhead;
    int			itrc;
    StreamData&		dumpsd;
    short		binhead_ns;
    float		binhead_dpos;
    LinScaler*		trcscale;
    const LinScaler*	curtrcscale;
    int			ext_nr_samples;
    float		ext_coord_scaling;
    float		ext_time_shift;
    float		ext_sample_rate;
    DumpMode		dumpmode;
    mutable std::string	dumpstr;
    int			maxnrdump;
    bool		force_rev0;

    bool		readTapeHeader();
    void		updateCDFromBuf();
    int			nrSamplesRead() const;
    void		interpretBuf(SeisTrcInfo&);
    bool		writeTapeHeader();
    void		fillHeaderBuf(const SeisTrc&);
    void		toPreSelected(DataCharacteristics&) const;
    const LinScaler*	getTraceScaler() const		{ return curtrcscale; }
    virtual void	toPreferred(DataCharacteristics&) const;

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;

};


#endif
