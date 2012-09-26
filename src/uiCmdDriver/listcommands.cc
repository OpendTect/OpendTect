/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "listcommands.h"
#include "cmddriverbasics.h"
#include "cmdrecorder.h"
#include "uimenu.h"


namespace CmdDrive
{


#define mParListSelPre( objnm, listobj, itemstr, itemnr, itmidxs, ambicheck ) \
\
    TypeSet<int> itmidxs; \
\
    if ( !mIsUdf(itemnr) ) \
    { \
	for ( int idx=0; idx<listobj->size(); idx++ ) \
	{ \
	    if ( mSearchKey(itemstr).isMatching(listobj->textOfItem(idx)) ) \
	    	itmidxs += idx; \
	} \
        mParStrPre( objnm, itmidxs, 0, itemstr, itemnr, "string", ambicheck ); \
	wildcardMan().check( mSearchKey(itemstr), \
			     listobj->textOfItem(itmidxs[0]) ); \
    } \


bool ComboCmd::act( const char* parstr )
{
    mParKeyStrInit( "combobox", parstr, parnext, keys, selnr );
    mParItemSelInit( "item", parnext, partail, itemstr, itemnr, false );
    mParTail( partail );

    mFindObjects( objsfound, uiComboBox, keys, nrgrey );
    mParKeyStrPre( "combobox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiComboBox*, uicombox, objsfound[0] );
    mParListSelPre( "item", uicombox, itemstr, itemnr, itemidxs, true );

    mActivate( Combo, Activator(*uicombox, itemidxs[0]) );
    return true;
}


ComboActivator::ComboActivator( const uiComboBox& uicombox, int itmidx )
    : actcombox_( const_cast<uiComboBox&>(uicombox) )
    , actitmidx_( itmidx )
{}


void ComboActivator::actCB( CallBacker* cb )
{
    if ( actitmidx_>=0 && actitmidx_<actcombox_.size() )
    {
	actcombox_.setCurrentItem( actitmidx_ );
	actcombox_.selectionChanged.trigger();
    }
}


bool NrComboItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "combobox", parnext, partail, keys, selnr );
    mParTail( partail );

    mFindObjects( objsfound, uiComboBox, keys, nrgrey );
    mParKeyStrPre( "combobox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiComboBox*, uicombox, objsfound[0] );
    mParIdentPost( identname, uicombox->size(), parnext );
    return true;
}


bool CurComboItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "combobox", parnext, parnexxt, keys, selnr );
    mParFormInit( parnexxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiComboBox, keys, nrgrey );
    mParKeyStrPre( "combobox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiComboBox*, uicombox, objsfound[0] );
    const int curidx = uicombox->currentItem();
    mParForm( answer, form, uicombox->textOfItem(curidx), curidx+1 );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool GetComboItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "combobox", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, parnexxxt, itemstr, itemnr, false );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiComboBox, keys, nrgrey );
    mParKeyStrPre( "combobox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiComboBox*, uicombox, objsfound[0] );
    mParListSelPre( "item", uicombox, itemstr, itemnr, itemidxs, true );
    mParForm( answer, form, uicombox->textOfItem(itemidxs[0]), itemidxs[0]+1 );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool IsComboItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "combobox", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, partail, itemstr, itemnr, false );
    mParTail( partail );

    mFindObjects( objsfound, uiComboBox, keys, nrgrey );
    mParKeyStrPre( "combobox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiComboBox*, uicombox, objsfound[0] );
    mParListSelPre( "item", uicombox, itemstr, itemnr, itemidxs, true );

    const int ison = uicombox->currentItem()==itemidxs[0] ? 1 : 0;
    mParIdentPost( identname, ison, parnext );
    return true;
}


bool ListClickCmd::act( const char* parstr )
{
    mParKeyStrInit( "list", parstr, parnext, keys, selnr );
    mParItemSelInit( "item", parnext, parnexxt, itemstr, itemnr, false );
    mParMouse( parnexxt, partail, clicktags, "Left" );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    
    mActivate( List, Activator(*uilist, itemidxs[0], clicktags) );

    return true;
}


