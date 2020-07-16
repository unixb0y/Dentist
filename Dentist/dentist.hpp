//
//  dentist.hpp
//  Dentist
//
//  Copyright © 2020 unixb0y. All rights reserved.
//

#ifndef dentist_hpp
#define dentist_hpp

#include <sys/kernel_types.h>
#include <sys/kern_control.h>

#include <Headers/kern_patcher.hpp>

#define kThisKextID "com.unixb0y.Dentist"

struct ClientInfo {
    u_int32_t sc_unit = 0;
    bool connected = false;
};

class Connector {
public:
    Connector() = default;
    
    void init();
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
    
    static errno_t ctlHandleSet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t len);
    static errno_t ctlHandleGet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t *len);
    
    static errno_t ctlHandleConnect(kern_ctl_ref ctlref, struct sockaddr_ctl *sac, void **unitinfo);
    static errno_t ctlHandleDisconnect(kern_ctl_ref ctlref, unsigned int unit, void *unitinfo);
    
    static errno_t ctlHandleWrite(kern_ctl_ref ctlref, unsigned int unit, void *userdata, mbuf_t m, int flags);

private:
    bool mDone = false;
    kern_ctl_reg mCtl;
    kern_ctl_ref mCtlref;
    
    ClientInfo mClient;    // client from userspace
    bool mEnabled = false; // enabled
    
    static Connector *instance;
    void *ioBTInstance = nullptr;
    
    /**
     *  Hooked methods / callbacks
     */
    static int64_t IOBluetoothHostControllerUSBTransport_SendHCIRequest(void *that, uint8_t *hciData, uint64_t a3);
    static int64_t IOBluetoothHostControllerUSBTransport_ReceiveInterruptData(void *that, uint8_t *data, uint64_t len, bool unknown);

    /**
     *  Original methods
     */
    mach_vm_address_t orgIOBluetoothHostControllerUSBTransport_SendHCIRequest {};
    mach_vm_address_t orgIOBluetoothHostControllerUSBTransport_ReceiveInterruptData {};
    mach_vm_address_t orgIOBluetoothHostControllerUSBTransport_HandleMessage {};
};

#endif /* dentist_hpp */
