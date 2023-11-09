#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
    bool		readDataPack(RegularSeisDataPack&,TaskRunner*) override;
    bool		skip(int nrtrcs=1) override;

    bool		supportsGoTo() const override		{ return true; }
    bool		goTo(const BinID&) override;
    bool		toStart();
    int			bytesOverheadPerTrace() const override;
    int			estimatedNrTraces() const override;
    bool		forRead() const override;

    void		usePar(const IOPar&) override;

    const CBVSReadMgr*	readMgr() const			{ return rdmgr_; }
    Pos::IdxPair2Coord	getTransform() const;

    bool		getGeometryInfo(PosInfo::CubeData&) const override;

    bool		implIsLink(const IOObj*) const override;
    bool		implRemove(const IOObj*,bool) const override;
    bool		implRename(const IOObj*,const char*) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;
    bool		getConfirmRemoveMsg(const IOObj*,uiString& msg,
					    uiString& canceltxt,
					    uiString& yestxt,
					    uiString& notxt) const override;
    const char*		defExtension() const override
			{ return sKeyDefExtension(); }

    static const char*	sKeyDefExtension();

    bool		is2D() const;
    void		set2D(bool yn=true);
    void		setCurGeomID(Pos::GeomID) override;
    bool		singleFile() const		{ return single_file_; }
    void		setSingleFile( bool yn=true )	{ single_file_ = yn; }
    void		setForceUseCBVSInfo(bool yn)	{ forceusecbvsinfo_=yn;}

    void		setCoordPol(bool dowrite,bool intrailer);
    void		setPreselDataType( int dt )	{ preseldatatype_ = dt;}

    bool		isUserSelectable(bool) const override	{ return true; }

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

    void		cleanUp() override;
    bool		initRead_() override;
    bool		initWrite_(const SeisTrc&) override;
    bool		commitSelections_() override;
    bool		writeTrc_(const SeisTrc&) override;
    void		blockDumped(int) override;
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);
    bool		inactiveSelData() const;
    int			selRes(const BinID&) const;

private:

    static const IOPar& datatypeparspec;

    void		destroyVars(int);
    bool		readData(TraceData* externalbuf) override;

};
