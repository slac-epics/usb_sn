#undef ASYN_SETOPTION_TTYNAME
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <dbCommon.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>
#include <aSubRecord.h>
#include <iocsh.h>
#include <drvAsynSerialPort.h>
#include <asynShellCommands.h>
#include <dirent.h>

#define MAX_MAPS 128
static struct usbmapping {
    char *port;
    char *serial;
    char *tty;
    char  lastserial[1024];
} map[MAX_MAPS];
static int mapcnt = 0;

static int readserial(char *tty, char *buf)
{
    FILE *fp;
    char fn[1024];
    int n = -1;

    sprintf(fn, "/sys/bus/usb-serial/devices/%s/../../serial", tty);
    if ((fp = fopen(fn, "r")) == NULL)
        return 1;
    if (fgets(buf, 1024, fp)) {
        n = strlen(buf);
        if (buf[n-1] == '\n')
            buf[n-1] = 0;
    }
    fclose(fp);
    return n < 0;
}

static char *findserial(char *serial)
{
    DIR *d;
    struct dirent *dir;
    char buf[1024];

    d = opendir("/sys/bus/usb-serial/devices");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (!strncmp(dir->d_name, "ttyUSB", 6) &&
                !readserial(dir->d_name, buf) &&
                !strcmp(buf, serial)) {
                closedir(d);
                return strdup(dir->d_name);
            }
        }
        closedir(d);
    }
    return NULL;
}

void USB_Map(char *port, char *serial)
{
    struct usbmapping *m = &map[mapcnt];
    char *tty;
    char buf[1024];

    if (mapcnt == MAX_MAPS) {
        printf("Too many USB maps!\n");
        return;
    }
    tty = findserial(serial);
    if (!tty)
        printf("%s: Serial number %s not found!\n", port, serial);
    else
        printf("%s: Serial number %s is /dev/%s.\n", port, serial, tty);
    m->port    = strdup(port);
    m->serial  = strdup(serial);
    strcpy(m->lastserial, serial);
    if (tty) {
        m->tty = tty;
        sprintf(buf, "/dev/%s", m->tty);
    } else {
        m->tty = strdup("NODEVICE");
        strcpy(buf, "NODEVICE");
    }
    drvAsynSerialPortConfigure(port, buf, 0, 0, 0);
    mapcnt++;
    return;
}

long USB_Check(struct aSubRecord *psub)
{
    char *port   = (char *)psub->a;
    epicsInt32 *valid = (epicsInt32 *)psub->vala;
    char buf[1024];
    int i;
    int v = 1;

    for (i = 0; i < mapcnt; i++)
        if (!strcmp(map[i].port, port))
            break;
    if (i == mapcnt) {
        *valid = 0;   /* No map?!? This is a configuration error. */
        printf("No map for %s!\n", port);
        return 0;
    }

    if (readserial(map[i].tty, buf)) {         /* Has our device has gone away? */
        v = 0;
        if (map[i].lastserial[0]) {
            printf("%s: device has gone away.\n", map[i].port);
#ifdef ASYN_SETOPTION_TTYNAME
            asynSetOption(map[i].port, 0, "ttyname", "NODEVICE");
#endif
            map[i].lastserial[0] = 0;
        }
    } else if (strcmp(buf, map[i].serial)) {   /* Has our device has changed? */
        v = 0;
        if (strcmp(buf, map[i].lastserial)) {
            /* Only print errors when the serial number changes. */
            printf("%s: device serial changed to %s (%s expected)\n",
                   map[i].port, buf, map[i].serial);
#ifdef ASYN_SETOPTION_TTYNAME
            asynSetOption(map[i].port, 0, "ttyname", "NODEVICE");
#endif
            strncpy(map[i].lastserial, buf, 1024);
        }
    }
    if (!v) {
        char *tty = findserial(map[i].serial); /* Can we find it somewhere else? */
        if (tty) {
            printf("%s: Serial number %s is now /dev/%s.\n", map[i].port, map[i].serial, tty);
            free(map[i].tty);
            map[i].tty = tty;
#ifdef ASYN_SETOPTION_TTYNAME
            sprintf(buf, "/dev/%s", tty);
            asynSetOption(map[i].port, 0, "ttyname", buf);
#endif
            v = 1;
        }
    }
    *valid = v;
    return 0;
}

static const iocshArg USB_MapArg0 = { "port name",iocshArgString};
static const iocshArg USB_MapArg1 = { "serial number",iocshArgString};
static const iocshArg *USB_MapArgs[] = {
    &USB_MapArg0, &USB_MapArg1,
};
static const iocshFuncDef USB_MapFuncDef = {"USB_Map",2,USB_MapArgs};

static void USB_MapCallFunc(const iocshArgBuf *args)
{
    USB_Map(args[0].sval, args[1].sval);
}

/*
 * This routine is called before multitasking has started, so there's
 * no race condition in the test/set of firstTime.
 */
static void
USBRegisterCommands(void)
{
    static int firstTime = 1;
    if (firstTime) {
        iocshRegister(&USB_MapFuncDef,USB_MapCallFunc);
        firstTime = 0;
    }
}

epicsExportRegistrar(USBRegisterCommands);
epicsRegisterFunction(USB_Check);
