#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-4-1996
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include "seismod.h"
#include "segyfiledef.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "strmdata.h"
#include "uistring.h"
#include "coordsystem.h"

class LinScaler;
class BendPoints2Coords;
namespace SEGY { class TxtHeader; class BinHeader; class TrcHeader; }

#define mSEGYTraceHeaderBytes	240


mExpClass(Seis) SEGYSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SEGYSeisTrcTranslator); isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgy"; }

    virtual bool	readInfo(SeisTrcInfo&);
    virtual bool	read(SeisTrc&);
    virtual bool	skip(int);
    bool		goToTrace(int);
    int			traceSizeOnDisk() const;
    bool		getFullTrcAsBuf(unsigned char*);

    bool		isRev0() const;
    int			numberFormat() const	{ return filepars_.fmt_; }
    int			estimatedNrTraces() const { return estnrtrcs_; }

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

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
    const unsigned char* blockBuf() const	{ return blockbuf_; }

    virtual bool	implManagesObjects( const IOObj* ) const
						{ return true; }
    void		cleanUp();

    void		setCoordSys(Coords::CoordSystem* crs)
						    { coordsys_.set( crs ); }

protected:

    SEGY::FilePars	filepars_;
    SEGY::FileReadOpts	fileopts_;
    SEGY::TxtHeader*	txthead_;
    SEGY::BinHeader&	binhead_;
    SEGY::TrcHeader&	trchead_; // must be *after* fileopts_
    LinScaler*		trcscale_;
    const LinScaler*	curtrcscale_;
    int			forcedrev_;

    bool		useinpsd_;
    TraceDataInterpreter* storinterp_; //Will be removed after 6.2
    unsigned char	headerbuf_[mSEGYTraceHeaderBytes];
    bool		headerdone_; //Will be in base class after 6.2

    // Following variables are inited by commitSelections
    unsigned char*	blockbuf_; //Will be removed after 6.2
    ComponentData*	inpcd_;
    TargetComponentData* outcd_;

    inline StreamConn&	sConn()		{ return *(StreamConn*)conn_; }

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
    void		fillErrMsg(const uiString&, bool);
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
    SEGY::OffsetCalculator offsetcalc_;
    Coord		curcoord_;
    BendPoints2Coords*	bp2c_;
    int			estnrtrcs_;
    bool		othdomain_;
    RefMan<Coords::CoordSystem>     coordsys_;

private:

    friend class SEGYDirectSeisTrcTranslator;

    virtual bool	readData(TraceData* externalbuf);

};


