#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.12 2003-11-07 12:21:52 bert Exp $
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

    int			dataBytes() const;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

    void		dumpToString( bool yn=true )	{ do_string_dump = yn; }
    bool		dumpingDone() const		{ return itrc >= 5; }
    const char*		dumpStr() const		{ return dumpstr.c_str(); }

    virtual const char*	defExtension() const		{ return "sgy"; }

    static const char*	sNumberFormat;
    static const char*	sExternalNrSamples;
    static const char*	sExternalCoordScaling;
    static const char*	sExternalTimeShift;
    static const char*	sExternalSampleRate;
    static const char*	sUseLiNo;

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
    bool		do_string_dump;
    mutable string	dumpstr;
    bool		use_lino;

    bool		readTapeHeader();
    void		updateCDFromBuf();
    int			nrSamplesRead() const;
    void		interpretBuf(SeisTrcInfo&);
    bool		writeTapeHeader();
    void		fillHeaderBuf(const SeisTrc&);
    void		toPreSelected(DataCharacteristics&) const;
    const LinScaler*	getTraceScaler() const		{ return curtrcscale; }

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;

};


#endif
