#ifndef segydirecttr_h
#define segydirecttr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seispsread.h"
#include "seispsioprov.h"
#include "seistrctr.h"
namespace SEGY { class DirectDef; }
namespace PosInfo { class CubeData; }
class SEGYSeisTrcTranslator;


/*!\brief Base class for readers from SEG-Y file sets */

namespace SEGY
{

mExpClass(Seis) DirectReader
{
public:

    					DirectReader()
					    : tr_(0), curfilenr_(-1)	{}
    virtual				~DirectReader();

    virtual DirectDef*			getDef()		= 0;
    virtual const char*			errMsg() const		= 0;
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
{
public:

    			SEGYDirect3DPSReader(const char* fnm);
			// Check errMsg() to see failure
			~SEGYDirect3DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.str(); }

    const PosInfo::CubeData& posData() const;

    virtual SEGY::DirectDef*	getDef()	{ return &def_; }
    virtual bool		goTo(const BinID&);

protected:

    SEGY::DirectDef&	def_;
    mutable BufferString errmsg_;

    SeisTrc*		getTrace(int,int,const BinID&) const;
    bool		goTo(int,int) const;

};


/*!\brief reads from 2D PS data store reader based on SEG-Y files */

mExpClass(Seis) SEGYDirect2DPSReader : public SeisPS2DReader
			    , public SEGY::DirectReader
{
public:

    			SEGYDirect2DPSReader(const char* dirnm,const char* lnm);
			// Check errMsg() to see failure
			~SEGYDirect2DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.str(); }

    const PosInfo::Line2DData& posData() const;

    virtual SEGY::DirectDef*	getDef()	{ return &def_; }
    virtual bool		goTo(const BinID&);

protected:

    SEGY::DirectDef&		def_;
    mutable BufferString	errmsg_;

    SeisTrc*			getTrace(int,int,int) const;
    bool			goTo(int,int) const;
};



mExpClass(Seis) SEGYDirectSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(SEGYDirect,SeisPS3D)
public:
    			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS3D)

    virtual bool	isReadDefault() const	{ return true; }
    virtual const char*	defExtension() const	{ return fileExt(); }
    static const char*	fileExt()		{ return "sgydef"; }

};


mExpClass(Seis) SEGYDirectSeisPS2DTranslator : public SeisPS2DTranslator
{			       isTranslator(SEGYDirect,SeisPS2D)
public:
    			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS2D)

    virtual bool	isReadDefault() const	{ return true; }

};


mExpClass(Seis) SEGYDirectSeisTrcTranslator : public SeisTrcTranslator
				   , public SEGY::DirectReader
{			isTranslator(SEGYDirect,SeisTrc)
public:

			SEGYDirectSeisTrcTranslator(const char*,const char*);
			~SEGYDirectSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgydef"; }

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int);
    bool		supportsGoTo() const		{ return true; }
    bool		isReadDefault() const		{ return true; }
    BinID		curBinID() const;

    void		usePar(const IOPar&);

    bool		implShouldRemove(const IOObj*) const { return false; }
    void		cleanUp();

    virtual SEGY::DirectDef* getDef()	{ return def_; }
    virtual bool	goTo(const BinID&);
    const char*		errMsg() const	{ return SeisTrcTranslator::errMsg(); }

protected:

    bool		commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&)
			{ errmsg = "Writing not supported"; return false; }
    virtual bool	writeTrc_(const SeisTrc&) { return false; }

    SEGY::DirectDef*	def_;
    bool		headerread_;
    int			ild_;
    int			iseg_;
    int			itrc_;

    void		initVars();
    const PosInfo::CubeData& cubeData() const;
    bool		toNextTrace();
    bool		positionTranslator();

};


#endif

