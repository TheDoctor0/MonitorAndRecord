#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

static int count = 0;
void hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                      libusb_hotplug_event event, void *user_data) {
  static libusb_device_handle *handle = NULL;
  struct libusb_device_descriptor desc;
  int rc;
  (void)libusb_get_device_descriptor(dev, &desc);
  if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
    rc = libusb_open(dev, &handle);
    if (LIBUSB_SUCCESS != rc) {
      printf("Could not open USB device\n");
    }
  } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
    if (handle) {
      libusb_close(handle);
      handle = NULL;
    }
  } else {
    printf("Unhandled event %d\n", event);
  }
  count++;
}
int main (void) {
  libusb_hotplug_callback callback;
  libusb_init(NULL);
  libusb_hotplug_prepare_callback(&callback, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, 0x045a, 0x5005,
                                  LIBUSB_HOTPLUG_CLASS_ANY, hotplug_callback, NULL);
  libusb_hotplug_register_callback(&callback);
  while (count < 2) {
    usleep(10000);
  }
  libusb_hotplug_deregister_callback(&callback);
  libusb_exit(NULL);
  return 0;
}
