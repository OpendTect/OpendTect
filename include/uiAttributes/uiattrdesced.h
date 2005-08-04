#ifndef uiattrdesced_h
#define uiattrdesced_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uiattrdesced.h,v 1.3 2005-08-04 14:27:41 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "changetracker.h"
#include "paramsetget.h"

namespace Attrib { class Desc; class DescSet; class DescSetMan; };

class uiAttrDescEd;
class uiAttrSel;
class uiAttrSelData;
class uiImagAttrSel;
class uiSteeringSel;
class uiSteerCubeSel;


/*! \brief Attribute description editor creater */

class uiAttrDescEdCreater
{
public:

    virtual			~uiAttrDescEdCreater()		{}
    virtual uiAttrDescEd*	create(uiParent*) const		= 0;

};


/*! \brief Attribute description editor */

class uiAttrDescEd : public uiGroup
{
public:

    virtual		~uiAttrDescEd();

    void		setDesc(Attrib::Desc*,Attrib::DescSetMan*);
    Attrib::Desc*	desc()			{ return attrdesc; }
    virtual const char*	commit(Attrib::Desc* desc=0);
			//!< returns null on success, error message otherwise
    			//!< If attribdesc is non-zero, that desc will be
    			//!< filled. If not, the internal desc will be filled.

    virtual bool	shouldEdit( const char* s ) const
			{ return name() == s; }
    virtual bool	canEdit( const char* ad ) const
			{ return shouldEdit(ad); }
    virtual void	set2D( bool yn )		{}

    virtual int		getOutputIdx(float val) const	{ return (int)val; }
    virtual float	getOutputValue(int idx) const	{ return (float)idx; }
    virtual void	setOutputStep(float step)	{}

    virtual const char* getDisplayName() const 		{ return 0; }
    virtual bool	useIfZIsTime() const		{ return true; }
    virtual bool	useIfZIsDepth() const		{ return true; }

    static const char*	timegatestr;
    static const char*	frequencystr;
    static const char*	stepoutstr;
    static const char*	filterszstr;

protected:

			uiAttrDescEd(uiParent*);

    virtual bool	setParameters(const Attrib::Desc&)	{ return true; }
    virtual bool	getParameters(Attrib::Desc&)		{ return true; }
    virtual bool	setInput(const Attrib::Desc&)		{ return true; }
    virtual bool	getInput(Attrib::Desc&)			{ return true; }
    virtual bool	setOutput(const Attrib::Desc&)		{ return true; }
    virtual bool	getOutput(Attrib::Desc&);

    void		fillOutput(Attrib::Desc&,int selout);
    void		fillInp(uiAttrSel*,Attrib::Desc&,int inpnr);
    void		fillInp(uiSteeringSel*,Attrib::Desc&,int inpnr);
    void		fillInp(uiSteerCubeSel*,Attrib::Desc&,int inpnr);
    
    void		putInp(uiAttrSel*,const Attrib::Desc&,int inpnr);
    void		putInp(uiSteerCubeSel*,const Attrib::Desc&,int inpnr);
    void		putInp(uiSteeringSel*,const Attrib::Desc&,int inpnr);

    BufferString	gateLabel() const;
    BufferString	shiftLabel() const;
    bool		zIsTime() const;

    ChangeTracker	chtr;
    uiAttrSel*		getInpFld(const char* txt=0,const uiAttrSelData* =0);
    uiImagAttrSel*	getImagInpFld();
    void		attrInpSel(CallBacker*);

private:

    Attrib::Desc*	attrdesc;
    Attrib::DescSetMan*	adsman;

};


#endif
