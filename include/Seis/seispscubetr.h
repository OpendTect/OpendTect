#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2004
________________________________________________________________________

-*/

#include "seistrctr.h"
#include "binid.h"
#include "uistrings.h"
class SeisPS3DReader;
namespace PosInfo { class CubeData; }


mExpClass(Seis) SeisPSCubeSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SeisPSCubeSeisTrcTranslator);
  isTranslator(SeisPSCube,SeisTrc)
public:

			SeisPSCubeSeisTrcTranslator(const char*,const char*);
			~SeisPSCubeSeisTrcTranslator();

    bool	readInfo(SeisTrcInfo&) override;
    bool	readData(TraceData*) override;
    bool	read(SeisTrc&) override;
    bool	skip(int) override;
    bool	forRead() const override		{ return true; }

    bool	supportsGoTo() const override		{ return true; }
    bool	goTo(const BinID&) override;
    int		bytesOverheadPerTrace() const override	{ return 52; }

    bool	implRemove(const IOObj*) const override	{ return false; }
    bool	implRename(const IOObj*,const char*,
			   const CallBack*) const override { return false; }
    bool	implSetReadOnly(const IOObj*,bool) const override
							{ return false; }

    const char*	connType() const override;
    bool	isUserSelectable( bool fr ) const override { return fr; }

protected:

    SeisPS3DReader*	psrdr_;
    SeisTrc&		trc_;
    PosInfo::CubeData&	posdata_;
    BinID		curbinid_;
    bool		inforead_;
    TypeSet<int>	trcnrs_;

    bool	initRead_() override;
    bool	initWrite_(const SeisTrc&) override
			{ errmsg_ = mINTERNAL("PS Cube is RO" ); return false; }
    bool	commitSelections_() override;

    bool	doRead(SeisTrc&,TypeSet<float>* offss=0);
    bool	toNext();

};