ListActivator::ListActivator( const uiListBox& uilist, int itmidx,
			      const BufferStringSet& clicktags )
    : actlist_( const_cast<uiListBox&>(uilist) )
    , actitmidx_( itmidx )
    , actclicktags_( clicktags )
{}


#define mHandleSelectionChangedBegin( oldselitems ) \
\
    const bool notifierwasenabled = actlist_.selectionChanged.disable(); \
    TypeSet<int> oldselitems; \
    for ( int idx=0; idx<actlist_.size(); idx++ ) \
    { \
	if ( actlist_.isSelected(idx) ) \
	   oldselitems += idx; \
    }

#define mInitListSelection( selecteditems, curitmidx ) \
\
    actlist_.clearSelection(); \
    actlist_.setCurrentItem( curitmidx ); \
    for ( int idx=0; idx<selecteditems.size(); idx++ ) \
	actlist_.setSelected( selecteditems[idx], true ); \

#define mHandleSelectionChangedEnd( oldselitems ) \
\
    if ( notifierwasenabled ) \
	actlist_.selectionChanged.enable(); \
\
    for ( int idx=0; idx<actlist_.size(); idx++ ) \
    { \
	if ( actlist_.isSelected(idx) == (oldselitems.indexOf(idx)<0) ) \
	{ \
	    actlist_.selectionChanged.trigger(); \
	    break; \
	} \
    }
	
#define mHandleLeftRightClick() \
{ \
    if ( actclicktags_.isPresent("Check") && \
	 actlist_.isItemCheckable(actitmidx_) ) \
    { \
	actlist_.setItemChecked( actitmidx_, \
				 !actlist_.isItemChecked(actitmidx_) ); \
    } \
    if ( actclicktags_.isPresent("Left") ) \
	actlist_.leftButtonClicked.trigger(); \
    if ( actclicktags_.isPresent("Right") ) \
	actlist_.rightButtonClicked.trigger(); \
}

void ListActivator::actCB( CallBacker* cb )
{
    if ( actitmidx_>=0 && actitmidx_<actlist_.size() )
    {
	if ( actlist_.maxNrOfSelections()>0 )
	{
	    mHandleSelectionChangedBegin( oldselitems );
	    mInitListSelection( oldselitems, actitmidx_ );
	    const bool wasselected = oldselitems.indexOf(actitmidx_) >= 0;

	    if ( actclicktags_.isPresent("Ctrl") )
		actlist_.setSelected( actitmidx_, !wasselected );
	    else
	    {
		if ( !actclicktags_.isPresent("Right") || !wasselected )
		    actlist_.clearSelection();
		actlist_.setSelected( actitmidx_, true );
	    }

	    mHandleSelectionChangedEnd( oldselitems );
	}

	mHandleLeftRightClick();
	if ( actclicktags_.isPresent("Double") )
	{
	    actlist_.doubleClicked.trigger();
	    mHandleLeftRightClick();
	}
    }
}


#define mListButtonCheck( uilist, itemstr, itemidx ) \
\
    if ( !uilist->isItemCheckable(itemidx) ) \
    { \
	mWinErrStrm << "List item \"" << itemstr << "\" has no button" \
		    << std::endl; \
	return false; \
    }

bool ListButtonCmd::act( const char* parstr )
{
    mParKeyStrInit( "list", parstr, parnext, keys, selnr );
    mParItemSelInit( "item", parnext, parnexxt, itemstr, itemnr, false );
    mParMouse( parnexxt, parnexxxt, clicktags, "Left" );
    mButtonCmdMouseTagCheck( clicktags );
    mParOnOffInit( parnexxxt, partail, onoff );
    mParTail( partail );

    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    mListButtonCheck( uilist, itemstr, itemidxs[0] );

    mParOnOffPre("check-box", onoff, uilist->isItemChecked(itemidxs[0]), true);

    clicktags.add( "Check" );
    mActivate( List, Activator(*uilist, itemidxs[0], clicktags) );

    mParOnOffPost( "check-box", onoff, uilist->isItemChecked(itemidxs[0]) );
    return true;
}


