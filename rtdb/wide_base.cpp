#include "wide_base.h"
#include "RTDB/wide_rtdb.h"
#include "TAOS/wide_taos.h"
#include "../wide_timescaledb.h"
#include "OPENTSDB/wide_opentsdb.h"
#include "INFLUXDB/wide_influxdb.h"

namespace rtdb
{

namespace wide
{

bool rtdb_init( const char * path )
{
    return NULL != tsdb_v3_init( path );
}

tsdb_v3_t * rtdb_tls()
{
    return tsdb_v3_tls( TSDB_V3_VERSION );
}

tsdb_v3_t * rtdb_new()
{
    return tsdb_v3_new( TSDB_V3_VERSION );
}

db_type_t get_db_type( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    const char * engine = NULL;
    p->tools->find_argv( argc, argv, "engine", & engine, NULL );
    if ( NULL == engine ) {
        engine = "";
    }

    if ( 0 == stricmp( "rtdb", engine ) ) {
        return DB_RTDB;
    } else if ( 0 == stricmp( "taos", engine ) ) {
#if ENABLE_TDENGINE
        return DB_TAOS;
#endif // #if ENABLE_TDENGINE
        return DB_UNKNOWN;
    }
    else if (0 == stricmp("timescaledb", engine)) {
#if ENABLE_TIMESCALEDB
        return DB_TIMESCALEDB;
#endif // #if ENABLE_TIMESCALEDB
         return DB_UNKNOWN;
    } 
    else if (0 == stricmp("opentsdb", engine)) {
#if ENABLE_OPENTSDB
        return DB_OPENTSDB;
#endif // #if ENABLE_OPENTSDB
        return DB_UNKNOWN;
    }
    else if (0 == stricmp("influxdb", engine)) {
#if ENABLE_INFLUXDB
        return DB_INFLUXDB;
#endif // #if ENABLE_INFLUXDB
        return DB_UNKNOWN;
    }
    else {
        TSDB_ERROR( p, "[engine=%s]invalid engine, only supported: rtdb | taos | timescaledb", engine );
        return DB_UNKNOWN;
    }
}

wide_base_t * wide_base_t::instance( db_type_t type, int argc, char ** argv )
{
    static wide_base_t * instances[ DB_LAST + 1 ];

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    if ( type >= DB_FIRST && type <= DB_LAST ) {
        wide_base_t * o = instances[ type ];
        if ( unlikely( NULL == o ) ) {
            switch ( type )
            {
            case DB_RTDB:
                o = new wide_rtdb_t();
                break;
            case DB_TAOS:
                o = new wide_taos_t();
                break;
            case DB_TIMESCALEDB:
#if ENABLE_TIMESCALEDB
                o = new wide_timescaledb_t();
#else
                TSDB_ERROR( p, "[type=%d]this system version not support for timescale", (int)type );
                o = NULL;
#endif
                break;
            case DB_OPENTSDB:
#if ENABLE_OPENTSDB
                o = new wide_opentsdb_t();
#else
                TSDB_ERROR(p, "[type=%d]this system version not support for opentsdb", (int)type);
                o = NULL;
#endif
                break;
            case DB_INFLUXDB:
#if ENABLE_INFLUXDB
                o = new wide_influxdb_t();
#else
                TSDB_ERROR(p, "[type=%d]this system version not support for influxdb", (int)type);
                o = NULL;
#endif
                break;
            default:
                TSDB_ERROR( p, "[type=%d]invalid type", (int)type );
                o = NULL;
                break;
            }
            // global initialize the engine
            int r = o->global_init(argc, argv);
            if (0 != r) {
                TSDB_ERROR(p, "[type=%d]invalid type", (int)type);
                delete o;
                return NULL;
            }
            instances[ type ] = o;
        }
        return o;
    }

    TSDB_ERROR( p, "[type=%d]invalid type", (int)type );
    return NULL;
}

} // namespace wide

} // namespace rtdb
