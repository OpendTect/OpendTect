#ifndef emsurfacetr_h
#define emsurfacetr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfacetr.h,v 1.1 2004-07-14 15:33:59 nanne Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "emhorizon.h"
#include "emfault.h"
#include "emsurfaceiodata.h"

namespace EM { class dgbSurfaceReader; };

typedef EM::Horizon EMHorizon;
typedef EM::Fault EMFault;


/*!\brief Read/write EM::Horizon to storage */

class EMHorizonTranslatorGroup : public TranslatorGroup
{				 isTranslatorGroup(EMHorizon)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon)

    const char*			defExtension() const	{ return "hor"; }

    static const char*		keyword;
};


class EMFaultTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(EMFault)
public:
    			mDefEmptyTranslatorGroupConstructor(EMFault)

    const char*		defExtension() const { return "flt"; }

    static const char*	keyword;

};



class EMSurfaceTranslator : public Translator
{
public:
    				EMSurfaceTranslator(const char* nm,
						    const char* unm)
				    : Translator(nm,unm)
				    , ioobj_(0)
				    , surface(0)
				    , sels_(sd_)	{}

    virtual			~EMSurfaceTranslator();

    bool			startRead(const IOObj&);
    bool			startWrite(const EM::Surface&);

    EM::SurfaceIODataSelection& selections()		{ return sels_; }

    virtual Executor*		reader(EM::Surface&)	{ return 0; }
				/*!< Executor is managed by client. */
    Executor*			writer(const IOObj&);
				/*!< Executor is managed by client. */ 

    const char*			errMsg() const		{ return errmsg_; }

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    				   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

protected:

    IOObj*			ioobj_;
    EM::Surface*		surface;
    BufferString		errmsg_;
    EM::SurfaceIOData		sd_;
    EM::SurfaceIODataSelection	sels_;

    virtual bool		prepRead()		{ return true; }
    virtual bool		prepWrite()		{ return true; }
    virtual Executor*		getWriter()		{ return 0; }

    void			init(const EM::Surface*,const IOObj*);
    void			setIOObj(const IOObj*);
};


class dgbEMSurfaceTranslator : public EMSurfaceTranslator
{
public:
    				dgbEMSurfaceTranslator(const char*,const char*);
    virtual			~dgbEMSurfaceTranslator();

    virtual Executor*		reader(EM::Surface&);

protected:

    static BufferString		createHOVName(const char* base,int idx);
    bool			setSurfaceTransform(const IOPar&);
    bool			binary;
    void			getSels(StepInterval<int>&,StepInterval<int>&);

    bool			prepRead();
    Executor*			getWriter();

    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }

    EM::dgbSurfaceReader*	reader_;
};


class dgbEMHorizonTranslator : public dgbEMSurfaceTranslator
{				isTranslator(dgb,EMHorizon)
public:
    				dgbEMHorizonTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizonTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }
};


class dgbEMFaultTranslator : public dgbEMSurfaceTranslator
{				isTranslator(dgb,EMFault)
public:
    				dgbEMFaultTranslator(const char* unm,
						     const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMFaultTranslator()			{}

protected:
    virtual bool		readOnlyZ() const		{ return false;}
    virtual bool		writeOnlyZ() const		{ return false;}
    virtual bool		hasRangeSelection() const	{ return false;}
};

#endif