static bool selectionEquals( const uiListBox& uilist, 
			     const TypeSet<int>& selset ) 
{
    for ( int idx=0; idx<uilist.size(); idx++ )
    {
	const bool tobeselected = selset.indexOf(idx)>=0;
	if ( uilist.isSelected(idx) != tobeselected )
	    return false;
    }
    return true;
}


bool ListSelectCmd::act( const char* parstr )
{
    mParKeyStrInit( "list", parstr, parnext, keys, selnr );
    mParItemSelInit( "first item", parnext, parnexxt, itemstr1, itemnr1, false);
    mParItemSelInit( "last item", parnexxt, parnexxxt, itemstr2, itemnr2, true);
    mParOnOffInit( parnexxxt, partail, onoff );
    mParTail( partail );
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );

    if ( uilist->maxNrOfSelections() == 0 )
    {
	mWinErrStrm << "This list allows no item selection" << std::endl; 
	return false; 
    }

    mParListSelPre( "first item", uilist, itemstr1, itemnr1, itemidxs1, false );
    mParListSelPre( "last item", uilist, itemstr2, itemnr2, itemidxs2, false );

    const int nritems = uilist->size();
    TypeSet<int> selset;
    for ( int idx=0; idx<nritems; idx++ )
    {
	bool specified = itemidxs1.indexOf(idx)>=0;
	if ( !itemidxs2.isEmpty() )
	{
	    const int firstidx = itemidxs1[0];
	    const int lastidx = itemidxs2[ itemidxs2.size()-1 ];

	    if ( firstidx <= lastidx )
		specified = idx>=firstidx && idx<=lastidx;
	    else
		specified = idx>=firstidx || idx<=lastidx;
	}

	if ( !onoff    && !specified )
	    continue;
	if ( onoff==-1 && (specified || !uilist->isSelected(idx)) )
	    continue;
	if ( onoff==1  && !specified && !uilist->isSelected(idx)  )
	    continue;

	selset += idx;
    }

    if ( uilist->maxNrOfSelections()==1 && selset.size()!=1 )
    {
	mWinErrStrm << "This single-selection list does not allow "
		    << selset.size() << " selected items" << std::endl;
	return false;
    }

    if ( selectionEquals(*uilist,selset) )
    {
	mWinWarnStrm << "List already showed the specified selection"
		     << std::endl;
    }

    mActivate( ListSelect, Activator(*uilist, selset) );

    if ( uilist->size()==nritems && !selectionEquals(*uilist,selset) )
    {
	mWinWarnStrm << "Specified selection has been overruled" << std::endl;
    }

    return true; 
}


ListSelectActivator::ListSelectActivator( const uiListBox& uilist,
					  const TypeSet<int>& selset )
    : actlist_( const_cast<uiListBox&>(uilist) )
    , actselset_( selset )
{}


void ListSelectActivator::actCB( CallBacker* cb )
{
    const int maxselectable = actlist_.maxNrOfSelections();
    if ( maxselectable>0 && actselset_.size()<=maxselectable )
    {
	mHandleSelectionChangedBegin( oldselitems );
	const int curitmidx = actselset_.isEmpty() ? -1 : actselset_[0];
	mInitListSelection( actselset_, curitmidx );
	mHandleSelectionChangedEnd( oldselitems );
    }
}


bool ListMenuCmd::act( const char* parstr )
{
    mParKeyStrInit( "list", parstr, parnext, keys, selnr );
    mParItemSelInit( "item", parnext, parnexxt, itemstr, itemnr, false );
    mParMouse( parnexxt, parnexxxt, clicktags, "Right" );
    mParPathStrInit( "menu", parnexxxt, parnexxxxt, menupath );
    mParOnOffInit( parnexxxxt, partail, onoff );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    
    prepareIntercept( menupath, onoff );
    
    mActivate( List, Activator(*uilist, itemidxs[0], clicktags) );

    BufferString objnm = "List item "; objnm += itemidxs[0]+1;
    return didInterceptSucceed( objnm );
}


