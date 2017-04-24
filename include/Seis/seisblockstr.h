#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2017
________________________________________________________________________

-*/

#include "seistrctr.h"

namespace Seis { namespace Blocks { class Reader; class Writer; }


mExpClass(Seis) SeisBlocksSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SeisBlocksSeisTrcTranslator);
  isTranslator(SeisBlocks,SeisTrc)

public:

      typedef Seis::Blocks::Reader  Reader;
      typedef Seis::Blocks::Writer  Writer;

			SeisBlocksSeisTrcTranslator(const char*,const char*);
			~SeisBlocksSeisTrcTranslator();
    virtual const char*	defExtension() const		{ return "cube"; }
    virtual bool	forRead() const			{ return !wrr_; }

    virtual bool	readInfo(SeisTrcInfo&);
    virtual bool	read(SeisTrc&);
    virtual bool	skip(int);
    virtual bool	supportsGoTo() const		{ return true; }
    virtual bool	goTo(const BinID&);
    virtual bool	isUserSelectable(bool) const	{ return true; }
    virtual bool	getGeometryInfo(PosInfo::CubeData&) const;

    virtual bool	write(const SeisTrc&);

    virtual void	usePar(const IOPar&);

    virtual bool	close();
    virtual void	cleanUp();
    virtual const char*	iconName() const		{ return "blockscube"; }

    virtual int		bytesOverheadPerTrace() const	{ return 0; }
    virtual bool	isSingleComponent() const	{ return false; }
    virtual int		estimatedNrTraces() const;

protected:

    Reader*		rdr_;
    Writer*		wrr_;
    Seis::Blocks::IOClass*  ioclss_;

    virtual bool	commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);

};
