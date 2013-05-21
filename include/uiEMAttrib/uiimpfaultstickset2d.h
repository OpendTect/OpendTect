#ifndef uiimpfaultstickset2d_h
#define uiimpfaultstickset2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2009
 RCS:           
________________________________________________________________________
-*/

#include "uiemattribmod.h"
#include "uiimpfault.h"
#include "emposid.h"

class BufferStringSet;
class MultiID;
class uiComboBox;

/*Brief Dialog for 2D FaultStickSet*/

mExpClass(uiEMAttrib) uiImportFaultStickSet2D : public uiImportFault
{
public:
    			uiImportFaultStickSet2D(uiParent*,const char* type);

protected:
    bool		acceptOK(CallBacker*);
    bool		getFromAscIO(std::istream&,EM::Fault&);

    uiComboBox*		linesetfld_;
    BufferStringSet&	linesetnms_;
    TypeSet<MultiID>	setids_;
};

#endif

