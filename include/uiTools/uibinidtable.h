/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id: uibinidtable.h,v 1.2 2003-03-12 16:23:30 arend Exp $
 ________________________________________________________________________

-*/

#include "uigroup.h"
#include "uidialog.h"

class uiTable;
class BinID;


class uiBinIDTable : public uiGroup
{
public:
			uiBinIDTable(uiParent*,int);
			uiBinIDTable(uiParent*,const TypeSet<BinID>&);

    void		setBinIDs(const TypeSet<BinID>&);
    void		getBinIDs(TypeSet<BinID>&);

protected:

    uiTable*		table;
			
    void		init(int);
    void		nodeAdded(CallBacker* cb=0);

};



class uiBinIDTableDlg : public uiDialog
{
public:
			uiBinIDTableDlg(uiParent*,const char*,int);
			uiBinIDTableDlg(uiParent*,const char*,
					const TypeSet<BinID>&);

    void                setBinIDs(const TypeSet<BinID>&);
    void                getBinIDs(TypeSet<BinID>&);

protected:

    uiBinIDTable*	table;
};
