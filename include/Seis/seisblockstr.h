#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2017
________________________________________________________________________

-*/

#include "seistrctr.h"
#include "datachar.h"

namespace Seis { namespace Blocks { class Reader; } }


mExpClass(Seis) BlocksSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(BlocksSeisTrcTranslator);
  isTranslator(Blocks,SeisTrc)

public:

    typedef Seis::Blocks::Reader		Reader;
    typedef DataCharacteristics::UserType	FPDataRepType;

			BlocksSeisTrcTranslator(const char*,const char*);
			~BlocksSeisTrcTranslator();
    const char*		defExtension() const override	{ return "blocks"; }
    bool		forRead() const override	{ return true; }

    bool		readInfo(SeisTrcInfo&) override;
    bool		read(SeisTrc&) override;
    bool		skip(int) override;
    bool		supportsGoTo() const override	{ return true; }
    bool		goTo(const BinID&) override;
    bool		isUserSelectable(bool forread) const override
			{ return forread; }
    bool		getGeometryInfo(PosInfo::CubeData&) const override;

    void		usePar(const IOPar&) override;

    bool		close() override;
    void		cleanUp() override;
    const char*		iconName() const override	{ return "blockscube"; }
    virtual void	convToConnExpr(BufferString&) const;

    int			bytesOverheadPerTrace() const override	{ return 0; }
    virtual bool	isSingleComponent() const	{ return false; }
    int			estimatedNrTraces() const override;

    static const char*	sKeyTrName()			{ return "Blocks"; }

    virtual BufferStringSet auxExtensions() const;

protected:

    Reader*		rdr_;
    FPDataRepType	preselfprep_;

    bool		commitSelections_() override;
    bool		initRead_() override;
    bool		initWrite_(const SeisTrc&) override;
    bool		writeTrc_(const SeisTrc&) override;
    virtual bool	wantBuffering() const		{ return false; }

};
