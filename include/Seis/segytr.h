#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.2 2001-04-04 11:13:53 bert Exp $
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include <segylike.h>
#include <segyhdr.h>

#define mSegyFmt "Number format"


class SEGYSeisTrcTranslator : public SegylikeSeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char* nm=0);
			~SEGYSeisTrcTranslator();

    int			dataBytes() const;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar*);

private:

    SegyTraceheader	trhead;
    int			itrc;
    int			numbfmt;
    StreamData&		dumpsd;
    short		binhead_ns;
    float		binhead_dpos;

    bool		readTapeHeader();
    void		updateCDFromBuf();
    int			nrSamplesRead() const;
    void		interpretBuf(SeisTrcInfo&);
    bool		writeTapeHeader();
    void		fillHeaderBuf(const SeisTrc&);
    void		toPreSelected(DataCharacteristics&) const;

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;

};


#endif
