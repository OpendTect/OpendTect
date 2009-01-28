
#ifndef uimapperrangeeditordlg_h
#define uimapperrangeeditordlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.h,v 1.5 2009-01-28 09:09:21 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "datapack.h"

class uiMapperRangeEditor;
namespace ColTab { struct MapperSetup; class Sequence; };

mClass uiMultiMapperRangeEditWin : public uiDialog
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

    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    int					activeattrbid_;
    const ColTab::MapperSetup*        	activectbmapper_;
    DataPackMgr&                	dpm_; 
    TypeSet<DataPack::ID>   		datapackids_;
    
    void				rangeChanged(CallBacker*);   
    void				dataPackDeleted(CallBacker*); 
};

#endif