bool NrListItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, partail, keys, selnr );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );

    mParIdentPost( identname, uilist->size(), parnext );
    return true;
}


bool IsListItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, partail, itemstr, itemnr, false );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    
    const int ison = uilist->maxNrOfSelections()<1 ? -1 :
		     uilist->isSelected( itemidxs[0] ) ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


bool IsListButtonOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, partail, itemstr, itemnr, false );
    mParTail( partail );

    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    mListButtonCheck( uilist, itemstr, itemidxs[0] );
    
    const int ison = uilist->isItemChecked(itemidxs[0]) ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


bool CurListItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParFramed( parnexxt, parnexxxt, framed );
    mParExtraFormInit( parnexxxt, partail, form, "Number`Color" );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );

    int curidx = -1;
    for ( int idx=0; idx<uilist->size(); idx++ ) 
    {
	if ( !uilist->isSelected(idx) )
	    continue;

	if ( curidx >= 0 )
	{
	    curidx = -1;
	    break;
	}

	curidx = idx;
    }
    if ( framed )
	curidx = uilist->currentItem();

    const char* text = uilist->textOfItem( curidx );
    mGetColorString( uilist->getColor(curidx), curidx>=0, colorstr );
    mParForm( answer, form, text, curidx+1 );
    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname, answer, parnext, form!=Colour );
    return true;
}


bool GetListItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, parnexxxt, itemstr, itemnr, false );
    mParExtraFormInit( parnexxxt, partail, form, "Number`Color" );
    mParTail( partail ); 
    
    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );

    const char* text = uilist->textOfItem( itemidxs[0] );
    mGetColorString( uilist->getColor(itemidxs[0]), true, colorstr );
    mParForm( answer, form, text, itemidxs[0]+1 );
    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname, answer, parnext, form!=Colour );
    return true;
}


#define mInterceptListMenu( menupath, allowroot, uilist, itemidx ) \
\
    BufferStringSet clicktags; clicktags.add( "Right" ); \
    CmdDriver::InterceptMode mode = \
		    allowroot ? CmdDriver::NodeInfo : CmdDriver::ItemInfo; \
    prepareIntercept( menupath, 0, mode ); \
    mActivate( List, Activator(*uilist, itemidx, clicktags) ); \
    BufferString objnm = "List item "; objnm += itemidx+1; \
    if ( !didInterceptSucceed(objnm) ) \
	return false;

bool NrListMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, parnexxxt, itemstr, itemnr, false );
    mParPathStrInit( "menu", parnexxxt, partail, menupath );
    mParTail( partail );

    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    mInterceptListMenu( menupath, true, uilist, itemidxs[0] );

    mParIdentPost( identname, interceptedMenuInfo().nrchildren_, parnext );
    return true;
}


bool IsListMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, parnexxxt, itemstr, itemnr, false );
    mParPathStrInit( "menu", parnexxxt, partail, menupath );
    mParTail( partail );

    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    mInterceptListMenu( menupath, false, uilist, itemidxs[0] );

    mParIdentPost( identname, interceptedMenuInfo().ison_, parnext );
    return true;
}


bool GetListMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "list", parnext, parnexxt, keys, selnr );
    mParItemSelInit( "item", parnexxt, parnexxxt, itemstr, itemnr, false );
    mParPathStrInit( "menu", parnexxxt, parnexxxxt, menupath );
    mParFormInit( parnexxxxt, partail, form );
    mParTail( partail );

    mFindListTableObjs( "list", objsfound, uiListBox, keys, nrgrey );
    mParKeyStrPre( "list", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiListBox*, uilist, objsfound[0] );
    mParListSelPre( "item", uilist, itemstr, itemnr, itemidxs, true );
    mInterceptListMenu( menupath, false, uilist, itemidxs[0] );

    const MenuInfo menuinfo = interceptedMenuInfo();
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname, answer, parnext );
    return true;
}


//====== CmdComposers =========================================================


