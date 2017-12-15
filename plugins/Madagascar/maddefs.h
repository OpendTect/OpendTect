#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

#include "madagascarmod.h"
#include "bufstringset.h"
#include "uistring.h"

// If you change this def, there is a copy (almost same name) in madagcattrib.h
#define mMadagascarLinkPackage "Madagascar Link"

class Executor;

namespace ODMad
{

mClass(Madagascar) ProgDef
{ mODTextTranslationClass(ProgDef);
public:

				ProgDef() : group_(0) {}
    BufferString		name_;
    BufferString		shortdesc_;
    BufferString		synopsis_;
    BufferString		comment_;
    const BufferString*		group_; //!< Never null

};


/* Scans $RSFROOT/doc/txt directory for program definitions */

mExpClass(Madagascar) ProgInfo
{ mODTextTranslationClass(ProgInfo);
public:
			//!< When PI() is first used, a Pre-Scan is done
			//!< if Pre-Scan check is OK, err msg will be empty

    bool		scanned() const		{ return scanned_; }
    Executor*		getScanner() const;
			//!< If scan fails, err msg will be filled

    const uiString&	errMsg() const		{ return errmsg_; }

    const ObjectSet<ProgDef>&	defs() const	{ return defs_; }
    const BufferStringSet&	groups() const	{ return groups_; }
    void			search(const char* globexpr,
				       ObjectSet<const ProgDef>&) const;
				//!< name_, shortdesc_, comment_

			ProgInfo();
			~ProgInfo()		{ cleanUp(); }

protected:

    BufferString	rsfroot_;
    BufferString	defdir_;
    uiString		errmsg_;
    bool		scanned_;
    BufferStringSet	groups_;
    ObjectSet<ProgDef>	defs_;

    void		cleanUp();
    void		doPreScanCheck();
    void		addEntry(const char*);

    friend class	ProgInfoScanner;
    mGlobal(Madagascar) friend	ProgInfo& PI();
};

mGlobal(Madagascar) ProgInfo& PI();



} // namespace ODMad
