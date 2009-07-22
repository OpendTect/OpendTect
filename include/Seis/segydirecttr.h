#ifndef segydirecttr_h
#define segydirecttr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2008
 RCS:		$Id: segydirecttr.h,v 1.6 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispsioprov.h"
namespace SEGY { class DirectDef; }
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
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::CubeData& posData() const	{ return posdata_; }

protected:

    PosInfo::CubeData&	posdata_;
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
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::Line2DData& posData() const	{ return posdata_; }

protected:

    PosInfo::Line2DData&	posdata_;
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


#endif
