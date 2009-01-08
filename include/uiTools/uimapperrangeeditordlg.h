
#ifndef uimultirangeseldisplaywin_h
#define uimultirangeseldisplaywin_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.h,v 1.2 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "datapack.h"

class uiMapperRangeEditor;
namespace ColTab { struct MapperSetup; class Sequence; };

mClass uiMultiRangeSelDispWin : public uiDialog
{
public:
    					uiMultiRangeSelDispWin(uiParent*,int n);

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int,DataPack::ID,
	    					      DataPackMgr::ID);
    void				setColTabMapperSetupWthSeq(int,
	    					     const ColTab::MapperSetup&,
						     const ColTab::Sequence&);
    int					activeAttrbID()
    					{ return activeattrbid_; }
    const ColTab::MapperSetup&		activeMapperSetup()
    					{ return *activectbmapper_; }
    Notifier<uiMultiRangeSelDispWin> 	rangeChange;

protected:

    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    int					activeattrbid_;
    const ColTab::MapperSetup*        	activectbmapper_; 
    
    void				rangeChanged(CallBacker*);    
};

#endif
