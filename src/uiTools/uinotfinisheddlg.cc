/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uinotfinisheddlg.h"

#include "uidialog.h"
#include "uimain.h"
#include "uibutton.h"
#include "uilabel.h"
#include "threadwork.h"


class NotFinishedPrompterData
{
public:
	NotFinishedPrompterData( const uiString& str )
	: string_(str)
	{}

	NotFinishedPrompterData( const uiString& str,const CallBack& cb,
							 const void* dataptr )
	: NotFinishedPrompterData(str)
	{
		cb_ = &cb;
		dataptr_ = dataptr;
	}

	uiString	string_;
	const CallBack* cb_			= nullptr;
	const void*	dataptr_	= nullptr;
};



class uiNotFinishedDlg : public uiDialog
{ mODTextTranslationClass(uiNotFinishedDlg);
public:
    uiNotFinishedDlg(uiParent* p, NotFinishedPrompter& prompter,
		     bool withcancel, const uiString& actiontype )
    : uiDialog(p, uiDialog::Setup(tr("Not Finished"),
				  tr("The following processes are running"),
				  mNoHelpKey))
    , prompter_(prompter)
    {
	buttons_.setNullAllowed();
	if ( !withcancel )
	    setCancelText( uiStrings::sEmptyString() );

	const uiString txt( tr("%1 now").arg( actiontype ) );
	setOkText( txt );

	for ( int idx=0; idx<prompter_.objects_.size(); idx++ )
	{
	    auto* label = new uiLabel( this, prompter_.objects_[idx]->string_ );
	    if ( prompter_.objects_[idx]->cb_ )
	    {
		auto* curbutton = uiButton::getStd( this, OD::Cancel,
						    mCB(this,uiNotFinishedDlg,
							buttonCB), true );
		curbutton->attach( rightOf, label );
		buttons_ += curbutton;
	    }
	    else
		buttons_ += nullptr;
	}
    }


    void reportSuccessfulStop()
    {
	if ( buttons_[activebutton_] )
	{
	    buttons_[activebutton_]->setText( tr("Stopped") );
	    buttons_[activebutton_]->setSensitive( false );
	}
    }


    void buttonCB( CallBacker* cb )
    {
	activebutton_ = buttons_.indexOf( (uiButton*)cb );
	if ( prompter_.objects_[activebutton_]->cb_ )
	    prompter_.objects_[activebutton_]->cb_->doCall( &prompter_ );
    }


    const void* getCurrentObjectData() const
    {
	return prompter_.objects_.validIdx(activebutton_) ?
			prompter_.objects_[activebutton_]->dataptr_ : nullptr;
    }

protected:
    int				activebutton_;
    ObjectSet<uiButton>		buttons_;
    NotFinishedPrompter&	prompter_;
};


NotFinishedPrompter& NotFinishedPrompter::NFP()
{
    mDefineStaticLocalObject( NotFinishedPrompter, res, );
    return res;
}


NotFinishedPrompter::NotFinishedPrompter()
    : promptEnding(this)
    , queueid_(Threads::WorkManager::twm().addQueue(
			Threads::WorkManager::Manual, "NotFinishedPrompter"))
{
    mAttachCB( Threads::WorkManager::twm().isShuttingDown,
	       NotFinishedPrompter::closeQueueCB );
}


NotFinishedPrompter::~NotFinishedPrompter()
{
    detachAllNotifiers();
}


void NotFinishedPrompter::closeQueueCB( CallBacker* cb )
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );
    queueid_ = -1;
}


bool NotFinishedPrompter::doTrigger( uiParent* parent, bool withcancel,
				     const uiString& actiontype )
{
    promptEnding.trigger();
    if ( !objects_.size() )
	return true;

    dlg_ = new uiNotFinishedDlg( parent, *this, withcancel, actiontype );
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


void NotFinishedPrompter::addObject( const char* str )
{
    objects_ += new NotFinishedPrompterData( toUiString(str) );
}


void NotFinishedPrompter::addObject( const char* str,const CallBack& cb,
				     const void* dataptr )
{
    objects_ += new NotFinishedPrompterData( toUiString(str), cb, dataptr );
}


void NotFinishedPrompter::addObject( const uiString& str )
{
    objects_ += new NotFinishedPrompterData( str );
}


void NotFinishedPrompter::addObject( const uiString& str, const CallBack& cb,
				     const void* dataptr )
{
    objects_ += new NotFinishedPrompterData( str, cb, dataptr );
}


void NotFinishedPrompter::removeObject( const uiString& str )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( str==objects_[idx]->string_ )
	{
	    delete objects_.removeSingle( idx );
	    break;
	}
    }
}


void NotFinishedPrompter::removeObject( const char* str )
{
    removeObject( toUiString(str) );
}


void NotFinishedPrompter::reportSuccessfulStop()
{
    if ( dlg_ )
	dlg_->reportSuccessfulStop();
}


uiParent* NotFinishedPrompter::getParent()
{
    return dlg_;
}


const void* NotFinishedPrompter::getCurrentObjectData() const
{
    return dlg_ ? dlg_->getCurrentObjectData() : nullptr;
}
