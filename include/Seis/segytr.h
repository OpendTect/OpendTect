#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.6 2002-04-08 20:58:41 bert Exp $
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include <segylike.h>
#include <segyhdr.h>
class LinScaler;

#define mSegyFmt "Number format"


class SEGYSeisTrcTranslator : public SegylikeSeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char* nm=0);
			~SEGYSeisTrcTranslator();

    int			dataBytes() const;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar*);

    void		dumpToString( bool yn=true )	{ do_string_dump = yn; }
    bool		dumpingDone() const		{ return itrc >= 5; }
    const char*		dumpStr() const			{ return dumpstr; }

    virtual const char*	defExtension() const		{ return "sgy"; }

    static const char*	sExternalNrSamples;
    static const char*	sExternalCoordScaling;
    static const char*	sExternalTimeShift;
    static const char*	sExternalSampleRate;
    static const char*	sUseLiNo;

protected:

    SegyTraceheader	trhead;
    int			itrc;
    int			numbfmt;
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
    bool		use_lino;
    char*		dumpstr;


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
