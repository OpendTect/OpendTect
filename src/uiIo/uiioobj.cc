/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2006
________________________________________________________________________

-*/


#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "dbman.h"
#include "iostrm.h"
#include "survinfo.h"
#include "transl.h"
#include "uistrings.h"

bool uiIOObj::removeImpl( bool rmentry, bool mustrm, bool doconfirm )
{
    bool dorm = true;
    mDynamicCastGet(IOStream*,iostrm,&ioobj_)
    const bool isuknownioobj = !iostrm && !ioobj_.isSubdir();
    if ( !isuknownioobj && !ioobj_.implManagesObjects() )
    {
	const bool isoutside = !ioobj_.isInCurrentSurvey();
	if ( !silent_ )
	{
	    uiString mess;
	    if ( !ioobj_.isSubdir() )
	    {
		mess = tr("Permanently Delete '%1' %2");
		mess.arg( ioobj_.name() );
		mess.arg( isoutside ? tr("\nFile is not in current survey.\n"
				     "Specify what you would like to delete")
				    : tr("from Database?") );
	    }
	    else
	    {
		mess = tr("Delete '%1' with folder\n'%2' %3");
		mess.arg( ioobj_.name() );
		mess.arg( ioobj_.mainFileName() );
		mess.arg( tr("\n- and everything in it! - ?") );
	    }

	    if ( isoutside )
	    {
		const int resp = gUiMsg().question( mess, tr("Delete file"),
						   tr("Delete link"),
						   uiStrings::sCancel(),
						   tr("Delete data") );
		if ( resp < 0 )
		    return false;

		dorm = resp;
	    }
	    else if ( doconfirm && !gUiMsg().askGoOn(mess,uiStrings::sDelete(),
							 uiStrings::sCancel()) )
	    {
		if ( mustrm )
		    return false;

		dorm = false;
	    }
	}
    }

    if ( dorm && !fullImplRemove(ioobj_) )
    {
	if ( !silent_ )
	{
	    uiString mess = tr("Cannot delete data file(s).\n"
			       "Remove entry from list anyway?");
	    if ( !gUiMsg().askRemove(mess) )
		return false;
	}
    }

    if ( rmentry )
    {
	ioobj_.removeFromDB();
	if ( IOObj::isSurveyDefault(ioobj_.key()) )
	{
	    PtrMan<Translator> trl = ioobj_.createTranslator();
	    const BufferString defaultkey(
			       trl->group()->getSurveyDefaultKey(&ioobj_) );
	    SI().removeKeyFromDefaultPars( defaultkey, true );
	}
    }

    return true;
}


bool uiIOObj::fillCtio( CtxtIOObj& ctio, bool warnifexist )
{
    if ( ctio.getName().isEmpty() )
    {
	if ( !ctio.ioobj_ )
	    return false;
	ctio.setName( ctio.ioobj_->name() );
    }

    const BufferString nm = ctio.getName();

    PtrMan<IOObj> existioobj = DBM().getByName( ctio.ctxt_, nm );
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	DBM().getEntry( ctio );
	return ctio.ioobj_;
    }

    if ( warnifexist )
    {
	uiString msg = tr("Overwrite existing '%1'?").arg(nm);
	if ( !gUiMsg().askOverwrite(msg) )
	    return false;
    }

    ctio.setObj( existioobj.release() );
    return true;
}
