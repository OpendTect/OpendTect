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
#include "position.h"
class IOObj;
class SeisPS3DReader;
namespace PosInfo { class CubeData; }


mClass SeisPSCubeSeisTrcTranslator : public SeisTrcTranslator
{				    isTranslator(SeisPSCube,SeisTrc)
public:

    			SeisPSCubeSeisTrcTranslator(const char*,const char*);
			~SeisPSCubeSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    virtual int		bytesOverheadPerTrace() const	{ return 52; }

    virtual bool	implRemove(const IOObj*) const	{ return false; }
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack*) const { return false; }
    virtual bool	implSetReadOnly(const IOObj*,bool) const
							{ return false; }

    const char*		connType() const;
    virtual bool	isReadDefault() const           { return true; }

protected:

    SeisPS3DReader*	psrdr_;
    SeisTrc&		trc_;
    PosInfo::CubeData&	posdata_;
    BinID		curbinid_;
    bool		inforead_;

    bool		initRead_();
    bool		initWrite_(const SeisTrc&)
    			{ errmsg = "Cannot write to PS Cube"; return false; }
    bool		commitSelections_();

    bool		doRead(SeisTrc&,TypeSet<float>* offss=0);
    bool		toNext();

    TypeSet<int>	trcnrs_;

};


#endif
