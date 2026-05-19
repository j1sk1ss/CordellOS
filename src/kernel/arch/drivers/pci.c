#include <arch/drivers/pci.h>

static pci_dev_t _dev_zero         = { 0 };
static uint32_t _pci_size_map[100] = { 0 };

uint32_t pci_read(pci_dev_t dev, uint32_t field) {
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable = 1;
	i386_outl(PCI_CONFIG_ADDRESS, dev.bits);

	uint32_t size = _pci_size_map[field];
	if (size == 1) {
		uint8_t t = i386_inb(PCI_CONFIG_DATA + (field & 3));
		return t;
	}
	else if (size == 2) {
		uint16_t t = i386_inw(PCI_CONFIG_DATA + (field & 2));
		return t;
	}
	else if (size == 4) {
		uint32_t t = i386_inl(PCI_CONFIG_DATA);
		return t;
	}

	return 0xffff;
}

void pci_write(pci_dev_t dev, uint32_t field, uint32_t value) {
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable    = 1;
	
	i386_outl(PCI_CONFIG_ADDRESS, dev.bits);
	i386_outl(PCI_CONFIG_DATA, value);
}

static inline uint32_t _get_device_type(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_CLASS) << 8;
	return t | pci_read(dev, PCI_SUBCLASS);
}

static inline uint32_t _get_secondary_bus(pci_dev_t dev) {
	return pci_read(dev, PCI_SECONDARY_BUS);
}

static inline uint32_t _pci_reach_end(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_HEADER_TYPE);
	return !t;
}

static pci_dev_t _pci_scan_bus(uint16_t, uint16_t, uint32_t, int);
static pci_dev_t _pci_scan_function(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, uint32_t function, int device_type) {
	pci_dev_t dev    = { 0 };
	dev.bus_num      = bus;
	dev.device_num   = device;
	dev.function_num = function;

	// If it's a PCI Bridge device, get the bus it's connected to and keep searching
	if(_get_device_type(dev) == PCI_TYPE_BRIDGE) 
		_pci_scan_bus(vendor_id, device_id, _get_secondary_bus(dev), device_type);
		
	// If type matches, we've found the device, just return it
	if(device_type == -1 || device_type == _get_device_type(dev)) {
		uint32_t devid  = pci_read(dev, PCI_DEVICE_ID);
		uint32_t vendid = pci_read(dev, PCI_VENDOR_ID);
		if(devid == device_id && vendor_id == vendid)
			return dev;
	}

	return _dev_zero;
}

static pci_dev_t _pci_scan_device(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, int device_type) {
	pci_dev_t dev = {0};
	dev.bus_num = bus;
	dev.device_num = device;

	if (pci_read(dev,PCI_VENDOR_ID) == PCI_NONE)
		return _dev_zero;

	pci_dev_t t = _pci_scan_function(vendor_id, device_id, bus, device, 0, device_type);
	if (t.bits) return t;
	if (_pci_reach_end(dev)) return _dev_zero;

	for (int function = 1; function < FUNCTION_PER_DEVICE; function++) 
		if (pci_read(dev,PCI_VENDOR_ID) != PCI_NONE) {
			t = _pci_scan_function(vendor_id, device_id, bus, device, function, device_type);
			if (t.bits) return t;
		}

	return _dev_zero;
}

static pci_dev_t _pci_scan_bus(uint16_t vendor_id, uint16_t device_id, uint32_t bus, int device_type) {
	for (int device = 0; device < DEVICE_PER_BUS; device++) {
		pci_dev_t t = _pci_scan_device(vendor_id, device_id, bus, device, device_type);
		if (t.bits) return t;
	}

	return _dev_zero;
}

pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type) {
	pci_dev_t t = _pci_scan_bus(vendor_id, device_id, 0, device_type);
	if (t.bits) return t;

	if (_pci_reach_end(_dev_zero)) kprintf("[%s %i] PCI GET DEVICE FAIL!\n", __FILE__, __LINE__);
	for (int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		pci_dev_t dev = {0};
		dev.function_num = function;

		if (pci_read(dev, PCI_VENDOR_ID) == PCI_NONE) break;
		t = _pci_scan_bus(vendor_id, device_id, function, device_type);
		if (t.bits) return t;
	}

	return _dev_zero;
}

void i386_pci_init() {
	_pci_size_map[PCI_VENDOR_ID]       = 2;
	_pci_size_map[PCI_DEVICE_ID]       = 2;
	_pci_size_map[PCI_COMMAND]	       = 2;
	_pci_size_map[PCI_STATUS]	       = 2;
	_pci_size_map[PCI_SUBCLASS]	       = 1;
	_pci_size_map[PCI_CLASS]		   = 1;
	_pci_size_map[PCI_CACHE_LINE_SIZE] = 1;
	_pci_size_map[PCI_LATENCY_TIMER]   = 1;
	_pci_size_map[PCI_HEADER_TYPE]     = 1;
	_pci_size_map[PCI_BIST]            = 1;
	_pci_size_map[PCI_BAR0]            = 4;
	_pci_size_map[PCI_BAR1]            = 4;
	_pci_size_map[PCI_BAR2]            = 4;
	_pci_size_map[PCI_BAR3]            = 4;
	_pci_size_map[PCI_BAR4]            = 4;
	_pci_size_map[PCI_BAR5]            = 4;
	_pci_size_map[PCI_INTERRUPT_LINE]  = 1;
	_pci_size_map[PCI_SECONDARY_BUS]   = 1;
}