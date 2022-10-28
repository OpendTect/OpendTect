/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobj.h"

#include "compoundkey.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "iodir.h"
#include "ioman.h"
#include "iostrm.h"
#include "survinfo.h"
#include "transl.h"

#include "uimsg.h"
#include "uistrings.h"



uiIOObj::uiIOObj( IOObj& i, bool silent )
    : ioobj_(i)
    , silent_(silent)
{}


uiIOObj::~uiIOObj()
{}


bool uiIOObj::removeImpl( bool rmentry, bool mustrm, bool doconfirm )
{
    PtrMan<Translator> trans = ioobj_.createTranslator();
    if ( !trans )
	return false;

    uiString msg, canceltxt, deepremovetext, shallowremovetxt;
    if ( trans->getConfirmRemoveMsg(&ioobj_,msg,canceltxt,deepremovetext,
				    shallowremovetxt) )
    {
	bool deepremove = true;
	if ( shallowremovetxt.isEmpty() )
	{
	    if ( !uiMSG().askRemove(msg) )
		return false;
	}
	else
	{
	    //TODO: Pop up a dialog with radio buttons for deep & shallow texts
	    const int resp = uiMSG().askGoOnAfter( msg, canceltxt,
					deepremovetext, shallowremovetxt );
	    if ( resp < 0 )
		return false;

	    deepremove = resp == 1;
	}

	if ( !IOM().implRemove(ioobj_,deepremove) )
	{
	    if ( !silent_ )
	    {
		uiString mess = tr("Could not delete data file(s).\n"
				   "Remove entry from list anyway?");
		if ( !uiMSG().askRemove(mess) )
		    return false;
	    }

	    return false;
	}
    }

    if ( rmentry )
    {
	const bool removed = IOM().permRemove( ioobj_.key() );
	if ( removed )
	{
	    if ( IOObj::isSurveyDefault(ioobj_.key()) )
	    {
		PtrMan<Translator> trl = ioobj_.createTranslator();
		CompoundKey defaultkey(
				   trl->group()->getSurveyDefaultKey(&ioobj_) );
		SI().getPars().removeWithKey( defaultkey.buf() );
		SI().savePars();
	    }
	}
    }

    return true;
}


bool uiIOObj::fillCtio( CtxtIOObj& ctio, bool warnifexist )
{
    if ( ctio.name().isEmpty() )
    {
	if ( !ctio.ioobj_ )
	    return false;
	ctio.setName( ctio.ioobj_->name() );
    }
    const char* nm = ctio.name().buf();

    const IODir iodir( ctio.ctxt_.getSelKey() );
    const IOObj* existioobj = iodir.get( nm, ctio.ctxt_.trgroup_->groupName() );
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	IOM().getEntry( ctio );
	return ctio.ioobj_;
    }

    if ( warnifexist )
    {
	uiString msg = tr("Overwrite existing '%1'?").arg(nm);
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
    }

    ctio.setObj( existioobj->clone() );
    return true;
}
