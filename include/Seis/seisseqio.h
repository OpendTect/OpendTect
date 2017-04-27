#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

#include "seiscommon.h"
#include "bufstring.h"
#include "factory.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;
class SeisPSReader;
class SeisTrcWriter;
class BufferStringSet;
namespace Seis		{ class SelData; }

namespace Seis
{

class Bounds;

/*!\brief Base class for Seismic Sequential IO classes */

mExpClass(Seis) SeqIO
{ mODTextTranslationClass(SeqIO);
public:

    virtual const char*	type() const			= 0;
    virtual Seis::GeomType geomType() const		= 0;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)		= 0;

    uiString		errMsg() const			{ return errmsg_; }

    static const char*	sKeyODType;

protected:

    mutable uiString errmsg_;
};


/*!\brief Base class for Seismic Sequential input classes */

mExpClass(Seis) SeqInp : public SeqIO
{ mODTextTranslationClass(SeqInp);
public:

    virtual bool	get(SeisTrc&) const		= 0;

    virtual void	fillPar(IOPar&) const;

    virtual Seis::Bounds* getBounds() const		{ return 0; }
    virtual int		estimateTotalNumber() const	{ return -1; }

    mDefineFactoryInClass(SeqInp,factory);

protected:

    static Seis::GeomType getGeomType(const IOPar&);

};


/*!\brief Base class for Seismic Sequential output classes */

mExpClass(Seis) SeqOut : public SeqIO
{ mODTextTranslationClass(SeqOut);
public:

    virtual bool	put(const SeisTrc&)		= 0;

    static BufferStringSet& classNames();
    static SeqOut*	make(const char*);
    static void		addClass(SeqOut*);

    mDefineFactoryInClass(SeqOut,factory);

};


} // namespace Seis
