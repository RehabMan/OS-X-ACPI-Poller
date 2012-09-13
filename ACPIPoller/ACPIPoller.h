/*
 * Copyright (c) 2012 RehabMan. All rights reserved.
 *
 * TODO: Include information about open source license here.
 *
 */

#ifndef __ACPIPoller__
#define __ACPIPoller__

#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOTimerEventSource.h>

#ifdef DEBUG_MSG
#define DEBUG_LOG(args...)  IOLog(args)
#else
#define DEBUG_LOG(args...)
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

class ACPIPoller : public IOService
{
	OSDeclareDefaultStructors(ACPIPoller)

public:
	virtual bool init(OSDictionary *dictionary = 0);
    virtual void free(void);
    virtual IOService *probe(IOService *provider, SInt32 *score);
    bool start(IOService *provider);
	void stop(IOService *provider);

private:
	IOACPIPlatformDevice*   m_pDevice;
    IOWorkLoop*             m_pWorkLoop;
	IOTimerEventSource*     m_pTimer;
    bool                    m_fInTimer;
    OSArray*                m_pMethods;
    
    IOReturn OnTimerEvent(void);
};

#endif // __ACPIPoller__