#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisioobjinfo.h"


mExpClass(uiSeis) uiSeisIOObjInfo
{ mODTextTranslationClass(uiSeisIOObjInfo);
public:

			uiSeisIOObjInfo(const IOObj&,bool error_feedback=true);
			uiSeisIOObjInfo(const MultiID&,bool err_feedback=true);

    bool		isOK() const		{ return sii.isOK(); }
    bool		is2D() const		{ return sii.is2D(); }
    bool		isPS() const		{ return sii.isPS(); }
    bool		isTime() const		{ return sii.isTime(); }
    bool		isDepth() const		{ return sii.isDepth(); }
    const ZDomain::Def&	zDomainDef() const	{ return sii.zDomainDef(); }

    bool		provideUserInfo() const;
    bool		provideUserInfo2D(
				const TypeSet<Pos::GeomID>* sel=0) const;
			// By default (sel=0) gives info for all lines.

    bool		checkSpaceLeft(const SeisIOObjInfo::SpaceInfo&) const;

    int			expectedMBs( const SeisIOObjInfo::SpaceInfo& s ) const
					{ return sii.expectedMBs(s); }
    bool		getRanges( TrcKeyZSampling& cs ) const
					{ return sii.getRanges( cs ); }
    bool		getBPS( int& b, int icmp=-1 ) const
					{ return sii.getBPS(b,icmp); }

    static const char*	sKeyEstMBs;

    const SeisIOObjInfo& ioObjInfo() const	{ return sii; }
    const IOObj*	ioObj() const		{ return sii.ioObj(); }

protected:

    SeisIOObjInfo	sii;
    bool		doerrs;

};


