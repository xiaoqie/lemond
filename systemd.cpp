#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include "systemd.h"
#include "json.h"

using namespace std;

string systemd_list_units() {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *bus = NULL;
    int r;
    const char *s1;
    const char *s2;
    const char *s3;
    const char *s4;
    const char *s5;
    const char *s6;
    const char *s7;
    uint s8;
    const char *s9;
    const char *s10;
    json_generator json;

    /* Connect to the system bus */
    r = sd_bus_open_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        goto finish;
    }

    /* Issue the method call and store the respons message in m */
    r = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",           /* service to contact */
                           "/org/freedesktop/systemd1",          /* object path */
                           "org.freedesktop.systemd1.Manager",   /* interface name */
                           "ListUnits",                          /* method name */
                           &error,                               /* object to return error in */
                           &m,                                   /* return message on success */
                           "");
    // out a(ssssssouso)

    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        goto finish;
    }

    // r = sd_bus_message_peek_type(m, &'a', &"ssssssouso");

    r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(ssssssouso)");
    if (r < 0) {
        fprintf(stderr, "Failed to enter container: %s\n", strerror(-r));
        goto finish;
    }
    json.list_start();
    while ((r = sd_bus_message_read(m, "(ssssssouso)", &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8, &s9, &s10)) > 0) {
        json.list_entry();
        json.list_start();
        json.list_entry(s1);
        json.list_entry(s2);
        json.list_entry(s3);
        json.list_entry(s4);
        json.list_entry(s5);
        json.list_entry(s6);
        json.list_entry(s7);
        json.list_entry(s8);
        json.list_entry(s9);
        json.list_entry(s10);
        json.list_end();
    }
    json.list_end();
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        goto finish;
    }

    finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);

    if (r < 0) {
        return "0";
    } else {
        return json.raw_str();
    }
}