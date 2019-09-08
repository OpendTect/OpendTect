#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2008
________________________________________________________________________

-*/

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

					DirectReader()
					    : tr_(0), curfilenr_(-1)	{}
    virtual				~DirectReader();

    virtual DirectDef*			getDef()		= 0;
    virtual uiString			errMsg() const		= 0;
    virtual SEGYSeisTrcTranslator*	getTranslator()		{ return tr_; }
    virtual bool			goTo(const BinID&)	= 0;

protected:

    mutable SEGYSeisTrcTranslator* tr_;
    mutable int			curfilenr_;

};

}


/*!\brief reads from 3D PS data store reader based on SEG-Y files */

mExpClass(Seis) SEGYDirect3DPSReader : public ::SeisPS3DReader
			    , public SEGY::DirectReader
{ mODTextTranslationClass(SEGYDirect3DPSReader);
public:

			SEGYDirect3DPSReader(const char* fnm);
			// Check errMsg() to see failure
			~SEGYDirect3DPSReader();

    SeisTrc*		getTrace(const TrcKey&,int) const;
    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const TrcKey&,SeisTrcBuf&) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    uiString		errMsg() const		{ return errmsg_; }

    const PosInfo::CubeData& posData() const;

    virtual SEGY::DirectDef*	getDef()	{ return &def_; }
    virtual bool		goTo(const BinID&);

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

    SeisTrc*		getTrace(const TrcKey&,int) const;
    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const TrcKey&,SeisTrcBuf&) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    uiString		errMsg() const		{ return errmsg_; }

    const PosInfo::Line2DData& posData() const;

    virtual SEGY::DirectDef*	getDef()	{ return &def_; }
    virtual bool		goTo(const BinID&);

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

    virtual bool	isUserSelectable( bool fr ) const { return fr; }
    virtual const char*	iconName() const		{ return "segy"; }
    virtual const char* defExtension() const		{ return fileExt(); }
    static const char*	fileExt()			{ return "sgydef"; }

    static const char*	sKeyNrInlPerFile()
			{ return "Number of inlines per file"; }

};


mExpClass(Seis) SEGYDirectSeisPS2DTranslator : public SeisPS2DTranslator
{ mODTextTranslationClass(SEGYDirectSeisPS2DTranslator);
  isTranslator(SEGYDirect,SeisPS2D)
public:
			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS2D)

    virtual bool	isUserSelectable( bool fr ) const { return fr; }

};


mExpClass(Seis) SEGYDirectSeisTrcTranslator : public SeisTrcTranslator
					    , public SEGY::DirectReader
{ mODTextTranslationClass(SEGYDirectSeisTrcTranslator);
  isTranslator(SEGYDirect,SeisTrc)
public:

      mUseType( SEGY,	DirectDef );

			SEGYDirectSeisTrcTranslator(const char*,const char*);
			~SEGYDirectSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgydef"; }

    bool	readInfo(SeisTrcInfo&) override;
    bool	skip(int) override;
    bool	supportsGoTo() const override	{ return true; }
    bool	isUserSelectable( bool fr ) const override { return true; }
    Conn*	curConn() override;
    BinID	curBinID() const;
    bool	forRead() const override	{ return forread_; }

    void	toSupported(DataCharacteristics&) const;
    bool	write(const SeisTrc&) override; // no buffer/resort needed

    void	usePar(const IOPar&) override;

    bool	close() override;
    void	cleanUp() override;
    IOObj*	createWriteIOObj(const IOObjContext&,
				 const DBKey&) const override;
    const char*	iconName() const override	{ return "segy"; }

    bool	getGeometryInfo(LineCollData&) const override;
    bool	goTo(const BinID&) override;
    uiString	errMsg() const  override { return SeisTrcTranslator::errMsg(); }

    virtual SEGY::DirectDef* getDef()	{ return def_; }
    static SEGYSeisTrcTranslator* createTranslator(const SEGY::DirectDef& def,
					       int filenr,bool commsel=true);
protected:

    bool		commitSelections_() override;
    virtual bool	initRead_() override;
    virtual bool	initWrite_(const SeisTrc&) override;

    SEGY::DirectDef*	def_;
    SEGY::FileDataSet*	fds_;
    int			ild_;
    int			iseg_;
    int			itrc_;
    bool		forread_;
    IOPar		segypars_;
    BufferString	segyfilename_;
    BufferString	segydeffilename_;

    void		initVars(bool for_read=true);
    const PosInfo::CubeData& cubeData() const;
    bool		toNextTrace();
    bool		positionTranslator();

private:

    void		setCompDataFromInput();
    virtual bool	readData(TraceData* externalbuf);

};
