#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2008
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

					DirectReader()		{}
    virtual				~DirectReader();

    virtual DirectDef*			getDef()		= 0;
    virtual uiString			errMsg() const		= 0;
    virtual SEGYSeisTrcTranslator*	getTranslator()		{ return tr_; }
    virtual bool			goTo(const BinID&)	= 0;

protected:

    mutable SEGYSeisTrcTranslator* tr_ = nullptr;
    mutable int			curfilenr_ = -1;

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

    SeisTrc*		getTrace(const BinID&,int) const;
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

    SeisTrc*		getTrace(const BinID&,int) const;
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

			SEGYDirectSeisTrcTranslator(const char*,const char*);
			~SEGYDirectSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgydef"; }

    virtual bool	readInfo(SeisTrcInfo&);
    virtual bool	read(SeisTrc&);
    virtual bool	skip(int);
    virtual bool	supportsGoTo() const		{ return true; }
    virtual bool	isUserSelectable(bool) const	{ return true; }
    virtual Conn*	curConn();
    virtual BinID	curBinID() const;
    virtual bool	forRead() const			{ return forread_; }

    virtual void	toSupported(DataCharacteristics&) const;
    virtual bool	write(const SeisTrc&); // no buffer/resort needed

    virtual void	usePar(const IOPar&);

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	close();
    virtual void	cleanUp();
    virtual IOObj*	createWriteIOObj(const IOObjContext&,
					 const MultiID&) const;
    virtual const char*	iconName() const		{ return "segy"; }

    virtual bool	getGeometryInfo(PosInfo::CubeData&) const;
    virtual SEGY::DirectDef* getDef()	{ return def_; }
    virtual bool	goTo(const BinID&);
    virtual uiString	errMsg() const	{ return SeisTrcTranslator::errMsg(); }

    static SEGYSeisTrcTranslator* createTranslator(const SEGY::DirectDef& def,
						   int filenr);
protected:

    bool		commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);

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
    virtual bool	readData(TraceData* externalbuf);

};


