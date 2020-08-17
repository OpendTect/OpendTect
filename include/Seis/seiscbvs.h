#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id$
________________________________________________________________________

CBVS-based seimic translator.

-*/

#include "seismod.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "cbvsinfo.h"
class CBVSReadMgr;
class CBVSWriteMgr;
class VBrickSpec;


mExpClass(Seis) CBVSSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(CBVSSeisTrcTranslator); isTranslator(CBVS,SeisTrc)
public:

			CBVSSeisTrcTranslator(const char*,const char*);
    static CBVSSeisTrcTranslator* make(const char* fnm,bool forinfoonly,
					bool is2d, uiString* errmsg=0,
					bool forceusecbvsinfo=false);
			~CBVSSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int nrtrcs=1);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    bool		toStart();
    virtual int		bytesOverheadPerTrace() const;
    virtual bool	forRead() const;

    virtual void	usePar(const IOPar&);

    const CBVSReadMgr*	readMgr() const			{ return rdmgr_; }
    Pos::IdxPair2Coord	getTransform() const;

    virtual bool	getGeometryInfo(PosInfo::CubeData&) const;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
				   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;
    const char*	defExtension() const	{ return sKeyDefExtension(); }
    static const char*	sKeyDefExtension();

    bool		is2D() const			{ return is2d_; }
    void		set2D(bool yn=true);
    bool		singleFile() const		{ return single_file_; }
    void		setSingleFile( bool yn=true )	{ single_file_ = yn; }
    void		setForceUseCBVSInfo(bool yn)	{ forceusecbvsinfo_=yn;}

    void		setCoordPol(bool dowrite,bool intrailer);
    void		setPreselDataType( int dt )	{ preseldatatype_ = dt;}

    bool		isUserSelectable(bool) const	{ return true; }

    static const char*	sKeyOptDir()		{ return "Optimized direction";}

protected:

    bool		forread_;
    bool		headerdone_; // Will be in base class after 6.2
    bool		donext_;
    int			nrdone_;
    int			coordpol_;

    // Following variables are inited by commitSelections_
    bool*		compsel_;
    unsigned char**	blockbufs_; // Will be removed after 6.2
    TraceDataInterpreter** storinterps_; // Will be removed after 6.2
    int			preseldatatype_;
    VBrickSpec&		brickspec_;

    CBVSReadMgr*	rdmgr_;
    CBVSWriteMgr*	wrmgr_;
    PosAuxInfo		auxinf_;
    bool		is2d_;
    bool		single_file_;
    bool		forceusecbvsinfo_;

    virtual void	cleanUp();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	commitSelections_();
    virtual bool	writeTrc_(const SeisTrc&);
    virtual void	blockDumped(int);
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);
    bool		inactiveSelData() const;
    int			selRes(const BinID&) const;

private:

    static const IOPar&	datatypeparspec;

    void		destroyVars(int);
    virtual bool	readData(TraceData* externalbuf);

};


