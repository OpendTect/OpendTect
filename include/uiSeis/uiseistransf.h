#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.13 2004-09-06 16:14:07 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class IOObj;
class Executor;
class uiGenInput;
class SeisSelData;
class uiSeisSubSel;
class SeisResampler;
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

			uiSeisTransfer(uiParent*,bool with_format,
					bool for_new_entry);

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
	    			   const char* executor_txt,
				   const char* work_txt) const;

    uiSeisSubSel*	selfld;
    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    void		setInput(const IOObj&);
    void		setSteering(bool);
    void		set2D(bool);
    void		getSelData(SeisSelData&) const;
    SeisResampler*	getResampler() const; //!< may return null

    int			maxBytesPerSample() const;

    bool		removeNull() const;

protected:

    bool		is2d;
    bool		issteer;

    void		updFldsForType(CallBacker*);

};


#endif
