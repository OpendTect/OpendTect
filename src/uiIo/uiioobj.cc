/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2006
________________________________________________________________________

-*/


#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iostrm.h"
#include "survinfo.h"
#include "transl.h"
#include "filepath.h"
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
		mess = tr("Permanently Delete '%1'%2");
		mess.arg( ioobj_.uiName() );
		mess.arg( isoutside ? tr("\nFile is not in current survey.\n"
				     "Specify what you would like to delete")
				    : tr(" from Database?") );
	    }
	    else
	    {
		mess = tr("Delete '%1' with folder\n'%2'%3");
		mess.arg( ioobj_.uiName() );
		mess.arg( ioobj_.fullUserExpr(true) );
		mess.arg( tr("\n- and everything in it! - ?") );
	    }

	    if ( isoutside )
	    {
		const int resp = uiMSG().question( mess, tr("Delete file"),
						   tr("Delete link"),
						   uiStrings::sCancel(),
						   tr("Delete data") );
		if ( resp < 0 )
		    return false;

		dorm = resp;
	    }
	    else if ( doconfirm && !uiMSG().askGoOn(mess,uiStrings::sDelete(),
							 uiStrings::sCancel()) )
	    {
		if ( mustrm )
		    return false;

		dorm = false;
	    }
	}
    }

    if ( dorm && !IOM().implRemove(ioobj_) )
    {
	if ( !silent_ )
	{
	    uiString mess = tr("Could not delete data file(s).\n"
			       "Remove entry from list anyway?");
	    if ( !uiMSG().askRemove(mess) )
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
