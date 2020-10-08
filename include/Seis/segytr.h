#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2-4-1996
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include "posinfo.h"
#include "segyfiledef.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "uistring.h"
#include "coordsystem.h"


class BendPoints2Coords;
namespace SEGY { class TxtHeader; class BinHeader; class TrcHeader; }
namespace PosInfo { class LineCollData; }

#define mSEGYTraceHeaderBytes	240


mExpClass(Seis) SEGYSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SEGYSeisTrcTranslator); isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();
    const char*	defExtension() const override	{ return "sgy"; }

    bool		readInfo(SeisTrcInfo&) override;
    bool		readData(TraceData* externalbuf) override;

    bool		skip(int) override;
    bool		goToTrace(int);
    int			traceSizeOnDisk() const;
    bool		getFullTrcAsBuf(unsigned char*);

    bool		isRev0() const;
    int			numberFormat() const	{ return filepars_.fmt_; }
    int			estimatedNrTraces() const { return estnrtrcs_; }

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&) override;

    const SEGY::TxtHeader* txtHeader() const	{ return txthead_; }
    const SEGY::BinHeader& binHeader() const	{ return binhead_; }
    const SEGY::TrcHeader& trcHeader() const	{ return trchead_; }
    void		setTxtHeader(SEGY::TxtHeader*);	//!< write; becomes mine
    void		setForcedRev( int rev )	{ forcedrev_ = rev; }
    void		setForceRev0( bool yn )	{ forcedrev_ = yn ? 0 : 1; }

    int			dataBytes() const;
    int			forcedRev() const	{ return forcedrev_; }
    bool		rev0Forced() const	{ return forcedrev_ == 0; }
    SEGY::FilePars&	filePars()		{ return filepars_; }
    SEGY::FileReadOpts&	fileReadOpts()		{ return fileopts_; }
    void		setSelComp( int icomp )	{ selcomp_ = icomp; }

    virtual bool	implManagesObjects( const IOObj* ) const
						{ return true; }
    void		cleanUp();

    static const char*	sKeyHdrEBCDIC()
			{ return "SEGY.Text Header EBCDIC"; }
    void		setCoordSys(Coords::CoordSystem* crs)
						    { coordsys_.set( crs ); }
    bool		getGeometryInfo(PosInfo::LineCollData&) const override;

protected:

    SEGY::FilePars	filepars_;
    SEGY::FileReadOpts	fileopts_;
    SEGY::TxtHeader*	txthead_;
    SEGY::BinHeader&	binhead_;
    SEGY::TrcHeader&	trchead_; // must be *after* fileopts_
    int			selcomp_;
    int			forcedrev_;

    bool		useinpsd_;
    unsigned char	headerbuf_[mSEGYTraceHeaderBytes];

    // Following variables are inited by commitSelections
    ComponentData*	inpcd_;
    TargetComponentData* outcd_;

    inline StreamConn&	sConn()		{ return *(StreamConn*)conn_; }

    bool	commitSelections_() override;
    bool	initRead_() override;
    bool	initWrite_(const SeisTrc&) override;
    bool	writeTrc_(const SeisTrc&) override;

    bool	readTraceHeadBuffer();
    bool	writeData(const SeisTrc&);
    bool	readTapeHeader();
    void	updateCDFromBuf();
    void	interpretBuf(SeisTrcInfo&);
    bool	writeTapeHeader();
    void	fillHeaderBuf(const SeisTrc&);
    void	selectWriteDataChar(DataCharacteristics&) const;
    void	fillErrMsg(const uiString&, bool);
    bool	noErrMsg();

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;
    virtual void	addWarn(int,const uiString& s=uiString::empty());
    uiString		getTrcPosStr() const;
    bool		tryInterpretBuf(SeisTrcInfo&);
    bool		skipThisTrace(SeisTrcInfo&,int&);

    int			curtrcnr_, prevtrcnr_;
    BinID		curbid_, prevbid_;
    float		curoffs_, prevoffs_;
    SEGY::OffsetCalculator offsetcalc_;
    Coord		curcoord_;
    BendPoints2Coords*	bp2c_;
    int			estnrtrcs_;
    bool		othdomain_;
    RefMan<Coords::CoordSystem>     coordsys_;

private:

    friend class SEGYDirectSeisTrcTranslator;

};
