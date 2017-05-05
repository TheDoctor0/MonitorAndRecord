//Compile
gcc -Wall -g -o udev_example udev_example.c -ludev

#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

int main (void)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}
	
	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* usb_device_get_devnode() returns the path to the device node
		   itself in /dev. */
		printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

		/* The device pointed to by dev contains information about
		   the hidraw device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". This will
		   be several levels up the tree, but the function will find
		   it.*/
		dev = udev_device_get_parent_with_subsystem_devtype(
		       dev,
		       "usb",
		       "usb_device");
		if (!dev) {
			printf("Unable to find parent usb device.");
			exit(1);
		}
	
		/* From here, we can call get_sysattr_value() for each file
		   in the device's /sys entry. The strings passed into these
		   functions (idProduct, idVendor, serial, etc.) correspond
		   directly to the files in the directory which represents
		   the USB device. Note that USB strings are Unicode, UCS2
		   encoded, but the strings returned from
		   udev_device_get_sysattr_value() are UTF-8 encoded. */
		printf("  VID/PID: %s %s\n",
		        udev_device_get_sysattr_value(dev,"idVendor"),
		        udev_device_get_sysattr_value(dev, "idProduct"));
		printf("  %s\n  %s\n",
		        udev_device_get_sysattr_value(dev,"manufacturer"),
		        udev_device_get_sysattr_value(dev,"product"));
		printf("  serial: %s\n",
		         udev_device_get_sysattr_value(dev, "serial"));
		udev_device_unref(dev);
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

	return 0;       
}

//Example 1

#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <libudev.h>
#include <fcntl.h>

struct udev *udev = udev_new();
if (!udev) {
    cout << "Can't create udev" <<endl;
}

struct udev_enumerate *enumerate = udev_enumerate_new(udev);
udev_enumerate_add_match_subsystem(enumerate, "usb");
udev_enumerate_scan_devices(enumerate);
struct udev_list_entry *dev_list_entry, *devices = udev_enumerate_get_list_entry(enumerate);
struct udev_device *dev;
udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path;
    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);
    if( udev_device_get_devnode(dev) != nullptr ){
        string vendor = (std::string) udev_device_get_sysattr_value(dev, "idVendor");
        string product = (std::string) udev_device_get_sysattr_value(dev, "idProduct");
        string description = (std::string)udev_device_get_sysattr_value(dev, "product");
        cout << vendor << product << description << endl;
    }
    udev_device_unref(dev);
}
udev_enumerate_unref(enumerate); 

//Removal
struct udev_device *dev;
struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
udev_monitor_enable_receiving(mon);

int fd = udev_monitor_get_fd(mon);

int flags = fcntl(fd, F_GETFL, 0);
if (flags == -1){
    debugError("Can't get flags for fd");
}
flags &= ~O_NONBLOCK;
fcntl(fd, F_SETFL, flags);
fd_set fds;
FD_ZERO(&fds);
FD_SET(fd, &fds);

while( _running ){
    cout << "waiting for udev" << endl;
    dev = udev_monitor_receive_device(mon);
    if (dev && udev_device_get_devnode(dev) != nullptr ) {
        string action = (std::string)udev_device_get_action(dev);
        if( action == "add" ){
            cout << "do something with your device... " << endl;
        } else {
            string path = (std::string)udev_device_get_devnode(dev);
            for( auto device : _DevicesList ){
                if( device.getPath() == path ){
                    _DevicesList.erase(iter);
                    cout << "Erased Device from list" << endl;
                    break;
                }
            }
        }
        udev_device_unref(dev);
    }
}
udev_monitor_unref(mon);