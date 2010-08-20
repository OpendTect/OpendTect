#ifndef emhorizonpreload_h
#define emhorizonpreload_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: emhorizonpreload.h,v 1.1 2010-08-20 11:23:26 cvsnageswara Exp $
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
    bool		loadHorizon(const MultiID&,TaskRunner* tr=0);
    const BufferString	name(const MultiID&) const;
    const MultiID	getMultiID( const BufferString& );
    BufferStringSet	getHorizonNames() const	{ return nameset_; }
    BufferString	errorMsg() const	{ return errmsg_; }
    bool		unloadHorizon(const BufferString&);

protected:
    void		surveyChgCB(CallBacker*);
    TypeSet<MultiID>	midset_;
    BufferStringSet	nameset_;
    BufferString	errmsg_;
};


mGlobal HorizonPreLoad& HPreL();

} //namespace
#endif
