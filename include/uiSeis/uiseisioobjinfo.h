#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisioobjinfo.h"


mExpClass(uiSeis) uiSeisIOObjInfo
{ mODTextTranslationClass(uiSeisIOObjInfo);
public:

			uiSeisIOObjInfo(const IOObj&,bool error_feedback=true);
			uiSeisIOObjInfo(const MultiID&,bool err_feedback=true);
			~uiSeisIOObjInfo();

    bool		isOK() const		{ return sii_.isOK(); }
    bool		is2D() const		{ return sii_.is2D(); }
    bool		isPS() const		{ return sii_.isPS(); }
    bool		isTime() const		{ return sii_.isTime(); }
    bool		isDepth() const		{ return sii_.isDepth(); }
    const ZDomain::Def& zDomainDef() const	{ return sii_.zDomainDef(); }
    const ZDomain::Info& zDomain() const	{ return sii_.zDomain(); }

    bool		provideUserInfo() const;
    bool		provideUserInfo2D(
				const TypeSet<Pos::GeomID>* sel=0) const;
			// By default (sel=0) gives info for all lines.

    bool		checkSpaceLeft(const SeisIOObjInfo::SpaceInfo&) const;

    int			expectedMBs( const SeisIOObjInfo::SpaceInfo& s ) const
					{ return sii_.expectedMBs(s); }
    bool		getRanges( TrcKeyZSampling& cs ) const
					{ return sii_.getRanges( cs ); }
    bool		getBPS( int& b, int icmp=-1 ) const
					{ return sii_.getBPS(b,icmp); }

    static const char*	sKeyEstMBs;

    const SeisIOObjInfo& ioObjInfo() const	{ return sii_; }
    const IOObj*	ioObj() const		{ return sii_.ioObj(); }

protected:

    SeisIOObjInfo	sii_;
    bool		doerrs_;

};
