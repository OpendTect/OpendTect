/*+
_____________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2020
________________________________________________________________________

 -*/

#include "uipathsel.h"

#include "file.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uifileselector.h"
#include "uigeninput.h"
#include "uilistbox.h"


uiPathListEditor::uiPathListEditor( uiParent* p, const BufferStringSet& paths )
  : uiEditObjectList(p,tr("paths"),true,true)
  , paths_(paths)
{
    listfld_->setHSzPol( uiObject::WideMax );
    fillList( 0 );
}


void uiPathListEditor::fillList( int newcur )
{
    setItems( paths_, newcur );
}


void uiPathListEditor::editReq( bool isadd )
{
    int selected_idx = currentItem();
    BufferString current_itm;
    if ( !isadd )
    {
	if ( selected_idx<0 )
	    return;
	current_itm = paths_.get( selected_idx );
    }

    uiFileSelectorSetup fssu;
    fssu.selectDirectory();
    uiFileSelector dlg( this, fssu );
    if ( !dlg.go() )
	return;

    if ( paths_.addIfNew( dlg.fileName() ) )
	selected_idx++;
    fillList( selected_idx );
}


void uiPathListEditor::removeReq()
{
    const int selected_idx = currentItem();
    if ( selected_idx<0 )
	return;
    paths_.removeSingle( selected_idx );
    fillList( selected_idx );
}


void uiPathListEditor::itemSwitch( bool up )
{
    const int selected_idx = currentItem();
    const int new_selected_idx = up ? selected_idx-1 : selected_idx+1;
    if ( selected_idx<0 || new_selected_idx<0
	 || new_selected_idx>paths_.size()-1 )
	return;
    paths_.swap( selected_idx, new_selected_idx );
    fillList( new_selected_idx );
}


uiPathSel::uiPathSel( uiParent* p, const uiString& caption )
    : uiCheckedCompoundParSel(p,uiString::empty(),false,uiStrings::sEdit())
    , selChange(this)
{
    txtfld_->setElemSzPol( uiObject::WideVar );
    txtfld_->setTitleText( caption );
    cbox_->attach( leftBorder );
    setChecked( false );
    setHAlignObj( txtfld_ );
    mAttachCB(postFinalise(), uiPathSel::initGrp);
}


uiPathSel::~uiPathSel()
{
    detachAllNotifiers();
}


void uiPathSel::initGrp( CallBacker* )
{
    mAttachCB(butPush, uiPathSel::doDlg);
    mAttachCB(checked, uiPathSel::checkedCB);
}


void uiPathSel::checkedCB( CallBacker* )
{
    if ( !isChecked() )
	setEmpty();
}


void uiPathSel::setPaths( const BufferStringSet& paths )
{
    paths_.erase();
    for ( int idx=0; idx<paths.size(); idx++ )
    {
	const BufferString& str = paths.get( idx );
	if ( !File::isDirectory( str ) ) continue;

	paths_.add( str );
    }
    if ( !paths_.isEmpty() )
	setChecked( true );
    updSummary( nullptr );
    selChange.trigger();
}


void uiPathSel::setEmpty()
{
    paths_.erase();
    updSummary( nullptr );
    selChange.trigger();
}

uiString uiPathSel::getSummary() const
{
    BufferString summ;
    for ( int idx=0; idx<paths_.size(); idx++ )
    {
	summ += paths_.get(idx);
	summ += idx == paths_.size()-1 ? BufferString::empty() : ":";
    }
    return summ.isEmpty() ? tr(" - ") : toUiString(summ);
}


class uiPathEditorDlg : public uiDialog
{ mODTextTranslationClass(uiPathEditorDlg)
public:
    uiPathEditorDlg( uiParent* p, const BufferStringSet& paths )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage(tr("Paths")),
				 mNoDlgTitle, mNoHelpKey ))
    {
	setCtrlStyle( CloseOnly );
	uiple_ = new uiPathListEditor( this, paths );
    }

    BufferStringSet getPaths() const
    {
	return uiple_->paths_;
    }
protected:
    uiPathListEditor*	uiple_;

};


void uiPathSel::doDlg( CallBacker* )
{
    uiPathEditorDlg dlg( this, paths_ );
    dlg.go();
    paths_ = dlg.getPaths();
    selChange.trigger();
}
