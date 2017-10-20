
#ifndef GPSDO_H
#define GPSDO_H

#include "sdkconfig.h"

#define GPS_LATITUDE			CONFIG_GPS_LATITUDE
#define GPS_LONGTITUDE			CONFIG_GPS_LONGTITUDE
#define GPS_ELEVATION			CONFIG_GPS_ELEVATION

#define INFO_PAGE_STATUS_BODY "<p>"\
"<font size=\"+2\">"\
"&nbsp&nbsp&nbsp<a href=\"/command\">Send a Command</a>&nbsp&nbsp&nbsp"\
"&nbsp&nbsp&nbsp<a href=\"/satellites\">View List of Satellites</a>&nbsp&nbsp&nbsp"\
"</p><p>"\
"&nbsp&nbsp&nbsp 10Mhz <font style=\"BACKGROUND-COLOR: %s\"> _ </font>"\
"&nbsp&nbsp&nbsp 1PPS <font style=\"BACKGROUND-COLOR: %s\"> _ </font>"\
"&nbsp&nbsp&nbsp Antenna <font style=\"BACKGROUND-COLOR: %s\"> _ </font>"\
"&nbsp&nbsp&nbsp Hold: <font style=\"BACKGROUND-COLOR: %s\"> %d </font>"\
"&nbsp&nbsp&nbsp NSat: <font style=\"BACKGROUND-COLOR: %s\"> %d </font>"\
"&nbsp&nbsp&nbsp State: <font style=\"BACKGROUND-COLOR: %s\"> %d </font>"\
"</font><font size=\"+4\">"\
"&nbsp&nbsp&nbsp GPS <font size=\"+4\" style=\"BACKGROUND-COLOR: %s\"> OK </font>"\
"</font></p><p>"\
"&nbsp&nbsp&nbsp Lock last acquired: %.24s"\
"&nbsp&nbsp&nbsp Lock last lost: %.24s"\
"</p>"

#define INFO_PAGE_TIME_BODY "<p>"\
"&nbsp&nbsp&nbsp TIME: %.24s"\
"&nbsp&nbsp&nbsp TFOM: <font style=\"BACKGROUND-COLOR: %s\"> %d </font>"\
"</p>"

#define INFO_PAGE_POSITION_BODY "<p>"\
"&nbsp&nbsp&nbsp LAT: %f"\
"&nbsp&nbsp&nbsp LONG: %f"\
"&nbsp&nbsp&nbsp Elev: %d"\
"</p>"

#define INFO_PAGE_SURVEY_BODY "<p>"\
"&nbsp&nbsp&nbsp<font style=\"BACKGROUND-COLOR: %s\"> Self Survey: %d:%02d </font>"\
"</p>"

#define SATELLITE_LIST_BODY "<p>"\
"<font size=\"+2\">"\
"&nbsp&nbsp&nbsp<a href=\"/command\">Send a Command</a>&nbsp&nbsp&nbsp"\
"&nbsp&nbsp&nbsp<a href=\"/\">GPS Info</a>&nbsp&nbsp&nbsp"\
"</font></p><p>"\
"<p><table border=1><tr><th>PRN</th><th>Chan</th><th>Elev</th></th><th>Azim</th><th>Signal</th><th>Last Seen</th></tr>"

#define COMMAND_PAGE_BODY "<p>"\
"<font size=\"+2\">"\
"&nbsp&nbsp&nbsp<a href=\"/\">GPS Info</a>&nbsp&nbsp&nbsp"\
"&nbsp&nbsp&nbsp<a href=\"/satellites\">View List of Satellites</a>&nbsp&nbsp&nbsp"\
"</font></p><p>"\
"<form action=\"POST\">"\
"Command:<input type=\"text\" name=\"command\" value=\"\">&nbsp"\
"<input type=\"text\" name=\"data\" value=\"\">&nbsp"\
"<input type=\"submit\" value=\"Submit\">"\
"</form>"\
"</p>\r\n"

#endif

