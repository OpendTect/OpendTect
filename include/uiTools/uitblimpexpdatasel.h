#ifndef uitblimpexpdatasel_h
#define uitblimpexpdatasel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.h,v 1.1 2006-10-30 17:03:50 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiGenInput;
class uiTableImpDataSelElem;
namespace Table { class FormatInfo; class FormatDesc; }

/*!\brief Table-based data import selection */

class uiTableImpDataSel : public uiGroup
{
public:
    				uiTableImpDataSel(uiParent*,Table::FormatDesc&);

    bool			commit();
    const char*			errMsg() const		{ return errmsg_; }

protected:

    Table::FormatDesc&		fd_;
    uiGenInput*			hdrendfld;
    ObjectSet<uiTableImpDataSelElem> hdrelems_;
    ObjectSet<uiTableImpDataSelElem> bodyelems_;
    const char*			errmsg_;

    uiGroup*			mkElemFlds(ObjectSet<Table::FormatInfo>&,
	    				   ObjectSet<uiTableImpDataSelElem>&,
					   bool);

};


#endif
