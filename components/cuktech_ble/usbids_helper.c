
#include <stdlib.h> //for NULL, bsearch
#include "usbids.h"

static UsbDevStruct UsbList[] = {
	{ 0x03f0, 0xFFFF, "HP", "Notebook" },

	{ 0x04e8, 0x681c, "Samsung Mobile", "Galaxy Portal/Spica/S" },
	{ 0x04e8, 0x681d, "Samsung Mobile", "Galaxy Portal/Spica Android Phone" },
	{ 0x04e8, 0x6843, "Samsung Mobile", "Kies mode" },
	{ 0x04e8, 0x684e, "Samsung Mobile", "Wave (GT-S8500)" },
	{ 0x04e8, 0x685b, "Samsung Mobile", "Phone (mass storage mode)" },
	{ 0x04e8, 0x685c, "Samsung Mobile", "Phone (Mass storage mode)" },
	{ 0x04e8, 0x685d, "Samsung Mobile", "Phone (Download mode)" },
	{ 0x04e8, 0x685e, "Samsung Mobile", "Phone (USB Debugging mode)" },
	{ 0x04e8, 0x6860, "Samsung Mobile", "Galaxy (MTP)" },
	{ 0x04e8, 0x6863, "Samsung Mobile", "Galaxy (network tethering)" },
	{ 0x04e8, 0x6864, "Samsung Mobile", "Galaxy (network tethering, USB debugging enabled)" },
	{ 0x04e8, 0x6865, "Samsung Mobile", "Galaxy (PTP mode)" },
	{ 0x04e8, 0x6866, "Samsung Mobile", "Galaxy (debugging mode)" },
	{ 0x04e8, 0x6868, "Samsung Mobile", "Escape Composite driver" },
	{ 0x04e8, 0x6877, "Samsung Mobile", "Galaxy S" },
	{ 0x04e8, 0x687a, "Samsung Mobile", "GT-E2370 mobile phone" },
	{ 0x04e8, 0x6888, "Samsung Mobile", "GT-B3730 Composite LTE device (Commercial)" },
	{ 0x04e8, 0x6889, "Samsung Mobile", "GT-B3730 Composite LTE device (Commercial)" },
	{ 0x04e8, 0x689a, "Samsung Mobile", "LTE Storage Driver [CMC2xx]" },
	{ 0x04e8, 0x689e, "Samsung Mobile", "GT-S5670 [Galaxy Fit]" },
	{ 0x04e8, 0xFFFF, "Samsung Mobile", "Phone" },

	{ 0x045e, 0xFFFF, "Microsoft", "Notebook" },

	{ 0x057e, 0x2000, "Nintendo", "Switch" },
	{ 0x057e, 0x2060, "Nintendo", "Switch 2" },
	{ 0x057e, 0xFFFF, "Nintendo", "Device" },

	{ 0x05ac, 0x1290, "Apple", "iPhone" },
	{ 0x05ac, 0x12a8, "Apple", "iPhone" },
	{ 0x05ac, 0x12ab, "Apple", "iPhone" },
	{ 0x05ac, 0x13a1, "Apple", "AirPods Charging Case" },
	{ 0x05ac, 0x7102, "Apple", "iPad Pro" },
	{ 0x05ac, 0x7108, "Apple", "iPad Air" },
	{ 0x05ac, 0x7109, "Apple", "iPad Pro" },
	{ 0x05ac, 0x710c, "Apple", "iPad mini" },
	{ 0x05ac, 0x710d, "Apple", "iPad Air" },
	{ 0x05ac, 0x710e, "Apple", "iPad Pro" },
	{ 0x05ac, 0x7111, "Apple", "iPad" },
	{ 0x05ac, 0x7112, "Apple", "iPad" },
	{ 0x05ac, 0x7114, "Apple", "iPad Air" },
	{ 0x05ac, 0x7115, "Apple", "iPad Air" },
	{ 0x05ac, 0x7116, "Apple", "iPad mini" },
	{ 0x05ac, 0x7117, "Apple", "iPad Pro" },
	{ 0x05ac, 0x711c, "Apple", "iPad Air" },
	{ 0x05ac, 0x711d, "Apple", "iPad Air" },
	{ 0x05ac, 0x711e, "Apple", "iPad Pro" },
	{ 0x05ac, 0x711b, "Apple", "iPad" },
	{ 0x05ac, 0x7302, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7304, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7306, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7307, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7308, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7309, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x730b, "Apple", "MacBook Air" },
	{ 0x05ac, 0x730c, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x730d, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x730e, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7310, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7312, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7313, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7314, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7318, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7315, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x7319, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x731a, "Apple", "MacBook Pro" },
	{ 0x05ac, 0x731b, "Apple", "MacBook Air" },
	{ 0x05ac, 0x731c, "Apple", "MacBook Air" },
	{ 0x05ac, 0x7504, "Apple", "iPhone 12" },
	{ 0x05ac, 0x7505, "Apple", "iPhone 12 Pro Max" },
	{ 0x05ac, 0x7506, "Apple", "iPhone 12 Pro" },
	{ 0x05ac, 0x7507, "Apple", "iPhone 13" },
	{ 0x05ac, 0x7509, "Apple", "iPhone 13 Pro Max" },
	{ 0x05ac, 0x750a, "Apple", "iPhone 13 Pro" },
	{ 0x05ac, 0x750b, "Apple", "iPhone SE 3" },
	{ 0x05ac, 0x750d, "Apple", "iPhone 14" },
	{ 0x05ac, 0x750e, "Apple", "iPhone 14 Pro Max" },
	{ 0x05ac, 0x750f, "Apple", "iPhone 14 Pro" },
	{ 0x05ac, 0x7510, "Apple", "iPhone 15" },
	{ 0x05ac, 0x7511, "Apple", "iPhone 15 Plus" },
	{ 0x05ac, 0x7513, "Apple", "iPhone 15 Pro Max" },
	{ 0x05ac, 0x7514, "Apple", "iPhone 15 Pro" },
	{ 0x05ac, 0x7515, "Apple", "iPhone 16 Pro" },
	{ 0x05ac, 0x7516, "Apple", "iPhone 16 Plus" },
	{ 0x05ac, 0x7517, "Apple", "iPhone 16 Pro Max" },
	{ 0x05ac, 0x7518, "Apple", "iPhone 16 Pro" },
	{ 0x05ac, 0x7519, "Apple", "iPhone 17 Pro Max" },
	{ 0x05ac, 0x751a, "Apple", "iPhone 17 Pro" },
	{ 0x05ac, 0x751b, "Apple", "iPhone 17" },
	{ 0x05ac, 0x751c, "Apple", "iPhone 16e" },
	{ 0x05ac, 0x751f, "Apple", "iPhone 18 Pro Max" },
	{ 0x05ac, 0xFFFF, "Apple", "iPhone" },

	{ 0x0b05, 0x550f, "ASUS", "Fonepad 7" },
	{ 0x0b05, 0x6101, "ASUS", "Cable Modem" },
	{ 0x0b05, 0x620a, "ASUS", "Remote NDIS Device" },
	{ 0x0b05, 0x7772, "ASUS", "Zenfone (MTP mode)" },
	{ 0x0b05, 0x7773, "ASUS", "Zenfone (Debug, MTP mode)" },
	{ 0x0b05, 0x7774, "ASUS", "Zenfone (RNDIS mode)" },
	{ 0x0b05, 0x7775, "ASUS", "Zenfone (Debug, RNDIS mode)" },
	{ 0x0b05, 0x7776, "ASUS", "Zenfone (PTP mode)" },
	{ 0x0b05, 0x7777, "ASUS", "Zenfone (Debug, PTP mode)" },
	{ 0x0b05, 0xFFFF, "ASUS", "Phone" },

	{ 0x0bb4, 0x0fff, "HTC", "Fastboot Bootloader" },
	{ 0x0bb4, 0x2008, "HTC", "Phone via MTP" },
	{ 0x0bb4, 0x200b, "HTC", "Phone via PTP" },
	{ 0x0bb4, 0xFFFF, "HTC", "Phone" },

	{ 0x0fce, 0x01ba, "Sony", "D6603 [Xperia Z3]" },
	{ 0x0fce, 0x01bb, "Sony", "D5803 [Xperia Z3 Compact] (MTP mode)" },
	{ 0x0fce, 0x0dde, "Sony", "Xperia Mini Pro Bootloader" },
	{ 0x0fce, 0x2137, "Sony", "Xperia X10 mini (USB debug)" },
	{ 0x0fce, 0x2138, "Sony", "Xperia X10 mini pro (Debug)" },
	{ 0x0fce, 0x2149, "Sony", "Xperia X8 (debug)" },
	{ 0x0fce, 0x3137, "Sony", "Xperia X10 mini" },
	{ 0x0fce, 0x3138, "Sony", "Xperia X10 mini pro" },
	{ 0x0fce, 0x3149, "Sony", "Xperia X8" },
	{ 0x0fce, 0x514f, "Sony", "Xperia arc S [Adb-Enable Mode]" },
	{ 0x0fce, 0x5169, "Sony", "Xperia S [Adb-Enable Mode]" },
	{ 0x0fce, 0x5177, "Sony", "Xperia Ion [Debug Mode]" },
	{ 0x0fce, 0x518c, "Sony", "C1605 [Xperia E dual] MTD mode" },
	{ 0x0fce, 0x51a7, "Sony", "D5503 (Xperia Z1 Compact)" },
	{ 0x0fce, 0x614f, "Sony", "Xperia X12 (debug mode)" },
	{ 0x0fce, 0x6166, "Sony", "Xperia Mini Pro" },
	{ 0x0fce, 0x618c, "Sony", "C1605 [Xperia E dual] MSC mode" },
	{ 0x0fce, 0x715a, "Sony", "Xperia Pro [Tethering]" },
	{ 0x0fce, 0x7166, "Sony", "Xperia Mini Pro (Tethering mode)" },
	{ 0x0fce, 0x7177, "Sony", "Xperia Ion [Tethering]" },
	{ 0x0fce, 0xe14f, "Sony", "Xperia Arc/X12" },
	{ 0x0fce, 0xe15a, "Sony", "Xperia Pro [Mass Storage Class]" },
	{ 0x0fce, 0xe161, "Sony", "Xperia Ray" },
	{ 0x0fce, 0xe166, "Sony", "Xperia Mini Pro" },
	{ 0x0fce, 0xe167, "Sony", "XPERIA mini" },
	{ 0x0fce, 0xFFFF, "Sony", "Phone" },

	{ 0x1004, 0x6356, "LG Mobile", "Optimus Android Phone [Virtual CD mode]" },
	{ 0x1004, 0x91c8, "LG Mobile", "P880 / USB tethering" },
	{ 0x1004, 0xFFFF, "LG Mobile", "PHone" },

	{ 0x12d1, 0x380b, "Huawei", "WiMAX USB modem(s)" },
	{ 0x12d1, 0xFFFF, "Huawei", "Phone" },

	{ 0x17ef, 0x7423, "Lenovo", "IdeaPad A1 Tablet" },
	{ 0x17ef, 0x7435, "Lenovo", "A789 (Mass Storage mode, with debug)" },
	{ 0x17ef, 0x743a, "Lenovo", "A789 (Mass Storage mode)" },
	{ 0x17ef, 0x7497, "Lenovo", "A789 (MTP mode)" },
	{ 0x17ef, 0x7498, "Lenovo", "A789 (MTP mode, with debug)" },
	{ 0x17ef, 0x749a, "Lenovo", "A789 (PTP mode)" },
	{ 0x17ef, 0x749b, "Lenovo", "A789 (PTP mode, with debug)" },
	{ 0x17ef, 0xa37f, "Lenovo", "ThinkPad" },
	{ 0x17ef, 0xFFFF, "Lenovo", "Phone" },

	{ 0x18d1, 0x2d00, "Google", "Accessory mode" },
	{ 0x18d1, 0x2d01, "Google", "Accessory mode with ADB support" },
	{ 0x18d1, 0x4ee1, "Google", "Nexus Device (MTP)" },
	{ 0x18d1, 0x4ee2, "Google", "Nexus Device (debug)" },
	{ 0x18d1, 0x4ee3, "Google", "Nexus Device (tether)" },
	{ 0x18d1, 0x4ee4, "Google", "Nexus Device (debug + tether)" },
	{ 0x18d1, 0x4ee5, "Google", "Nexus Device (PTP)" },
	{ 0x18d1, 0x4ee6, "Google", "Nexus Device (PTP + debug)" },
	{ 0x18d1, 0x7102, "Google", "Toshiba Thrive tablet" },
	{ 0x18d1, 0xb004, "Google", "Pandigital / B&N Novel 9\" tablet" },
	{ 0x18d1, 0xd001, "Google", "Nexus Device (fastboot)" },
	{ 0x18d1, 0xd002, "Google", "Nexus Device (debug)" },
	{ 0x18d1, 0xd109, "Google", "LG G2x MTP" },
	{ 0x18d1, 0xd10a, "Google", "LG G2x MTP (debug)" },
	{ 0x18d1, 0xFFFF, "Google", "Phone" },

	{ 0x19d2, 0x1008, "ZTE Mobile", "K3570-Z" },
	{ 0x19d2, 0x1010, "ZTE Mobile", "K3571-Z" },
	{ 0x19d2, 0x1217, "ZTE Mobile", "MF652" },
	{ 0x19d2, 0x1218, "ZTE Mobile", "MF652" },
	{ 0x19d2, 0x2000, "ZTE Mobile", "MF627/MF628/MF628+/MF636+ HSDPA/HSUPA" },
	{ 0x19d2, 0xFFFF, "ZTE Mobile", "Phone" },

	{ 0x1d68, 0xFFFF, "Linux Foundation", "Notebook" },

	{ 0x2109, 0x0103, "VIA", "USB Hub" },
	{ 0x2109, 0xFFFF, "VIA", "Notebook" },

	{ 0x22b8, 0x6415, "Motorola", "ROKR Z6 (MTP mode)" },
	{ 0x22b8, 0x6604, "Motorola", "Washington CDMA Phone" },
	{ 0x22b8, 0x6631, "Motorola", "CDC Modem" },
	{ 0x22b8, 0x7001, "Motorola", "Q Smartphone" },
	{ 0x22b8, 0xFFFF, "Motorola", "Phone" },

	{ 0x291a, 0x110a, "Anker", "Anker Prime Power Bank 26K" },
	{ 0x291a, 0x110b, "Anker", "Anker Prime Power Bank 20K" },
	{ 0x291a, 0x1665, "Anker", "Anker Power Bank 5K" },
	{ 0x291a, 0x1669, "Anker", "Anker Power Bank 10K" },
	{ 0x291a, 0x1340, "Anker", "Anker Power Bank 27K" },
	{ 0x291a, 0x25f7, "Anker", "Anker Wireless Charger 35W" },
	{ 0x291a, 0xFFFF, "Anker", "Power Bank" },

	{ 0x29d9, 0xFFFF, "OPPO", "Phone" },

	{ 0x29cf, 0x6375, "POCO", "X4 GT" },
	{ 0x29cf, 0xFFFF, "POCO", "Phone" },

	{ 0x2717, 0x209e, "Xiaomi", "Phone (MTP)" },
	{ 0x2717, 0xFFFF, "Xiaomi", "Phone" },

	{ 0x2a45, 0x0001, "Meizu Corp.", "MX Phone (BICR)" },
	{ 0x2a45, 0x0c02, "Meizu Corp.", "MX Phone (MTP & ADB)" },
	{ 0x2a45, 0x0c03, "Meizu Corp.", "MX Phone (BICR & ADB)" },
	{ 0x2a45, 0x2008, "Meizu Corp.", "MX Phone (MTP)" },
	{ 0x2a45, 0x200a, "Meizu Corp.", "MX Phone (MTP & ACM & ADB)" },
	{ 0x2a45, 0x200b, "Meizu Corp.", "MX Phone (PTP)" },
	{ 0x2a45, 0x200c, "Meizu Corp.", "MX Phone (PTP & ADB)" },
	{ 0x2a45, 0x2012, "Meizu Corp.", "MX Phone (MTP & ACM)" },
	{ 0x2a45, 0xFFFF, "Meizu Corp.", "Phone" },

	{ 0x2a70, 0xFFFF, "OnePlus", "Phone" },

	{ 0x2b4c, 0xFFFF, "Vivo", "Phone" },

	{ 0x413c, 0xFFFF, "Dell", "Notebook" },

	{ 0x8087, 0xFFFF, "Intel", "Notebook" },
};

