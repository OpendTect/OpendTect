#ifndef seispscubetr_h
#define seispscubetr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id$
________________________________________________________________________

-*/

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

    virtual bool	readInfo(SeisTrcInfo&);
    virtual bool	read(SeisTrc&);
    virtual bool	skip(int);
    virtual bool	forRead() const			{ return true; }

    virtual bool	supportsGoTo() const		{ return true; }
    virtual bool	goTo(const BinID&);
    virtual int		bytesOverheadPerTrace() const	{ return 52; }

    virtual bool	implRemove(const IOObj*) const	{ return false; }
    virtual bool	implRename(const IOObj*,const char*,
				   const CallBack*) const { return false; }
    virtual bool	implSetReadOnly(const IOObj*,bool) const
							{ return false; }

    virtual const char*	connType() const;
    virtual bool	isUserSelectable( bool fr ) const { return fr; }

protected:

    SeisPS3DReader*	psrdr_;
    SeisTrc&		trc_;
    PosInfo::CubeData&	posdata_;
    BinID		curbinid_;
    bool		inforead_;

    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&)
			{ errmsg_ = tr( "No write to PS Cube" ); return false; }
    virtual bool	commitSelections_();

    bool		doRead(SeisTrc&,TypeSet<float>* offss=0);
    bool		toNext();

    TypeSet<int>	trcnrs_;

};


#endif
