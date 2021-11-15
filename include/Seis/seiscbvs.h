#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
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

    bool		readInfo(SeisTrcInfo&) override;
    bool		skip(int nrtrcs=1);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    bool		toStart();
    virtual int		bytesOverheadPerTrace() const;
    int			estimatedNrTraces() const override;
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

    bool		is2D() const;
    void		set2D(bool yn=true);
    void		setCurGeomID(Pos::GeomID) override;
    bool		singleFile() const		{ return single_file_; }
    void		setSingleFile( bool yn=true )	{ single_file_ = yn; }
    void		setForceUseCBVSInfo(bool yn)	{ forceusecbvsinfo_=yn;}

    void		setCoordPol(bool dowrite,bool intrailer);
    void		setPreselDataType( int dt )	{ preseldatatype_ = dt;}

    bool		isUserSelectable(bool) const	{ return true; }

    static const char*	sKeyOptDir()		{ return "Optimized direction";}

protected:

    bool		forread_ = true;
    bool		donext_ = false;
    int			nrdone_ = 0;
    int			coordpol_;

    // Following variables are inited by commitSelections_
    bool*		compsel_ = nullptr;
    int			preseldatatype_ = 0;
    VBrickSpec&		brickspec_;

    CBVSReadMgr*	rdmgr_ = nullptr;
    CBVSWriteMgr*	wrmgr_ = nullptr;
    PosAuxInfo		auxinf_;
    bool		single_file_ = false;
    bool		forceusecbvsinfo_ = false;

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


