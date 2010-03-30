#ifndef uiemauxdatasel_h
#define uiemauxdatasel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiemauxdatasel.h,v 1.1 2010-03-30 20:56:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiiosurface.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class uiGenInput;
class uiIOObjSel;
class uiSurfaceRead;
class uiSurfaceWrite;

namespace EM { class Surface; class SurfaceIODataSelection; class Horizon3D; }


/*! \brief Dialog for surface aux data selection. */

mClass uiEMAuxDataSel : public uiGroup
{
public:
			uiEMAuxDataSel(uiParent*,const char* label,
				    const MultiID* =0, const char* auxdata=0 );
    const MultiID&	getSurfaceID() const;
    const char*		getAuxDataSel() const;

protected:

    uiPushButton*	selectbutton_;
    uiLabel*		label_;
    uiGenInput*		textfld_;
};

#endif
