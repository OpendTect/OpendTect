#ifndef uiattrgetfile_h
#define uiattrgetfile_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uiattrgetfile.h,v 1.5 2012-08-03 13:00:47 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

class uiFileInput;
class uiTextEdit;
class uiFileInput;
namespace Attrib { class DescSet; }


mClass(uiAttributes) uiGetFileForAttrSet : public uiDialog
{
public:
			uiGetFileForAttrSet(uiParent*,bool isads,bool is2d);
			~uiGetFileForAttrSet();

    const char*		fileName() const		{ return fname_; }
    Attrib::DescSet&	attrSet()			{ return attrset_; }

protected:

    uiFileInput*	fileinpfld;
    uiTextEdit*		infofld;
    BufferString	fname_;
    Attrib::DescSet&	attrset_;
    bool		isattrset_;

    void		srchDir(CallBacker*);
    void		selChg(CallBacker* =0);
    bool		acceptOK(CallBacker*);

};


#endif

