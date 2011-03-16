#ifndef segydirecttr_h
#define segydirecttr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2008
 RCS:		$Id: segydirecttr.h,v 1.10 2011-03-16 16:17:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispsioprov.h"
#include "seistrctr.h"
namespace SEGY { class DirectDef; }
namespace PosInfo { class CubeData; }
class SEGYSeisTrcTranslator;


/*!\brief reads from 3D PS data store reader based on SEG-Y files */

mClass SEGYDirect3DPSReader : public ::SeisPS3DReader
{
public:

    			SEGYDirect3DPSReader(const char* fnm);
			// Check errMsg() to see failure
			~SEGYDirect3DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.str(); }

    const PosInfo::CubeData& posData() const;

protected:

    SEGY::DirectDef&	def_;
    mutable SEGYSeisTrcTranslator* tr_;
    mutable int		curfilenr_;
    mutable BufferString errmsg_;

    SeisTrc*		getTrace(int,int,int,const BinID&) const;

};


/*!\brief reads from 2D PS data store reader based on SEG-Y files */

mClass SEGYDirect2DPSReader : public SeisPS2DReader
{
public:

    			SEGYDirect2DPSReader(const char* dirnm,const char* lnm);
			// Check errMsg() to see failure
			~SEGYDirect2DPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.str(); }

    const PosInfo::Line2DData& posData() const;

protected:

    SEGY::DirectDef&		def_;
    mutable SEGYSeisTrcTranslator* tr_;
    mutable int			curfilenr_;
    mutable BufferString	errmsg_;

    SeisTrc*			getTrace(int,int,int,int) const;
};



mClass SEGYDirectSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(SEGYDirect,SeisPS3D)
public:
    			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS3D)

    virtual bool	isReadDefault() const	{ return true; }
    virtual const char*	defExtension() const	{ return fileExt(); }
    static const char*	fileExt()		{ return "sgydef"; }

};


mClass SEGYDirectSeisPS2DTranslator : public SeisPS2DTranslator
{			       isTranslator(SEGYDirect,SeisPS2D)
public:
    			mDefEmptyTranslatorConstructor(SEGYDirect,SeisPS2D)

    virtual bool	isReadDefault() const	{ return true; }

};


mClass SEGYDirectSeisTrcTranslator : public SeisTrcTranslator
{			      isTranslator(SEGYDirect,SeisTrc)
public:

			SEGYDirectSeisTrcTranslator(const char*,const char*);
			~SEGYDirectSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgydef"; }

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int);
    bool		supportsGoTo() const		{ return true; }
    bool		isReadDefault() const		{ return true; }
    bool		goTo(const BinID&);
    BinID		curBinID() const;

    void		usePar(const IOPar&);

    bool		implShouldRemove(const IOObj*) const { return false; }
    void		cleanUp();

protected:

    bool		commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&)
			{ errmsg = "Writing not supported"; return false; }
    virtual bool	writeTrc_(const SeisTrc&) { return false; }

    SEGY::DirectDef*	def_;
    mutable SEGYSeisTrcTranslator* tr_;
    bool		headerread_;
    int			curfilenr_;
    int			ild_;
    int			iseg_;
    int			itrc_;

    void		initVars();
    const PosInfo::CubeData& cubeData() const;
    bool		toNextTrace();

};


#endif