//05AC751F00C8 iphone 18 pro max
//05AC751900B4 iphone 17 pro max
//05AC751700B4 iPhone 16 Pro Max
//05AC73060244 MacBook Pro M1
//05AC731203D4 MacBook Pro M2
//05AC731902D0 MacBook Pro M4

uint16_t UsbListLength = sizeof(UsbList) / sizeof(UsbList[0]);

static int cmp(const void *vp, const void *vq)
{
	const UsbDevStruct *p = (UsbDevStruct *)vp;
	const UsbDevStruct *q = (UsbDevStruct *)vq;

	if (p->VendorID > q->VendorID) {
		return 1;
	} else if (p->VendorID < q->VendorID) {
		return -1;
	} else if (p->DeviceID > q->DeviceID) {
		return 1;
	} else if (p->DeviceID < q->DeviceID) {
		return -1;
	}

	return 0;
}

static int cmpv(const void *vp, const void *vq)
{
	const UsbDevStruct *p = (UsbDevStruct *)vp;
	const UsbDevStruct *q = (UsbDevStruct *)vq;

	if (p->VendorID > q->VendorID) {
		return 1;
	} else if (p->VendorID < q->VendorID) {
		return -1;
	}

	return 0;
}

UsbDevStruct *UsbListFind(uint16_t vendor, uint16_t device)
{
	UsbDevStruct key;

	key.VendorID = vendor;
	key.DeviceID = device;
	return (UsbDevStruct *)bsearch(&key, UsbList, UsbListLength, sizeof(*UsbList), cmp);
}

UsbDevStruct *UsbListFindVid(uint16_t vendor, uint16_t device)
{
	UsbDevStruct key;

	key.VendorID = vendor;
	key.DeviceID = device;
	return (UsbDevStruct *)bsearch(&key, UsbList, UsbListLength, sizeof(*UsbList), cmpv);
}

