#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seispsread.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "uistring.h"

namespace SEGY { class DirectDef; }
namespace PosInfo { class CubeData; }
class SEGYSeisTrcTranslator;
class StreamConn;

#define mSEGYDirectTranslNm "SEGYDirect"


/*!\brief Base class for readers from SEG-Y file sets */

namespace SEGY
{
class FileDataSet;


mExpClass(Seis) DirectReader
{ mODTextTranslationClass(DirectReader);
public:

					DirectReader();
    virtual				~DirectReader();

    virtual DirectDef*			getDef()		= 0;
    virtual uiString			errMsg() const		= 0;
    virtual SEGYSeisTrcTranslator*	getTranslator()		{ return tr_; }
    virtual bool			goTo(const BinID&)	= 0;

protected:

    mutable SEGYSeisTrcTranslator* tr_ = nullptr;
    mutable int			curfilenr_ = -1;

};

} // namespace SEGY


/*!\brief reads from 3D PS data store reader based on SEG-Y files */

mExpClass(Seis) SEGYDirect3DPSReader : public ::SeisPS3DReader
			    , public SEGY::DirectReader
{ mODTextTranslationClass(SEGYDirect3DPSReader);
public:

			SEGYDirect3DPSReader(const char* fnm);
			// Check errMsg() to see failure
			~SEGYDirect3DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const override;
    bool		getGather(const BinID&,SeisTrcBuf&) const override;
    uiString		errMsg() const override		{ return errmsg_; }

    const PosInfo::CubeData& posData() const override;

    SEGY::DirectDef*	getDef() override		{ return &def_; }
    bool		goTo(const BinID&) override;

protected:

    SEGY::DirectDef&	def_;
    mutable uiString	errmsg_;

    SeisTrc*		getTrace(int,int,const BinID&) const;
    bool		goTo(int,int) const;

};


/*!\brief reads from 2D PS data store reader based on SEG-Y files */

mExpClass(Seis) SEGYDirect2DPSReader : public SeisPS2DReader
			    , public SEGY::DirectReader
{ mODTextTranslationClass(SEGYDirect2DPSReader);
public:

			SEGYDirect2DPSReader(const char* dirnm,Pos::GeomID);
			SEGYDirect2DPSReader(const char* dirnm,const char* lnm);
			// Check errMsg() to see failure
			~SEGYDirect2DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const override;
    bool		getGather(const BinID&,SeisTrcBuf&) const override;
    uiString		errMsg() const override		{ return errmsg_; }

    const PosInfo::Line2DData& posData() const override;

    SEGY::DirectDef*	getDef() override		{ return &def_; }
    bool		goTo(const BinID&) override;

protected:

    SEGY::DirectDef&		def_;
    mutable uiString		errmsg_;

    SeisTrc*			getTrace(int,int,int) const;
    bool			goTo(int,int) const;
};



mExpClass(Seis) SEGYDirectSeisPS3DTranslator : public SeisPS3DTranslator
{ mODTextTranslationClass(SEGYDirectSeisPS3DTranslator);
  isTranslator(SEGYDirect,SeisPS3D)
public:
			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS3D)
			~SEGYDirectSeisPS3DTranslator();

    bool		isUserSelectable( bool fr ) const override
			{ return fr; }

    const char*		iconName() const override	{ return "segy"; }
    const char*		defExtension() const override	{ return fileExt(); }
    static const char*	fileExt()			{ return "sgydef"; }

    static const char*	sKeyNrInlPerFile()
			{ return "Number of inlines per file"; }

};


mExpClass(Seis) SEGYDirectSeisPS2DTranslator : public SeisPS2DTranslator
{ mODTextTranslationClass(SEGYDirectSeisPS2DTranslator);
  isTranslator(SEGYDirect,SeisPS2D)
public:
			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS2D)
			~SEGYDirectSeisPS2DTranslator();

    bool		isUserSelectable( bool fr ) const override
			{ return fr; }

};


mExpClass(Seis) SEGYDirectSeisTrcTranslator : public SeisTrcTranslator
					    , public SEGY::DirectReader
{ mODTextTranslationClass(SEGYDirectSeisTrcTranslator);
  isTranslator(SEGYDirect,SeisTrc)
public:

			SEGYDirectSeisTrcTranslator(const char*,const char*);
			~SEGYDirectSeisTrcTranslator();

    const char*		defExtension() const override	{ return "sgydef"; }

    bool		readInfo(SeisTrcInfo&) override;
    bool		read(SeisTrc&) override;
    bool		skip(int) override;
    bool		supportsGoTo() const override		{ return true; }
    bool		isUserSelectable(bool) const override	{ return true; }
    Conn*		curConn() override;
    virtual BinID	curBinID() const;
    bool		forRead() const override	{ return forread_; }
    bool		getConfirmRemoveMsg(const IOObj*,uiString& msg,
					    uiString& canceltxt,
					    uiString& yestxt,
					    uiString& notxt) const override;

    virtual void	toSupported(DataCharacteristics&) const;
    bool		write(const SeisTrc&) override;
			// no buffer/resort needed

    void		usePar(const IOPar&) override;

    bool		implRemove(const IOObj*,bool) const override;
    bool		close() override;
    void		cleanUp() override;
    IOObj*		createWriteIOObj(const IOObjContext&,
					 const MultiID&) const override;
    const char*		iconName() const override	{ return "segy"; }

    bool		getGeometryInfo(PosInfo::CubeData&) const override;
    SEGY::DirectDef*	getDef() override		{ return def_; }
    bool		goTo(const BinID&) override;
    uiString		errMsg() const override
			{ return SeisTrcTranslator::errMsg(); }

    static SEGYSeisTrcTranslator* createTranslator(const SEGY::DirectDef& def,
						   int filenr);
protected:

    bool		commitSelections_() override;
    bool		initRead_() override;
    bool		initWrite_(const SeisTrc&) override;

    SEGY::DirectDef*	def_ = nullptr;
    SEGY::FileDataSet*	fds_ = nullptr;
    int			ild_;
    int			iseg_;
    int			itrc_;
    bool		forread_ = true;
    IOPar		segypars_;
    BufferString	segyfilename_;
    BufferString	segydeffilename_;

    void		initVars(bool for_read=true);
    const PosInfo::CubeData& cubeData() const;
    bool		toNextTrace();
    bool		positionTranslator();

private:

    void		setCompDataFromInput();
    bool		readData(TraceData* externalbuf) override;

};
