#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.11 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class IOObj;
class Executor;
class SeisSelection;
class uiGenInput;
class uiBinIDSubSel;
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

			uiSeisTransfer(uiParent*,bool with_format);

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const SeisSelection& from,const IOObj* to,
	    			   const char* executor_txt,
				   const char* work_txt) const;

    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    void		setSteering(bool);
    void		set2D(bool);

    int			maxBytesPerSample() const;

    bool		removeNull() const;

protected:

    bool		is2d;
    bool		issteer;

    void		updFldsForType(CallBacker*);

};


#endif
