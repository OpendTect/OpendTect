
#ifndef uimapperrangeeditordlg_h
#define uimapperrangeeditordlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "datapack.h"

class uiPushButton;
class uiMapperRangeEditor;
namespace ColTab { class MapperSetup; class Sequence; };

mExpClass(uiTools) uiMultiMapperRangeEditWin : public uiDialog
{
public:
    					uiMultiMapperRangeEditWin(uiParent*,
						int n,DataPackMgr::ID dmid);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int,DataPack::ID);
    void				setColTabMapperSetup(int,
	    					const ColTab::MapperSetup&);
    void				setColTabSeq(int,
	    					const ColTab::Sequence&);
    int					activeAttrbID()
    					{ return activeattrbid_; }
    const ColTab::MapperSetup&		activeMapperSetup()
    					{ return *activectbmapper_; }
    Notifier<uiMultiMapperRangeEditWin> 	rangeChange;

protected:

    uiPushButton*			statbut_;
    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    int					activeattrbid_;
    const ColTab::MapperSetup*        	activectbmapper_;
    DataPackMgr&                	dpm_; 
    TypeSet<DataPack::ID>   		datapackids_;
    
    void				rangeChanged(CallBacker*);   
    void				showStatDlg(CallBacker*);   
    void				dataPackDeleted(CallBacker*); 
};

#endif

