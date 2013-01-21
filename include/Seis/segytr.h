#ifndef segytr_h
#define segytr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id$
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include "seismod.h"
#include "segyfiledef.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "strmdata.h"
class LinScaler;
class BendPoints2Coords;
namespace SEGY { class TxtHeader; class BinHeader; class TrcHeader; }

#define mSEGYTraceHeaderBytes	240


mExpClass(Seis) SEGYSeisTrcTranslator : public SeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgy"; }

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int);
    bool		goToTrace(int);
    int			traceSizeOnDisk() const;
    bool		getFullTrcAsBuf(unsigned char*);

    bool		isRev1() const;
    int			numberFormat() const	{ return filepars_.fmt_; }
    int			estimatedNrTraces() const { return estnrtrcs_; }

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

    const SEGY::TxtHeader* txtHeader() const	{ return txthead_; }
    const SEGY::BinHeader& binHeader() const	{ return binhead_; }
    const SEGY::TrcHeader& trcHeader() const	{ return trchead_; }
    void		setTxtHeader(SEGY::TxtHeader*);	//!< write; becomes mine
    void		setForceRev0( bool yn )	{ forcerev0_ = yn; }

    int			dataBytes() const;
    bool		rev0Forced() const	{ return forcerev0_; }
    SEGY::FilePars&	filePars()		{ return filepars_; }
    SEGY::FileReadOpts&	fileReadOpts()		{ return fileopts_; }
    const unsigned char* blockBuf() const	{ return blockbuf_; }

    bool		implShouldRemove(const IOObj*) const { return false; }
    void		cleanUp();

protected:

    SEGY::FilePars	filepars_;
    SEGY::FileReadOpts	fileopts_;
    SEGY::TxtHeader*	txthead_;
    SEGY::BinHeader&	binhead_;
    SEGY::TrcHeader&	trchead_; // must be *after* fileopts_
    LinScaler*		trcscale_;
    const LinScaler*	curtrcscale_;
    bool		forcerev0_;

    bool		useinpsd_;
    TraceDataInterpreter* storinterp_;
    unsigned char	headerbuf_[mSEGYTraceHeaderBytes];
    bool		headerdone_;
    bool		headerbufread_;

    // Following variables are inited by commitSelections
    unsigned char*	blockbuf_;
    ComponentData*	inpcd_;
    TargetComponentData* outcd_;
    SamplingData<int>	offsdef_;

    inline StreamConn&	sConn()		{ return *(StreamConn*)conn; }

    bool		commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	writeTrc_(const SeisTrc&);

    bool		readTraceHeadBuffer();
    bool		readDataToBuf();
    bool		readData(SeisTrc&);
    bool		writeData(const SeisTrc&);
    virtual bool	readTapeHeader();
    virtual void	updateCDFromBuf();
    virtual void	interpretBuf(SeisTrcInfo&);
    virtual bool	writeTapeHeader();
    virtual void	fillHeaderBuf(const SeisTrc&);
    void		selectWriteDataChar(DataCharacteristics&) const;
    void		fillErrMsg(const char*,bool);
    bool		noErrMsg();

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;
    void		addWarn(int,const char*);
    const char*		getTrcPosStr() const;
    bool		tryInterpretBuf(SeisTrcInfo&);
    bool		skipThisTrace(SeisTrcInfo&,int&);

    int			curtrcnr_, prevtrcnr_;
    BinID		curbid_, prevbid_;
    float		curoffs_, prevoffs_;
    Coord		curcoord_;
    BendPoints2Coords*	bp2c_;
    int			estnrtrcs_;
    bool		othdomain_;

};


#endif

