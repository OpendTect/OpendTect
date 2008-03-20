#ifndef emsurfacetr_h
#define emsurfacetr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfacetr.h,v 1.10 2008-03-20 21:36:32 cvskris Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM
{
    class dgbSurfaceReader;
    class Fault;
    class Horizon3D;
    class Horizon2D;
    class Horizon;
    class Surface;
}

typedef EM::Horizon3D 	EMHorizon3D;
typedef EM::Horizon2D	EMHorizon2D;
typedef EM::Horizon	EMAnyHorizon;
typedef EM::Fault	EMFault;


/*!\brief Read/write EM::Horizon to storage */

class EMHorizon3DTranslatorGroup : public TranslatorGroup
{				   isTranslatorGroup(EMHorizon3D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon3D)

    const char*			defExtension() const	{ return "hor"; }

    static const char*		keyword;
};


class EMHorizon2DTranslatorGroup : public TranslatorGroup
{				   isTranslatorGroup(EMHorizon2D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon2D)

    const char*			defExtension() const	{ return "2dh"; }

    static const char*		keyword;
};


class EMAnyHorizonTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(EMAnyHorizon)
public:
			    mDefEmptyTranslatorGroupConstructor(EMAnyHorizon)

static const char*	    keyword;
};


class EMFaultTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(EMFault)
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
    Executor*			writer(const IOObj&,bool fullimplremove=true);
				/*!< Executor is managed by client. */ 

    const char*			errMsg() const		{ return errmsg_.buf();}

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    				   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    static bool			getBinarySetting();

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


class dgbEMHorizon3DTranslator : public dgbEMSurfaceTranslator
{				 isTranslator(dgb,EMHorizon3D)
public:
    				dgbEMHorizon3DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon3DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }
};


class dgbEMHorizon2DTranslator : public dgbEMSurfaceTranslator
{				isTranslator(dgb,EMHorizon2D)
public:
    				dgbEMHorizon2DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon2DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{return false;}
    virtual bool		writeOnlyZ() const		{return true;}
    virtual bool		hasRangeSelection() const	{return false;}
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
