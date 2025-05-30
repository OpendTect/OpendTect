#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "executor.h"
#include "multiid.h"
#include "ptrman.h"

class IOObj;

namespace Well {

// brief class to copy well data from one well to a new well.

mClass(Well) Copier
{ mODTextTranslationClass(Copier)
public:

			Copier(const MultiID&,const char* outwellnm);
			~Copier();

    void		setOverwriteAllowed(bool);
    bool		doCopy();
    uiString		errMsg() const			{ return errmsg_; }
    MultiID		copiedWellID() const		{ return outputid_; }

private:

    PtrMan<IOObj>	getOutputIOObj();

    bool		allowoverwrite_			= false;
    MultiID		outputid_;
    const MultiID	inpid_;
    const BufferString	outwellname_;
    mutable uiString	errmsg_;
};
} // namespace well


mExpClass(Well) MultiWellCopier : public Executor
{mODTextTranslationClass(MultiWellCopier)
public:
			MultiWellCopier(
			    const ObjectSet<std::pair<const MultiID&,
					    const BufferString>>& ipidoutnmset);
			~MultiWellCopier();

    void		setOverwriteAllowed(bool);

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    const TypeSet<MultiID>&	copiedWellIDs() const	{ return outputids_; }

private:

    od_int64		nrwells_;
    od_int64		nrdone_				    = 0;
    mutable uiString	errmsg_;
    bool		allowoverwrite_			    = false;

    const ObjectSet<std::pair<const MultiID&,const BufferString>>&  copyset_;
    TypeSet<MultiID>	outputids_;

    int			nextStep() override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    bool		inputIsOK(const MultiID&,const char*) const;

};
