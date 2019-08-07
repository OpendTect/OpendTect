#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2001
________________________________________________________________________

CBVS-based seimic translator.

-*/

#include "seistrctr.h"
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
#ifndef __od6like__
    const char*	iconName() const override	{ return "od6"; }
#endif
    const char*	defExtension() const override	{ return sKeyDefExtension(); }

    bool	readInfo(SeisTrcInfo&) override;
    bool	readData(TraceData* externalbuf) override;
    bool	skip(int nrtrcs=1) override;

    bool	supportsGoTo() const override		{ return true; }
    bool	goTo(const BinID&) override;
    bool	toStart();
    bool	isSingleComponent() const override	{ return false; }
    int		bytesOverheadPerTrace() const override;
    bool	forRead() const override;

    void	usePar(const IOPar&) override;

    const CBVSReadMgr*	readMgr() const			{ return rdmgr_; }
    Pos::IdxPair2Coord	getTransform() const;

    bool	getGeometryInfo(LineCollData&) const override;

    bool	implRemove(const IOObj*) const override;
    bool	implRename(const IOObj*,const char*,
				   const CallBack* cb=0) const override;
    bool	implSetReadOnly(const IOObj*,bool) const override;
    static const char*	sKeyDefExtension();

    bool	is2D() const			{ return is2d_; }
    void	set2D(bool yn=true);
    bool	singleFile() const		{ return single_file_; }
    void	setSingleFile( bool yn=true )	{ single_file_ = yn; }
    void	setForceUseCBVSInfo(bool yn)	{ forceusecbvsinfo_=yn;}

    void	setCoordPol(bool dowrite,bool intrailer);
    void	setDataRep( OD::DataRepType t )	{ datarep_ = t; }

    bool	isUserSelectable(bool) const	{ return true; }

    static const char*	sKeyOptDir()		{ return "Optimized direction";}

protected:

    bool		forread_;
    bool		donext_;
    int			nrdone_;
    int			coordpol_;

    // Following variables are inited by commitSelections_
    bool*		compsel_;
    OD::DataRepType	datarep_;
    VBrickSpec&		brickspec_;

    CBVSReadMgr*	rdmgr_;
    CBVSWriteMgr*	wrmgr_;
    PosAuxInfo		auxinf_;
    bool		is2d_;
    bool		single_file_;
    bool		forceusecbvsinfo_;
    static const IOPar&	datatypeparspec;

    void	cleanUp() override;
    bool	initRead_() override;
    bool	initWrite_(const SeisTrc&) override;
    bool	commitSelections_() override;
    bool	writeTrc_(const SeisTrc&) override;
    void	blockDumped(int) override;
    bool	startWrite();
    bool	toNext();
    bool	getFileName(BufferString&);
    bool	inactiveSelData() const;
    int		selRes(const BinID&) const;
    BinID	curMgrBinID() const;
    void	updBinIDFromMgr(BinID&) const;

private:

    void	destroyVars();

};
