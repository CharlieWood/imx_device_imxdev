/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this implements a GPS hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/gps.goldfish.so
 *
 * it will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from android_location_GpsLocationProvider.cpp
 */


#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>

#define  LOG_TAG  "gps_nmea"
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <hardware/gps.h>

#define  GPS_DEBUG  0

#if GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#define __HAVE_CDSHF_AND_CDDPS__ 
#define __HAVE_NMEA_CALLBACK__

#define GPS_SERIAL_DEVICE  "/dev/ttymxc1"
#define GPS_POWER_CONTROL "/sys/devices/platform/gps-control.0/gps_pwr_en"
GpsStatus sGpsStatus;


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   T O K E N I Z E R                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

typedef struct {
    const char*  p;
    const char*  end;
} Token;

#define  MAX_NMEA_TOKENS 100 

typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int
nmea_tokenizer_init( NmeaTokenizer*  t, const char*  p, const char*  end )
{
    int    count = 0;
    char*  q;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        //jump over double comma such as ",,"
        if (q > p) {
            if (count < MAX_NMEA_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token
nmea_tokenizer_get( NmeaTokenizer*  t, int  index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else
        tok = t->tokens[index];

    return tok;
}


static int
str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double
str2float( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   P A R S E R                           *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

#define  NMEA_MAX_SIZE 500 

#define  GGA_FLAG       0x0001
#define  GSA_FLAG       0x0002
#define  GSV_FLAG       0x0004
#define  RMC_FLAG       0x0008
#define  SHF_FLAG       0x0010
#define  DPS_FLAG       0x0020
#define  PRS_FLAG       0x0040
#define  GDO_FLAG       0x0080

#define  LOCATION_FLAG  0x0009

#ifdef   __HAVE_CDSHF_AND_CDDPS__
#define  SVINFO_FLAG    0x0036
#else
#define  SVINFO_FLAG    0x0006
#endif

typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    int     utc_diff;
    uint16_t     cb_flag;
    GpsLocation  fix;
    GpsSvStatus    sv;
    GpsCallbacks  callback;
    char    in[ NMEA_MAX_SIZE+1 ];
} NmeaReader;


static void
nmea_reader_update_utc_diff( NmeaReader*  r )
{
    time_t         now = time(NULL);
    struct tm      tm_local;
    struct tm      tm_utc;
    long           time_local, time_utc;

    gmtime_r( &now, &tm_utc );
    localtime_r( &now, &tm_local );

    time_local = tm_local.tm_sec +
                 60*(tm_local.tm_min +
                 60*(tm_local.tm_hour +
                 24*(tm_local.tm_yday +
                 365*tm_local.tm_year)));

    time_utc = tm_utc.tm_sec +
               60*(tm_utc.tm_min +
               60*(tm_utc.tm_hour +
               24*(tm_utc.tm_yday +
               365*tm_utc.tm_year)));

    r->utc_diff = time_utc - time_local;
}


static void
nmea_reader_init( NmeaReader*  r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;
    r->cb_flag = 0;
    r->callback.location_cb = NULL;
    r->callback.status_cb = NULL;
    r->callback.sv_status_cb = NULL;
    r->callback.nmea_cb = NULL;

    nmea_reader_update_utc_diff( r );
}


    static void
nmea_reader_set_callback( NmeaReader*  r, GpsCallbacks*  cb )
{
    if(cb == NULL){
        r->callback.location_cb = NULL;
        r->callback.status_cb = NULL;
        r->callback.sv_status_cb = NULL;
        r->callback.nmea_cb = NULL;
    } else {
        r->callback.location_cb = cb->location_cb;
        r->callback.status_cb = cb->status_cb;
        r->callback.sv_status_cb = cb->sv_status_cb;
        r->callback.nmea_cb = cb->nmea_cb;
    }
    if(cb != NULL && (r->cb_flag & SVINFO_FLAG ) > 0){
        D("%s: sending lastest sv status to new callback", __FUNCTION__);
        r->callback.sv_status_cb( &r->sv );
        r->sv.num_svs = 0;
        r->cb_flag &= (~SVINFO_FLAG) ;
    }
}


static int
nmea_reader_update_time( NmeaReader*  r, Token  tok )
{
    int        hour, minute;
    double     seconds;
    struct tm  tm;
    time_t     fix_time;

    if (tok.p + 6 > tok.end)
        return -1;

    if (r->utc_year < 0) {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        r->utc_year = tm.tm_year + 1900;
        r->utc_mon  = tm.tm_mon + 1;
        r->utc_day  = tm.tm_mday;
    }

    hour    = str2int(tok.p,   tok.p+2);
    minute  = str2int(tok.p+2, tok.p+4);
    seconds = str2float(tok.p+4, tok.end);

    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_sec   = (int) seconds;
    tm.tm_year  = r->utc_year - 1900;
    tm.tm_mon   = r->utc_mon - 1;
    tm.tm_mday  = r->utc_day;
    tm.tm_isdst = -1;

    fix_time = mktime( &tm ) - r->utc_diff;
    r->fix.timestamp = (long long)fix_time * 1000;
    return 0;
}

static int
nmea_reader_update_date( NmeaReader*  r, Token  date, Token  time )
{
    Token  tok = date;
    int    day, mon, year;

    if (tok.p + 6 != tok.end) {
        D("date not properly formatted: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    day  = str2int(tok.p, tok.p+2);
    mon  = str2int(tok.p+2, tok.p+4);
    year = str2int(tok.p+4, tok.p+6) + 2000;

    if ((day|mon|year) < 0) {
        D("date not properly formatted: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }

    r->utc_year  = year;
    r->utc_mon   = mon;
    r->utc_day   = day;

    return nmea_reader_update_time( r, time );
}


static double
convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}


static int
nmea_reader_update_latlong( NmeaReader*  r,
                            Token        latitude,
                            char         latitudeHemi,
                            Token        longitude,
                            char         longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end) {
        D("latitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        D("longitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;
    return 0;
}


static int
nmea_reader_update_altitude( NmeaReader*  r,
                             Token        altitude,
                             Token        units )
{
    double  alt;
    Token   tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(tok.p, tok.end);
    D("update altitude, altitude= %f", r->fix.altitude);
    return 0;
}


static int
nmea_reader_update_bearing( NmeaReader*  r,
                            Token        bearing )
{
    double  alt;
    Token   tok = bearing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_BEARING;
    r->fix.bearing  = str2float(tok.p, tok.end);
    return 0;
}


static int
nmea_reader_update_speed( NmeaReader*  r,
                          Token        speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end) * (float)0.51444444444;
    return 0;
}

static int 
nmea_reader_update_svinfo( NmeaReader* r,
        Token    prn,
        Token    snr,
        Token    elevation,
        Token    azimuth)
{
    int i = r->sv.num_svs;
    int ret;

    ret = r->sv.sv_list[i].prn =  (prn.p < prn.end) ? str2int(prn.p, prn.end) : -1 ;
    if (ret < 0 ) return -1;

    ret = r->sv.sv_list[i].snr = (snr.p < snr.end) ? (float)str2int(snr.p, snr.end) : -1; 
    if (ret < 0 ) return -2;

    ret  = r->sv.sv_list[i].elevation = (elevation.p < elevation.end) ? (float)str2int(elevation.p, elevation.end) : -1; 
    if (ret < 0 ) return -3;

    ret = r->sv.sv_list[i].azimuth = (azimuth.p < azimuth.end) ? (float)str2int(azimuth.p, azimuth.end) : -1; 
    if (ret < 0 ) return -4;

    r->sv.num_svs += 1;
    return 0;
}

static void
nmea_reader_parse( NmeaReader*  r )
{
    /* we received a complete sentence, now parse it to generate
     * a new GPS fix...
     */
    NmeaTokenizer  tzer[1];
    Token          tok;

#ifdef __HAVE_NMEA_CALLBACK__
    //nmea callback, added by xecle 2010-8-11
    if ( r->callback.nmea_cb && r->pos > 8)
    {
//        D("NMEA %.*s callbacking....", 6, r->in);
        r->callback.nmea_cb ( r->fix.timestamp, r->in, r->pos -2 );
    }
#endif

    D("Received: '%.*s'", r->pos, r->in);
    //LOGD("%.*s", r->pos, r->in);
    if (r->pos < 9) {
        D("Too short. discarded.");
        return;
    }

    nmea_tokenizer_init(tzer, r->in, r->in + r->pos);
#if GPS_DEBUG
    {
        int  n;
        D("Found %d tokens", tzer->count);
        for (n = 0; n < tzer->count; n++) {
            Token  tok = nmea_tokenizer_get(tzer,n);
            //D("%2d: '%.*s'", n, tok.end-tok.p, tok.p);
        }
    }
#endif

    tok = nmea_tokenizer_get(tzer, 0);
    if (tok.p + 5 > tok.end) {
        D("sentence id '%.*s' too short, ignored.", tok.end-tok.p, tok.p);
        return;
    }

    // ignore first two characters.
    tok.p += 2;
    if ( !memcmp(tok.p, "GGA", 3) ) {
        // GPS fix
        Token  tok_time          = nmea_tokenizer_get(tzer,1);
        Token  tok_latitude      = nmea_tokenizer_get(tzer,2);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,3);
        Token  tok_longitude     = nmea_tokenizer_get(tzer,4);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,5);
        Token  tok_altitude      = nmea_tokenizer_get(tzer,9);
        Token  tok_altitudeUnits = nmea_tokenizer_get(tzer,10);

        nmea_reader_update_time(r, tok_time);
        nmea_reader_update_latlong(r, tok_latitude,
                                      tok_latitudeHemi.p[0],
                                      tok_longitude,
                                      tok_longitudeHemi.p[0]);
        nmea_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);
        r->cb_flag |= GGA_FLAG ;

    } else if ( !memcmp(tok.p, "GSV", 3) ) {
        //GSV
        int i,no;
        int ret;
        Token   tok_num         = nmea_tokenizer_get(tzer,1);
        Token   tok_order       = nmea_tokenizer_get(tzer,2);
        int     num             = str2int(tok_num.p, tok_num.end);
        int     order           = str2int(tok_order.p, tok_order.end);
        if( order == 1) 
            r->sv.num_svs = 0;
        
        Token  tok_total         = nmea_tokenizer_get(tzer,3);
        no = (tzer->count-1)/4;
        for (i =0; i<no; i++) {
            Token  tok_prn           = nmea_tokenizer_get(tzer,4+i*4);
            Token  tok_elevation     = nmea_tokenizer_get(tzer,5+i*4);
            Token  tok_azimuth       = nmea_tokenizer_get(tzer,6+i*4);
            Token  tok_snr           = nmea_tokenizer_get(tzer,7+i*4);
            ret = nmea_reader_update_svinfo(r, tok_prn,
                    tok_snr,
                    tok_elevation,
                    tok_azimuth);
			if (ret < 0 )
			  D("GSV update_svinfo error @ %d", ret);
        }
        D("receive a GSV: %d/%d  NMEA: '%.*s", order, num, r->pos, r->in);

        if( num == order) r->cb_flag |= GSV_FLAG ; 

    } else if ( !memcmp(tok.p, "GSA", 3) ) {
        //GSA
        Token tok_prn;
        int i,prn;
        r->sv.used_in_fix_mask = 0;  //masked base on prn
            for( i=0; i<12; i++){
                tok_prn = nmea_tokenizer_get(tzer,i+3);
                prn = str2int(tok_prn.p, tok_prn.end) -1;
                r->sv.used_in_fix_mask |= ( ((uint32_t)1) << prn);
            }
            r->cb_flag |= GSA_FLAG;

    } else if ( !memcmp(tok.p, "RMC", 3) ) {
        Token  tok_time          = nmea_tokenizer_get(tzer,1);
        Token  tok_fixStatus     = nmea_tokenizer_get(tzer,2);
        Token  tok_latitude      = nmea_tokenizer_get(tzer,3);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,4);
        Token  tok_longitude     = nmea_tokenizer_get(tzer,5);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,6);
        Token  tok_speed         = nmea_tokenizer_get(tzer,7);
        Token  tok_bearing       = nmea_tokenizer_get(tzer,8);
        Token  tok_date          = nmea_tokenizer_get(tzer,9);

        D("in RMC, fixStatus=%c", tok_fixStatus.p[0]);
        if (tok_fixStatus.p[0] == 'A')
        {
            nmea_reader_update_date( r, tok_date, tok_time );

            nmea_reader_update_latlong( r, tok_latitude,
                                           tok_latitudeHemi.p[0],
                                           tok_longitude,
                                           tok_longitudeHemi.p[0] );

            nmea_reader_update_bearing( r, tok_bearing );
            nmea_reader_update_speed  ( r, tok_speed );
        } else r->fix.flags = 0;
        r->cb_flag |= RMC_FLAG ;
    } else if ( !memcmp(tok.p, "SHF", 3) ) {
        r->cb_flag |= SHF_FLAG;
    } else if ( !memcmp(tok.p, "DPS", 3) ) {
        r->cb_flag |= DPS_FLAG;
    } else {
        tok.p -= 2;
        D("unknown sentence '%.*s", tok.end-tok.p, tok.p);
    }
//    if ((r->cb_flag & LOCATION_FLAG) == LOCATION_FLAG  && (r->fix.flags !=0)) {
    if ((r->cb_flag & RMC_FLAG) == RMC_FLAG ) {
#if GPS_DEBUG
        char   temp[256];
        char*  p   = temp;
        char*  end = p + sizeof(temp);
        struct tm   utc;

        p += snprintf( p, end-p, "sending fix" );
        if (r->fix.flags & GPS_LOCATION_HAS_LAT_LONG) {
            p += snprintf(p, end-p, " lat=%g lon=%g", r->fix.latitude, r->fix.longitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ALTITUDE) {
            p += snprintf(p, end-p, " altitude=%g", r->fix.altitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_SPEED) {
            p += snprintf(p, end-p, " speed=%g", r->fix.speed);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_BEARING) {
            p += snprintf(p, end-p, " bearing=%g", r->fix.bearing);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ACCURACY) {
            p += snprintf(p,end-p, " accuracy=%g", r->fix.accuracy);
        }
        gmtime_r( (time_t*) &r->fix.timestamp, &utc );
        p += snprintf(p, end-p, " time=%s", asctime( &utc ) );
        D(temp);
#endif
        if (r->callback.location_cb) {
            r->callback.location_cb( &r->fix );
            r->fix.flags = 0;
            r->cb_flag &= (~LOCATION_FLAG);
        }
        else {
            D("no callback, keeping data until needed !");
        }
    }
    if ((r->cb_flag & SVINFO_FLAG) > 0 ) {
        r->sv.ephemeris_mask = 0xffffffff;
        r->sv.almanac_mask = 0xffffffff;
        if(r->callback.sv_status_cb){
            D("send a sv status,sv num:%d, used mask:%x, allbacking ......",
                    r->sv.num_svs,r->sv.used_in_fix_mask);
            r->callback.sv_status_cb( &r->sv );
        }
        r->cb_flag &= (~SVINFO_FLAG);
    }
}


static void
nmea_reader_addc( NmeaReader*  r, int  c )
{
    if (r->overflow) {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in)-1 ) {
        r->overflow = 1;
        r->pos      = 0;
        return;
    }

    r->in[r->pos] = (char)c;
    r->pos       += 1;

    if (c == '\n') {
        nmea_reader_parse( r );
        r->pos = 0;
    }
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       C O N N E C T I O N   S T A T E                 *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

/* commands sent to the gps thread */
enum {
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};


/* this is the state of our connection to the ix210_gpsd daemon */
typedef struct {
    int                     init;
    int                     fd;
    GpsCallbacks            callbacks;
    pthread_t               thread;
    int                     control[2];
} GpsState;

static GpsState  _gps_state[1];


static void
gps_power_control( int on )
{
    int ret;
    int file;
    file = open(GPS_POWER_CONTROL, O_WRONLY);
    if (file==-1) {
        LOGE("could not open gps power control: errno=%d: %s", errno, strerror(errno));
        return;
    }
    if (on)
        ret = write(file, "1", 1);
    else
        ret = write(file, "0", 1);
    close(file);
}

static void
gps_state_done( GpsState*  s )
{
    // tell the thread to quit, and wait for it
    char   cmd = CMD_QUIT;
    void*  dummy;
    write( s->control[0], &cmd, 1 );
    pthread_join(s->thread, &dummy);

    // close the control socket pair
    close( s->control[0] ); s->control[0] = -1;
    close( s->control[1] ); s->control[1] = -1;

    // close connection to the QEMU GPS daemon
    close( s->fd ); s->fd = -1;
    s->init = 0;
}


static void
gps_state_start( GpsState*  s )
{
    char  cmd = CMD_START;
    int   ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        D("%s: could not send CMD_START command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static void
gps_state_stop( GpsState*  s )
{
    char  cmd = CMD_STOP;
    int   ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        D("%s: could not send CMD_STOP command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}


static int
epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

/* this is the main thread, it waits for commands from gps_state_start/stop and,
 * when started, messages from the QEMU GPS daemon. these are simple NMEA sentences
 * that must be parsed to be converted into GPS fixes sent to the framework
 */
static void*
gps_state_thread( void*  arg )
{
    GpsState*   state = (GpsState*) arg;
    NmeaReader  reader[1];
    int         epoll_fd   = epoll_create(2);
    int         started    = 0;
    int         gps_fd     = state->fd;
    int         control_fd = state->control[1];

    nmea_reader_init( reader );

    // register control file descriptors for polling
    epoll_register( epoll_fd, control_fd );
    epoll_register( epoll_fd, gps_fd );

    D("gps thread running");

    // now loop
    for (;;) {
        struct epoll_event   events[2];
        int                  ne, nevents;

        nevents = epoll_wait( epoll_fd, events, 2, -1 );
        if (nevents < 0) {
            if (errno != EINTR)
                LOGE("epoll_wait() unexpected error: %s", strerror(errno));
            continue;
        }
        D("gps thread received %d events", nevents);
        for (ne = 0; ne < nevents; ne++) {
            if ((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0) {
                LOGE("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                goto Exit;
            }
            if ((events[ne].events & EPOLLIN) != 0) {
                int  fd = events[ne].data.fd;

                if (fd == control_fd)
                {
                    char  cmd = 255;
                    int   ret;
                    D("gps control fd event");
                    do {
                        ret = read( fd, &cmd, 1 );
                    } while (ret < 0 && errno == EINTR);

                    if (cmd == CMD_QUIT) {
                        D("gps thread quitting on demand");
                        goto Exit;
                    }
                    else if (cmd == CMD_START) {
                        if (!started) {
                            // power on
                            gps_power_control(1);
                            D("gps thread starting  location_cb=%p", state->callbacks.location_cb);
                            started = 1;
                            nmea_reader_set_callback( reader, &state->callbacks );
                            sGpsStatus.status = GPS_STATUS_SESSION_BEGIN;
                            state->callbacks.status_cb(&sGpsStatus);
                            //LOGE("%s status_cb(GPS_STATUS_SESSION_BEGIN)\n", __FUNCTION__);
                        }
                    }
                    else if (cmd == CMD_STOP) {
                        if (started) {
                            D("gps thread stopping");
                            started = 0;
                            nmea_reader_set_callback( reader, NULL );
                            sGpsStatus.status = GPS_STATUS_SESSION_END;
                            state->callbacks.status_cb(&sGpsStatus);
                            //LOGE("%s status_cb(GPS_STATUS_SESSION_END)\n", __FUNCTION__);
                            // power off
                            gps_power_control(0);
                        }
                    }
                }
                else if (fd == gps_fd)
                {
                    char  buff[32];
                    //D("gps fd event");
                    for (;;) {
                        int  nn, ret;

                        ret = read( fd, buff, sizeof(buff) );
                        if (ret < 0) {
                            if (errno == EINTR)
                                continue;
                            if (errno != EWOULDBLOCK)
                                LOGE("error while reading from gps daemon socket: %s:", strerror(errno));
                            break;
                        }
                        //D("received %d bytes: %.*s", ret, ret, buff);
                        for (nn = 0; nn < ret; nn++)
                            nmea_reader_addc( reader, buff[nn] );
                    }
                    D("gps fd event end");
                }
                else
                {
                    LOGE("epoll_wait() returned unkown fd %d ?", fd);
                }
            }
        }
    }
Exit:
    return NULL;
}


static int gps_serial_open(void)
{
    struct termios termios;
    speed_t speed;
    int i, fd, flags;
    int oFlags = O_NOCTTY | O_RDWR;
    oFlags |= O_NONBLOCK;       // open O_NONBLOCK to avoid program hang at open

    speed = B115200;

    fd = open(GPS_SERIAL_DEVICE, oFlags);
    if (-1 == fd)
    {
        LOGE("gps_serial_open: erro %d \"%s\" Opening the serial port.\n",
                errno, strerror(errno));
        return -1;
    }

    D("gps will read from '%s'", GPS_SERIAL_DEVICE );

    flags = fcntl(fd, F_GETFL);
    if (-1 == flags)
    {
        LOGE("read GPS serial port nonblocking");
        return -1;
    }
    flags &= ~O_NONBLOCK;
    flags |=  O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flags))
    {
        LOGE("set GPS serial port nonblocking");
        return -1;
    }

    // Return to the block mode
    i = tcgetattr(fd, &termios);

    if (i < 0)
    {
        close(fd);
        LOGE("gps_serial_open: IOCTL failed - %s\n", strerror(errno));
        return -1;
    }

    termios.c_iflag = IGNBRK | IGNPAR;
    termios.c_oflag = 0;
    termios.c_lflag = 0;

    // Extremely important!!!
    // For the ASIC to work, Set Hardware Control by CRTSCTS
    termios.c_cflag = speed | CS8 | CREAD | CLOCAL | HUPCL;

    //memset(termios.c_cc, 0, NCCS);
    //termios.c_cc[VMIN] = 0;
    //termios.c_cc[VTIME]= 1;

    i = tcsetattr(fd, TCSANOW, &termios);

    if (i < 0)
    {
        close(fd);
        LOGE("gps_serial_open: IOCTL failed - %s\n", strerror(errno));
        return -1;
    }

    return fd;           // Success
}

static void
gps_state_init( GpsState*  state )
{
    state->init       = 1;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;
    sGpsStatus.status = GPS_STATUS_NONE;

    state->fd = gps_serial_open();

    if (state->fd < 0) {
        D("no gps hardware detected");
        return;
    }

    if ( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 ) {
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

    if ( pthread_create( &state->thread, NULL, gps_state_thread, state ) != 0 ) {
        LOGE("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }

    D("gps state initialized");
    return;

Fail:
    gps_state_done( state );
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


static int
nmea_gps_init(GpsCallbacks* callbacks)
{
    GpsState*  s = _gps_state;

    if (!s->init)
        gps_state_init(s);

    if (s->fd < 0)
        return -1;

    s->callbacks = *callbacks;

    return 0;
}

static void
nmea_gps_cleanup(void)
{
    GpsState*  s = _gps_state;

    if (s->init)
        gps_state_done(s);
}


static int
nmea_gps_start()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    D("%s: called", __FUNCTION__);
    gps_state_start(s);
    return 0;
}


static int
nmea_gps_stop()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    D("%s: called", __FUNCTION__);
    gps_state_stop(s);
    return 0;
}


static int
nmea_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    return 0;
}

static int
nmea_gps_inject_location(double latitude, double longitude, float accuracy)
{
    return 0;
}

static void
nmea_gps_delete_aiding_data(GpsAidingData flags)
{
}

static int nmea_gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
    // FIXME - support fix_frequency
    return 0;
}

static const void*
nmea_gps_get_extension(const char* name)
{
    return NULL;
}

static const GpsInterface  nmeaGpsInterface = {
    .size = sizeof(GpsInterface),
    .init = nmea_gps_init,
    .start = nmea_gps_start,
    .stop = nmea_gps_stop,
    .cleanup = nmea_gps_cleanup,
    .inject_time = nmea_gps_inject_time,
    .inject_location = nmea_gps_inject_location,
    .delete_aiding_data = nmea_gps_delete_aiding_data,
    .set_position_mode = nmea_gps_set_position_mode,
    .get_extension = nmea_gps_get_extension,
};

static const GpsInterface* gps__get_gps_interface(struct gps_device_t* dev)
{
    return &nmeaGpsInterface;
}

static int open_gps(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    struct gps_device_t *dev = malloc(sizeof(struct gps_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->get_gps_interface = gps__get_gps_interface;

    *device = (struct hw_device_t*)dev;
    return 0;
}


static struct hw_module_methods_t gps_module_methods = {
    .open = open_gps
};

const struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "nema GPS Module",
    .author = "Letou, Inc.",
    .methods = &gps_module_methods,
};
