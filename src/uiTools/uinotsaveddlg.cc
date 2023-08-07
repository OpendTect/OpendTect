/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uinotsaveddlg.h"

#include "uidialog.h"
#include "uimain.h"
#include "uibutton.h"
#include "uilabel.h"
#include "threadwork.h"


class NotSavedPrompterData
{
public:
    NotSavedPrompterData( const uiString& str,const CallBack& cb,bool issaveas,
			 const void* dataptr )
	: string_(str), cb_(cb), issaveas_(issaveas)
	, dataptr_( dataptr )
    {}
    uiString	    string_;
    CallBack        cb_;
    bool            issaveas_;
    const void*     dataptr_;
};



class uiNotSavedDlg : public uiDialog
{ mODTextTranslationClass(uiNotSavedDlg);
public:
    uiNotSavedDlg(uiParent* p, NotSavedPrompter& prompter, bool withcancel,
		  const uiString& actiontype )
	: uiDialog( p, uiDialog::Setup( tr("Not Saved"),
		    tr("The following objects are not saved"), mNoHelpKey ) )
	, prompter_( prompter )
    {
	if ( !withcancel ) setCancelText( uiStrings::sEmptyString() );

	const uiString txt( tr("%1 now").arg( actiontype ) );
	setOkText( txt );

	for ( int idx=0; idx<prompter_.objects_.size(); idx++ )
	{
	    auto* label = new uiLabel( this,
					    prompter_.objects_[idx]->string_ );

	    auto* curbutton = uiButton::getStd( this, OD::Save,
					mCB(this,uiNotSavedDlg,buttonCB),
					prompter_.objects_[idx]->issaveas_ );
	    curbutton->attach( rightOf, label );

	    if ( idx )
		curbutton->attach( alignedBelow, buttons_[idx-1] );

	    buttons_ += curbutton;
	}
    }

    void	reportSuccessfullSave()
    {
	buttons_[activebutton_]->setText( tr("Saved") );
	buttons_[activebutton_]->setSensitive( false );
    }

    void	buttonCB( CallBacker* cb )
    {
	activebutton_ = buttons_.indexOf( (uiButton*)cb );
	prompter_.objects_[activebutton_]->cb_.doCall( &prompter_ );
    }

    const void*				getCurrentObjectData() const
    {
	return prompter_.objects_.validIdx(activebutton_)
	    ? prompter_.objects_[activebutton_]->dataptr_
	    : nullptr;
    }
    bool				isSaveAs() const
    {
	return prompter_.objects_.validIdx(activebutton_)
	    ? prompter_.objects_[activebutton_]->issaveas_
	    : false;
    }


protected:
    int				activebutton_;
    ObjectSet<uiButton>		buttons_;
    NotSavedPrompter&		prompter_;

};


NotSavedPrompter& NotSavedPrompter::NSP()
{
    mDefineStaticLocalObject( NotSavedPrompter, res, );
    return res;
}


NotSavedPrompter::NotSavedPrompter()
    : promptSaving(this)
    , dlg_(nullptr)
    , queueid_(
	Threads::WorkManager::twm().addQueue(Threads::WorkManager::Manual,
						"NotSavedPrompter"))
{
    mAttachCB( Threads::WorkManager::twm().isShuttingDown,
	       NotSavedPrompter::closeQueueCB );
}


void NotSavedPrompter::closeQueueCB( CallBacker* cb )
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );
    queueid_ = -1;
}


bool NotSavedPrompter::doTrigger( uiParent* parent, bool withcancel,
				  const uiString& actiontype )
{
    promptSaving.trigger();
    if ( !objects_.size() )
	return true;

    dlg_ = new uiNotSavedDlg( parent, *this, withcancel, actiontype );
    dlg_->setModal( true );
    bool retval = dlg_->go();
    deleteAndNullPtr( dlg_ );

    deepErase( objects_ );
    if ( retval )
	Threads::WorkManager::twm().executeQueue( queueID() );
    else
	Threads::WorkManager::twm().emptyQueue( queueID(), false );

    return retval;
}


void NotSavedPrompter::addObject( const char* str,const CallBack& cb,
	bool issaveas, const void* dataptr )
{
    objects_ += new NotSavedPrompterData( toUiString(str), cb, issaveas,
								    dataptr );
}

void NotSavedPrompter::addObject( const uiString& str,const CallBack& cb,
    bool issaveas, const void* dataptr )
{
    objects_ += new NotSavedPrompterData( str, cb, issaveas, dataptr );
}


void NotSavedPrompter::reportSuccessfullSave()
{
    if ( dlg_ )
	dlg_->reportSuccessfullSave();
}


uiParent* NotSavedPrompter::getParent()
{ return dlg_; }


const void* NotSavedPrompter::getCurrentObjectData() const
{
    return dlg_ ? dlg_->getCurrentObjectData() : nullptr;
}


bool NotSavedPrompter::isSaveAs() const
{
    return dlg_ ? dlg_->isSaveAs() : false;
}
