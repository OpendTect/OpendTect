#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.3 2002-11-01 09:48:06 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class IOObj;
class Executor;
class uiGenInput;
class uiBinIDSubSel;
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

			uiSeisTransfer(uiParent*,bool with_format,
				       bool with_steps=true);

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj* from,const IOObj* to,
	    			   const char* executor_txt,
				   const char* work_txt) const;

    uiBinIDSubSel*	subselfld;
    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    bool		provideUserInfo(const IOObj&) const;
};


#endif
