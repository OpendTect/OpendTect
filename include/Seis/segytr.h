#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "segyfiledef.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "strmdata.h"
#include "uistring.h"

class BendPoints2Coords;
class SeisStoreAccess;
namespace SEGY { class TxtHeader; class BinHeader; class TrcHeader; }

#define mSEGYTraceHeaderBytes	240


mExpClass(Seis) SEGYSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SEGYSeisTrcTranslator); isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();

    const char*		defExtension() const override	{ return "sgy"; }

    bool		readInfo(SeisTrcInfo&) override;
    bool		skip(int) override;
    bool		goToTrace(int);
    int			traceSizeOnDisk() const;
    bool		getFullTrcAsBuf(unsigned char*);

    bool		isRev0() const;
    int			numberFormat() const	{ return filepars_.fmt_; }
    int			estimatedNrTraces() const override;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&) override;

    const SEGY::TxtHeader* txtHeader() const	{ return txthead_; }
    const SEGY::BinHeader& binHeader() const	{ return binhead_; }
    const SEGY::TrcHeader& trcHeader() const	{ return trchead_; }
    void		setTxtHeader(SEGY::TxtHeader*); //!< write; becomes mine
    void		setForcedRev( int rev ) { forcedrev_ = rev; }
    void		setForceRev0( bool yn ) { forcedrev_ = yn ? 0 : 1; }

    int			dataBytes() const;
    int			forcedRev() const	{ return forcedrev_; }
    bool		rev0Forced() const	{ return forcedrev_ == 0; }
    SEGY::FilePars&	filePars()		{ return filepars_; }
    SEGY::FileReadOpts& fileReadOpts()		{ return fileopts_; }

    bool		implIsLink( const IOObj* ) const override
			{ return true; }
    bool		implRemove( const IOObj*, bool ) const override
			{ return true; }

    void		cleanUp() override;

    void		setIs2D(bool yn) override;
    void		setIsPS(bool yn) override;

    static bool		writeSEGYHeader(const SeisStoreAccess&,const char* fnm);
			//<! Will enforce sgyhdr extension

protected:

    SEGY::FilePars	filepars_;
    SEGY::FileReadOpts	fileopts_;
    SEGY::TxtHeader*	txthead_ = nullptr;
    SEGY::BinHeader&	binhead_;
    SEGY::TrcHeader&	trchead_; // must be *after* fileopts_
    int			forcedrev_ = -1;

    bool		useinpsd_ = false;
    unsigned char	headerbuf_[mSEGYTraceHeaderBytes];

    // Following variables are inited by commitSelections
    ComponentData*	inpcd_ = nullptr;
    TargetComponentData* outcd_ = nullptr;

    inline StreamConn&	sConn()		{ return *(StreamConn*)conn_; }

    bool		commitSelections_() override;
    bool		initRead_() override;
    bool		initWrite_(const SeisTrc&) override;
    bool		writeTrc_(const SeisTrc&) override;

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
    void		fillErrMsg(const uiString&, bool);
    bool		noErrMsg();

    DataCharacteristics getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;
    void		addWarn(int,const char*) override;
    const char*		getTrcPosStr() const;
    bool		tryInterpretBuf(SeisTrcInfo&);
    bool		skipThisTrace(SeisTrcInfo&,int&);

    int			curtrcnr_, prevtrcnr_;
    BinID		curbid_, prevbid_;
    float		curoffs_ = -1.f;
    float		prevoffs_ = 0.f;
    SEGY::OffsetCalculator offsetcalc_;
    Coord		curcoord_;
    BendPoints2Coords*	bp2c_ = nullptr;
    int			estnrtrcs_ = -1;
    bool		othdomain_ = false;

private:

    friend class SEGYDirectSeisTrcTranslator;

    bool		readData(TraceData* externalbuf) override;

public:

    mDeprecatedObs
    const unsigned char* blockBuf() const	{ return nullptr; }

};
