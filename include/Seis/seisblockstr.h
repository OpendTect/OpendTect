#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2017
________________________________________________________________________

-*/

#include "seistrctr.h"

namespace Seis { namespace Blocks { class Reader; class Writer; } }


mExpClass(Seis) BlocksSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(BlocksSeisTrcTranslator);
  isTranslator(Blocks,SeisTrc)

public:

  mUseType( Seis::Blocks,	Reader );
  mUseType( Seis::Blocks,	Writer );

			BlocksSeisTrcTranslator(const char*,const char*);
			~BlocksSeisTrcTranslator();

    const char*	defExtension() const override;
    const char*	iconName() const override		{ return "blockscube"; }
    bool	forRead() const override		{ return !wrr_; }

    bool	readInfo(SeisTrcInfo&) override;
    bool	read(SeisTrc&) override;
    bool	readData(TraceData*) override;
    bool	skip(int) override;
    bool	supportsGoTo() const override		{ return true; }
    bool	goTo(const BinID&) override;
    bool	isUserSelectable(bool) const override	{ return true; }
    bool	getGeometryInfo(LineCollData&) const override;

    void	usePar(const IOPar&) override;

    bool	close() override;
    void	cleanUp() override;
    void	convToConnExpr(BufferString&) const;

    int		bytesOverheadPerTrace() const override	{ return 0; }
    bool	isSingleComponent() const override	{ return false; }
    int		estimatedNrTraces() const override;

    static const char*	sKeyTrName()			{ return "Blocks"; }

    virtual BufferStringSet auxExtensions() const;

protected:

    Reader*		rdr_;
    Writer*		wrr_;
    OD::DataRepType	preseldatarep_;

    bool	commitSelections_() override;
    bool	initRead_() override;
    bool	initWrite_(const SeisTrc&) override;
    bool	writeTrc_(const SeisTrc&) override;
    bool	wantBuffering() const override		{ return false; }

};
