#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
________________________________________________________________________

-*/

#include "seismod.h"
#include "seistrctr.h"
#include "binid.h"
class IOObj;
class SeisPS3DReader;
namespace PosInfo { class CubeData; }


mExpClass(Seis) SeisPSCubeSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SeisPSCubeSeisTrcTranslator);
  isTranslator(SeisPSCube,SeisTrc)
public:

			SeisPSCubeSeisTrcTranslator(const char*,const char*);
			~SeisPSCubeSeisTrcTranslator();

    bool	readInfo(SeisTrcInfo&) override;
    bool	read(SeisTrc&) override;
    bool	skip(int) override;
    bool	forRead() const override		{ return true; }

    bool	supportsGoTo() const override		{ return true; }
    bool	goTo(const BinID&) override;
    int		bytesOverheadPerTrace() const override	{ return 52; }

    bool	implIsLink(const IOObj*) const override { return true; }
    bool	implRemove(const IOObj*,bool) const override { return false; }
    bool	implRename(const IOObj*,const char*) const override
							{ return false; }
    bool	implSetReadOnly(const IOObj*,bool) const override
							{ return false; }

    const char* connType() const override;
    bool	isUserSelectable( bool fr ) const override { return fr; }

protected:

    SeisPS3DReader*	psrdr_;
    SeisTrc&		trc_;
    PosInfo::CubeData&	posdata_;
    BinID		curbinid_;
    bool		inforead_;

    bool		initRead_() override;
    bool		initWrite_(const SeisTrc&) override
			{ errmsg_ = tr( "No write to PS Cube" ); return false; }
    bool		commitSelections_() override;

    bool		doRead(SeisTrc&,TypeSet<float>* offss=0);
    bool		toNext();

    TypeSet<int>	trcnrs_;

};


