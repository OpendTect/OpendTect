#ifndef uitblimpexpdatasel_h
#define uitblimpexpdatasel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.h,v 1.2 2006-12-14 18:34:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiGenInput;
class uiTableImpDataSelElem;
namespace Table { class TargetInfo; class FormatDesc; }

/*!\brief Table-based data import selection

  This class is meant to accept data structures describing table import/export
  as defined in General/tabledef.h. Resulting FormatDesc's selections can be
  used by a descendent of Table::AscIO.

  For example, the Wavelet import dialog creates a Table::FormatDesc object
  using WaveletAscIO::getDesc(), this class lets the user fill the FormatDesc's
  selection_, after which WaveletAscIO creates the imported object.
 
 */

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

    uiGroup*			mkElemFlds(ObjectSet<Table::TargetInfo>&,
	    				   ObjectSet<uiTableImpDataSelElem>&,
					   bool);

};


#endif
