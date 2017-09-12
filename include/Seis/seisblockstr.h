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
    virtual const char*	defExtension() const		{ return "blocks"; }
    virtual bool	forRead() const			{ return true; }

    virtual bool	readInfo(SeisTrcInfo&);
    virtual bool	read(SeisTrc&);
    virtual bool	skip(int);
    virtual bool	supportsGoTo() const		{ return true; }
    virtual bool	goTo(const BinID&);
    virtual bool	isUserSelectable(bool forread) const	{ return forread; }
    virtual bool	getGeometryInfo(PosInfo::CubeData&) const;

    virtual void	usePar(const IOPar&);

    virtual bool	close();
    virtual void	cleanUp();
    virtual const char*	iconName() const		{ return "blockscube"; }
    virtual void	convToConnExpr(BufferString&) const;

    virtual int		bytesOverheadPerTrace() const	{ return 0; }
    virtual bool	isSingleComponent() const	{ return false; }
    virtual int		estimatedNrTraces() const;

    static const char*	sKeyTrName()			{ return "Blocks"; }

    virtual BufferStringSet auxExtensions() const;

protected:

    Reader*		rdr_;
    FPDataRepType	preselfprep_;

    virtual bool	commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	writeTrc_(const SeisTrc&);
    virtual bool	wantBuffering() const		{ return false; }

};
