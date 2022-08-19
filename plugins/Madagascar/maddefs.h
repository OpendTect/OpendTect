#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarmod.h"
#include "bufstringset.h"
#include "uistring.h"

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