#define mGetListItemName( uilistobj, curitemidx, curitemname, casedep ) \
    mGetItemName( uilistobj,size,textOfItem,curitemidx,curitemname,casedep )


void ComboCmdComposer::init()
{
    itemedited_ = false;
    bursteventnames_.add( "editTextChanged" );
}


bool ComboCmdComposer::accept( const CmdRecEvent& ev )
{
    const bool accepted = CmdComposer::accept( ev );
    if ( ignoreflag_ || quitflag_ )
	return accepted;

    char* msgnext; char* msgnexxt;
    int oldnritems; int oldcuritem;

    if ( accepted )
    {
	oldnritems = strtol( ev.msg_, &msgnext, 0 );
	oldcuritem = strtol( msgnext, &msgnexxt, 0 );
	mSkipBlanks( msgnexxt );
	if ( mMatchCI(msgnexxt, "editTextChanged") )
	    notDone();
    }

    if ( ev.begin_ == ev.openqdlg_ )
	return accepted;

    mDynamicCastGet( const uiComboBox*, uicombo, eventlist_[0]->object_ );

    if ( accepted && mMatchCI(msgnexxt, "editTextChanged") )
    {
	BufferString inittext = uicombo->textOfItem( uicombo->currentItem() );

	if ( (oldnritems==uicombo->size() && oldcuritem==uicombo->currentItem())
	     || (mIsUdf(oldnritems) && inittext!=uicombo->text()) )
	{
	    itemedited_ = true;
	    edittext_ = uicombo->text();
	}

	return true;
    }

    bool enter = false;
    if ( accepted && mMatchCI(msgnexxt, "selectionChanged") )
    {
	if ( !itemedited_ )
	{
	    mGetListItemName( uicombo, uicombo->currentItem(), curitemname,
			      namecasedep );
	    insertWindowCaseExec( ev, namecasedep );
	    mRecOutStrm << "Combo \"" << ev.keystr_ << "\" \""
			<< curitemname << "\"" << std::endl;
	    return true;
	}

	enter = edittext_ == uicombo->text();
    }

    mWriteInputCmd( itemedited_, edittext_, enter ); 
	    
    if ( !enter )
	mRefuseAndQuit();

    return accepted;
}


void ListCmdComposer::init()
{
    reInit();
    stagenr_ = -1;

    bursteventnames_.add( "selectionChanged" );
}


void ListCmdComposer::reInit()
{
    stagenr_ = 0;
    selchanged_ = false;
    clickedidx_ = -1;
    leftclicked_ = false; 
    ctrlclicked_ = false;
    listcmdsflushed_ = false;
}


#define mGetListBox( uilist, retval ) \
\
    if ( eventlist_.isEmpty() ) \
	return retval; \
    mDynamicCastGet( const uiListBox*, uilist, eventlist_[0]->object_ ); \
    if ( !uilist ) \
	return retval;


void ListCmdComposer::updateInternalState()
{
    mGetListBox( uilist, );
    if ( updateflag_ || CmdRecStopper::isInStopperList(uilist) )
    {
	storeListState();
	updateflag_ = false;
    }
}


void ListCmdComposer::storeListState()
{
    mGetListBox( uilist, );
    selecteditems_.erase();
    checkeditems_.erase();

    for ( int idx=0; idx<uilist->size(); idx++ )
    {
	if ( uilist->isSelected(idx) )
	    selecteditems_ += idx;
	if ( uilist->isItemChecked(idx) )
	    checkeditems_ += idx;
    } 
}


void ListCmdComposer::labelStoredStateOld()
{
    wasselecteditems_ = selecteditems_;
    wascheckeditems_ = checkeditems_;
}


void ListCmdComposer::labelStoredStateNew()
{
    isselecteditems_ = selecteditems_;
    ischeckeditems_ = checkeditems_;
}


