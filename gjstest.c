#include <gjsjoystick.h>
#include <stdio.h>

int main(void) {
	GjsJoystick* js = gjs_joystick_open("/dev/input/js0");

	GError* err;

	gchar* name = gjs_joystick_describe(js, &err);

	if(!name) {
		fprintf(stderr, "E: %s\n", err->message);
		return 1;
	}

	guint axes = gjs_joystick_get_axis_count(js, &err);
	guint buttons = gjs_joystick_get_button_count(js, &err);

	printf("The joystick is a %s\n", name);
	printf("It has %u axes and %u buttons\n", axes, buttons);

	guchar i;

	for(i=0; i<axes; i++) {
		printf("Axis %u: %s\n", i, gjs_joystick_describe_axis(js, i));
	}
	for(i=0; i<buttons; i++) {
		printf("Button %u: %s\n", i, gjs_joystick_describe_button(js, i));
	}

	g_object_unref(js);

	return 0;
}
