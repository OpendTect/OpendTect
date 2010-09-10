#ifndef emhorizonpreload_h
#define emhorizonpreload_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: emhorizonpreload.h,v 1.4 2010-09-10 06:44:52 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "callback.h"

#include "bufstringset.h"
#include "typeset.h"

class MultiID;
class TaskRunner;

namespace EM
{

mClass HorizonPreLoad : public CallBacker
{
public:
				HorizonPreLoad();

    bool			load(const TypeSet<MultiID>&,TaskRunner* tr=0);
    const MultiID&		getMultiID(const char* name) const;
    const BufferStringSet&	getPreloadedNames() const   { return nameset_; }
    const char*			errorMsg() const	    { return errmsg_; }
    bool			unload(const BufferStringSet& hornms);

protected:
    void		surveyChgCB(CallBacker*);

    TypeSet<MultiID>	midset_;
    BufferStringSet	nameset_;
    BufferString	errmsg_;
};


mGlobal HorizonPreLoad& HPreL();

} //namespace

#endif