#define mIsSet( iswasselectedchecked, itemidx ) \
    ( iswasselectedchecked##items_.indexOf(itemidx) >= 0 )


void ListCmdComposer::writeListSelect()
{
    if ( selchanged_ )
    {
	if ( stagenr_ < 2 )
	{
	    storeListState();
	    labelStoredStateNew();
	}

	if ( stagenr_>1 && isselecteditems_.size()==1 &&
			   mIsSet(isselected,clickedidx_) )
	    return;

	const int nrifdifferential = writeListSelect( true, true );
	const int nrifclearedfirst = writeListSelect( false, true );

	const bool differential = nrifclearedfirst==0 ||
				  nrifdifferential < nrifclearedfirst;

	writeListSelect( differential );
    }
}


int ListCmdComposer::writeListSelect( bool differential, bool virtually )
{
    mGetListBox( uilist, -1 );
    int nrlistselects = 0;
    int firstidx = mUdf(int);
    int lastidx;
    int blockstate;

    for ( int idx=0; idx<=uilist->size(); idx++ )
    {
	const int oldstate = differential ? mIsSet(wasselected,idx) : 0;

	int curstate = oldstate;
	if ( idx == uilist->size() )
	    curstate = mUdf(int);
	else if ( idx != clickedidx_ )
	    curstate = mIsSet( isselected, idx );
	else if ( !mIsSet(isselected, idx) )
	    curstate = 1;
	else if ( leftclicked_ )
	    curstate = 0;
	else if ( !mIsUdf(firstidx) )
	    curstate = blockstate;

	if ( !virtually && idx==clickedidx_ )
	    ctrlclicked_ = curstate != mIsSet(isselected, idx);

	if ( mIsUdf(firstidx) )
	{
	    if ( curstate != oldstate )
	    {
		firstidx = idx;
		lastidx = idx;
		blockstate = curstate;
	    }
	}
	else if ( curstate == blockstate )
	{
	    if ( curstate != oldstate )
		lastidx = idx;
	}
	else 
	{
	    idx--;

	    if ( !virtually )
	    {
		const bool clear = !differential && !nrlistselects;
		writeListSelect( firstidx, lastidx, blockstate, clear );
	    }

	    nrlistselects++;
	    firstidx = mUdf(int);
	}
    }
    return nrlistselects;
}


void ListCmdComposer::writeListSelect( int firstidx, int lastidx,
				       int blockstate, bool clear )
{
    mGetListBox( uilist, );

    BufferString itemrg = "\"";
    mGetListItemName( uilist, firstidx, itemname1, casedep1 );
    mGetListItemName( uilist, lastidx, itemname2, casedep2 );
    itemrg += itemname1; itemrg += "\"";
    if ( lastidx != firstidx )
    {
	itemrg += " \""; itemrg += itemname2; itemrg += "\"";
    }

    const char* onoff = clear ? "" : ( blockstate ? " On" : " Off" );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, casedep1 || casedep2 );
    mRecOutStrm << "ListSelect \"" << ev.keystr_ << "\" " << itemrg
		<< onoff << std::endl;
}


#define mGetMouseTag( mousetag ) \
\
    BufferString mousetag = " "; \
    if ( ctrlclicked_ ) \
	mousetag += "Ctrl"; \
    if ( stagenr_==3 || stagenr_==4 ) \
	mousetag += "Double"; \
    mousetag += leftclicked_ ? "Left" : "Right";


void ListCmdComposer::writeListButton()
{
    mGetListBox( uilist, );
    mGetListItemName( uilist, clickedidx_, itemname, casedep );

    mGetMouseTag( mousetag );
    if ( mMatchCI(mousetag, " Left") )
	mousetag.setEmpty();

    const char* onoff = mIsSet(ischecked,clickedidx_) ? " On" : " Off";

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, casedep );
    mRecOutStrm << "ListButton \"" << ev.keystr_ << "\" \"" << itemname
		<< "\"" << mousetag << onoff << std::endl;
}


void ListCmdComposer::writeListMenu( const CmdRecEvent& menuevent )
{
    mGetListBox( uilist, );
    mGetListItemName( uilist, clickedidx_, itemname, casedep );

    mGetMouseTag( mousetag );
    if ( mMatchCI(mousetag, " Right") )
	mousetag.setEmpty();

    const char* onoff = !menuevent.mnuitm_->isCheckable() ? "" :
			( menuevent.mnuitm_->isChecked() ? " On" : " Off" );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, menuevent.casedep_ || casedep );
    mRecOutStrm << "ListMenu \"" << ev.keystr_ << "\" \"" << itemname
		<< "\"" << mousetag << " \"" << menuevent.menupath_
		<< "\"" << onoff << std::endl;
}


