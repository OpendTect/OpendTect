/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          November 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uinotsaveddlg.cc,v 1.3 2011-02-16 22:11:10 cvskris Exp $";

#include "uinotsaveddlg.h"

#include "uidialog.h"
#include "uimain.h"
#include "uibutton.h"
#include "uilabel.h"


struct NotSavedPrompterData
{
public:
    NotSavedPrompterData(const char* str,const CallBack& cb,bool issaveas,
			 const void* dataptr )
	: string_(str), cb_(cb), issaveas_(issaveas)
	, dataptr_( dataptr )
    {}
    BufferString    string_;
    CallBack        cb_;
    bool            issaveas_;
    const void*     dataptr_;
};



class uiNotSavedDlg : public uiDialog
{
public:
    uiNotSavedDlg(uiParent* p, NotSavedPrompter& prompter, bool withcancel,
	          bool isshutdown )
	: uiDialog( p, uiDialog::Setup( "Not Saved",
		    "The following objects are not saved", mNoHelpID ) )
	, prompter_( prompter )
    {
	if ( !withcancel ) setCancelText( 0 );
	else
	{
	    setCancelText(
		    isshutdown ? "Cancel shutdown" : "Cancel survey change" );
	}

	setOkText( isshutdown ? "Shutdown now" : "Switch survey now" );

	for ( int idx=0; idx<prompter_.objects_.size(); idx++ )
	{
	    uiLabel* label =
		new uiLabel( this, prompter_.objects_[idx]->string_ );

	    uiPushButton* curbutton = new uiPushButton( this, "Save now",
		    mCB(this,uiNotSavedDlg,buttonCB),
		    prompter_.objects_[idx]->issaveas_ );
	    curbutton->attach( rightOf, label );

	    if ( idx ) curbutton->attach( alignedBelow, buttons_[idx-1] );
	    buttons_ += curbutton;
	}
    }

    void	reportSuccessfullSave()
    {
	buttons_[activebutton_]->setText( "Saved" );
	buttons_[activebutton_]->setSensitive( false );
    }
    void	buttonCB(CallBacker* cb)
    {
	activebutton_ = buttons_.indexOf( (uiPushButton*) cb );
	prompter_.objects_[activebutton_]->cb_.doCall( &prompter_ );
    }

    const void*				getCurrentObjectData() const
    {
	return prompter_.objects_.validIdx(activebutton_)
	    ? prompter_.objects_[activebutton_]->dataptr_
	    : 0;
    }
    bool 				isSaveAs() const
    {
	return prompter_.objects_.validIdx(activebutton_)
	    ? prompter_.objects_[activebutton_]->issaveas_
	    : false;
    }


protected:
    int					activebutton_;
    ObjectSet<uiPushButton>		buttons_;
    NotSavedPrompter&			prompter_;
};


NotSavedPrompter& NotSavedPrompter::NSP()
{
    static PtrMan<NotSavedPrompter> res = new NotSavedPrompter;
    return *res;
}


NotSavedPrompter::NotSavedPrompter()
    : promptSaving( this )
    , dlg_( 0 )
{}


bool NotSavedPrompter::doTrigger( uiParent* parent, bool withcancel,
       				  bool isshutdown )
{
    promptSaving.trigger();
    if ( !objects_.size() )
	return true;

    dlg_ = new uiNotSavedDlg( parent, *this, withcancel, isshutdown );
    bool retval = dlg_->go();
    delete dlg_;
    dlg_ = 0;

    deepErase( objects_ );

    return retval;
}


void NotSavedPrompter::addObject( const char* str,const CallBack& cb,
	bool issaveas, const void* dataptr )
{
    objects_ += new NotSavedPrompterData( str, cb, issaveas, dataptr );
}



void NotSavedPrompter::reportSuccessfullSave()
{
    if ( dlg_ ) dlg_->reportSuccessfullSave();
}


uiParent* NotSavedPrompter::getParent()
{ return dlg_; }


const void* NotSavedPrompter::getCurrentObjectData() const
{ return dlg_ ? dlg_->getCurrentObjectData() : 0; }


bool NotSavedPrompter::isSaveAs() const
{ return dlg_ ? dlg_->isSaveAs() : false; }