void ListCmdComposer::writeListClick()
{
    if ( stagenr_ < 2 )
	return;

    mGetListBox( uilist, );
    mGetListItemName( uilist, clickedidx_, itemname, casedep );
    mGetMouseTag( mousetag );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, casedep );
    mRecOutStrm << "ListClick \"" << ev.keystr_ << "\" \"" << itemname
		<< "\"" << mousetag << std::endl;
}


bool ListCmdComposer::accept( const CmdRecEvent& ev )
{
    const bool accepted = CmdComposer::accept( ev );
    if ( quitflag_ || ignoreflag_ )
	return accepted;

    if ( !accepted && stagenr_<2 && !selchanged_ )
	return false;

    BufferString notifiername;
    char* msgnexxt; int itemidx = -1;

    if ( accepted )
    {
	if ( done() )
	{
	    updateflag_ = true;
	    notDone();
	}

	if ( ev.nraccepts_ ) 
	    return true;

	const char* msgnext = getNextWord( ev.msg_, notifiername.buf() );
	itemidx = strtol( msgnext, &msgnexxt, 0 );

	if ( mMatchCI(notifiername, "itemEntered") && !ev.begin_ )
	{
	    shrinkEventList( 1, -3 );
	    voideventnames_.add( "itemEntered" );
	    stagenr_ = 0;
	    //mNotifyTest( uiListBox, ev.object_, selectionChanged );
	    //mNotifyTest( uiListBox, ev.object_, rightButtonClicked );
	    //mNotifyTest( uiListBox, ev.object_, doubleClicked );
	    return true;
	}
    }

    if ( ev.begin_ == ev.openqdlg_ )
	return accepted;

    if ( accepted )
    {
	const bool notileft = mMatchCI( notifiername, "leftButtonClicked" );
	const bool notiright = mMatchCI( notifiername, "rightButtonClicked" );

	if ( stagenr_ == -1 )
	    return true;

	if ( stagenr_ == 0 )
	{
	    shrinkEventList( 3, -2 );
	    labelStoredStateOld();
	    stagenr_ = 1;
	}

	if ( stagenr_ == 1 )
	{
	    if ( notileft || notiright )
	    {
		stagenr_ = 2;
		clickedidx_ = itemidx;
		leftclicked_ = notileft;
		ctrlclicked_ = false;
		storeListState();
		labelStoredStateNew();
	    }
	    if ( mMatchCI(notifiername, "selectionChanged") )
		selchanged_ = true;

	    return true;
	}

	if ( stagenr_ == 2 )
	{
	    if ( mMatchCI(notifiername,"doubleClicked") &&
		 clickedidx_==itemidx ) 
	    {
		stagenr_ = 3;
		return true;
	    }
	}

	if ( stagenr_ == 3 )
	{
	    if ( notileft || notiright )
	    {
		if ( clickedidx_==itemidx && leftclicked_==notileft )
		    stagenr_ = 4;
	    }
	    return true;
	}
    }

    if ( !listcmdsflushed_ )
    {
	writeListSelect();

	const bool listbutchange = mIsSet(ischecked,clickedidx_) !=
	    			   mIsSet(waschecked,clickedidx_);

	if ( ev.dynamicpopup_ )
	{
	    if ( listbutchange )
		mRecOutStrm << "# ListButtonMenu: Command not yet implemented"
			    << std::endl;

	    writeListMenu( ev );
	}
	else if ( listbutchange )
	    writeListButton();
	else
	    writeListClick();

	listcmdsflushed_ = true;
    }

    if ( stagenr_ != 3 )
    {
	reInit();
	if ( accepted )
	    return accept( ev );
    }

    return accepted;
}


}; // namespace CmdDrive
