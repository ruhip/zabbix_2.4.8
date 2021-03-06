/*
** Zabbix
** Copyright (C) 2001-2016 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "common.h"
#include "comms.h"
#include "db.h"
#include "log.h"
#include "dbcache.h"

#include "operations.h"

/******************************************************************************
 *                                                                            *
 * Function: select_discovered_host                                           *
 *                                                                            *
 * Purpose: select hostid of discovered host                                  *
 *                                                                            *
 * Parameters: dhostid - discovered host id                                   *
 *                                                                            *
 * Return value: hostid - existing hostid, 0 - if not found                   *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 ******************************************************************************/
static zbx_uint64_t	select_discovered_host(const DB_EVENT *event)
{
	const char	*__function_name = "select_discovered_host";
	DB_RESULT	result;
	DB_ROW		row;
	zbx_uint64_t	hostid = 0;
	char		*sql = NULL;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() eventid:" ZBX_FS_UI64,
			__function_name, event->eventid);

	switch (event->object)
	{
		case EVENT_OBJECT_DHOST:
			sql = zbx_dsprintf(sql,
				"select h.hostid,h.status"
				" from hosts h,interface i,dservices ds,dchecks dc,drules dr"
				" where h.hostid=i.hostid"
					" and i.ip=ds.ip"
					" and ds.dcheckid=dc.dcheckid"
					" and dc.druleid=dr.druleid"
					" and h.status in (%d,%d)"
					" and " ZBX_SQL_NULLCMP("dr.proxy_hostid", "h.proxy_hostid")
					" and i.useip=1"
					" and ds.dhostid=" ZBX_FS_UI64
				" order by i.hostid",
				HOST_STATUS_MONITORED, HOST_STATUS_NOT_MONITORED,
				event->objectid);
			break;
		case EVENT_OBJECT_DSERVICE:
			sql = zbx_dsprintf(sql,
				"select h.hostid,h.status"
				" from hosts h,interface i,dservices ds,dchecks dc,drules dr"
				" where h.hostid=i.hostid"
					" and i.ip=ds.ip"
					" and ds.dcheckid=dc.dcheckid"
					" and dc.druleid=dr.druleid"
					" and h.status in (%d,%d)"
					" and " ZBX_SQL_NULLCMP("dr.proxy_hostid", "h.proxy_hostid")
					" and i.useip=1"
					" and ds.dserviceid=" ZBX_FS_UI64
				" order by i.hostid",
				HOST_STATUS_MONITORED, HOST_STATUS_NOT_MONITORED,
				event->objectid);
			break;
		default:
			goto exit;
	}

	result = DBselectN(sql, 1);

	zbx_free(sql);

	if (NULL != (row = DBfetch(result)))
		ZBX_STR2UINT64(hostid, row[0]);
	DBfree_result(result);
exit:
	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():" ZBX_FS_UI64, __function_name, hostid);

	return hostid;
}

/******************************************************************************
 *                                                                            *
 * Function: add_discovered_host_groups                                       *
 *                                                                            *
 * Purpose: add group to host if not added already                            *
 *                                                                            *
 * Author: Alexander Vladishev                                                *
 *                                                                            *
 ******************************************************************************/

static void     add_discovered_host_groups(zbx_uint64_t hostid, zbx_vector_uint64_t *groupids)
{
        const char      *__function_name = "add_discovered_host_groups";

        DB_RESULT       result;
        DB_ROW          row;
        zbx_uint64_t    groupid;
        char            *sql = NULL;
        size_t          sql_alloc = 256, sql_offset = 0;
        int             i;

        zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

        sql = zbx_malloc(sql, sql_alloc);
        /*
        zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
                        "select groupid"
                        " from hosts_groups"
                        " where hostid=" ZBX_FS_UI64
                                " and",
                        hostid);
        DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "groupid", groupids->values, groupids->values_num);
        */
         zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
                        "select groupid"
                        " from hosts_groups"
                        " where hostid=" ZBX_FS_UI64,
                        hostid);
        result = DBselect("%s", sql);

        zbx_free(sql);

        for( i = 0; i < groupids->values_num; i++ )
        {
                 int ngroupid = groupids->values[i];
                 zabbix_log(LOG_LEVEL_INFORMATION,"fuck_show 1 groupid:%d",ngroupid);
        }
        int nDiscoverGroup = -1;
        int nOtherGroupid = -1;
        while (NULL != (row = DBfetch(result)))
        {
                ZBX_STR2UINT64(groupid, row[0]);
                zabbix_log(LOG_LEVEL_INFORMATION,"froad_yes:db_groupid:%d",groupid);
                i = zbx_vector_uint64_search(groupids, groupid , ZBX_DEFAULT_UINT64_COMPARE_FUNC);
                if( i > -1  )
                {
                    zbx_vector_uint64_remove_noorder(groupids, i);
                }
                if( groupid == 5 )
                {
                    nDiscoverGroup = 1;
                }else{
                    nOtherGroupid = groupid;
                }
        }

        for( i = 0; i < groupids->values_num; i++ )
        {
                 int ngroupid = groupids->values[i];
                 zabbix_log(LOG_LEVEL_INFORMATION,"foad_show 2 groupid:%d",ngroupid);
        }
        if( groupids->values_num > 0)
        {

                if( 1 == nDiscoverGroup  &&   nOtherGroupid > 0  )
                {
                     zabbix_log(LOG_LEVEL_INFORMATION,"froad_0: clear groupids");
                     zbx_vector_uint64_clear(groupids);
                }else if( -1 == nDiscoverGroup  &&   nOtherGroupid > 0 )
                {
                     int nCount = groupids->values_num;
                     for( i = 0; i < nCount;)
                     {
                         int ngroupid = groupids->values[i];
                         zabbix_log(LOG_LEVEL_INFORMATION,"froad_1:enter add_discovered_host_groups,remove groupid:%d",ngroupid);
                         if( 5 != ngroupid )
                         {
                             zabbix_log(LOG_LEVEL_INFORMATION,"froad_2:enter add_discovered_host_groups");
                             zbx_vector_uint64_remove_noorder(groupids, i);
                             nCount = groupids->values_num;
                         }else
                         {
                             i++;
                         }
                     }
                }
        }
        for( i = 0; i < groupids->values_num; i++ )
        {
                 int ngroupid = groupids->values[i];
                 zabbix_log(LOG_LEVEL_INFORMATION,"froad_show 3 groupid:%d",ngroupid);
        }
        /*
        while (NULL != (row = DBfetch(result)))
        {
                ZBX_STR2UINT64(groupid, row[0]);

                if (FAIL == (i = zbx_vector_uint64_search(groupids, groupid, ZBX_DEFAULT_UINT64_COMPARE_FUNC)))
                {
                        THIS_SHOULD_NEVER_HAPPEN;
                        continue;
                }

                zabbix_log(LOG_LEVEL_INFORMATION,"fuck:enter add_discovered_host_groups,remove groupid:%d",groupid);
                zbx_vector_uint64_clear(groupids);
                //zbx_vector_uint64_remove_noorder(groupids, i);
        }
        */

        DBfree_result(result);
        if( 0 == groupids->values_num )
        {
            zabbix_log(LOG_LEVEL_INFORMATION,"froad:enter add_discovered_host_groups,0 == groupids->values_num");
        }
        if (0 != groupids->values_num)
        {
                zbx_uint64_t    hostgroupid;
                zbx_db_insert_t db_insert;

                hostgroupid = DBget_maxid_num("hosts_groups", groupids->values_num);

                zbx_db_insert_prepare(&db_insert, "hosts_groups", "hostgroupid", "hostid", "groupid", NULL);

                zbx_vector_uint64_sort(groupids, ZBX_DEFAULT_UINT64_COMPARE_FUNC);

                for (i = 0; i < groupids->values_num; i++)
                {
                        zbx_db_insert_add_values(&db_insert, hostgroupid++, hostid, groupids->values[i]);
                }

                zbx_db_insert_execute(&db_insert);
                zbx_db_insert_clean(&db_insert);
        }

        zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}


/*modify by ruhip 20170206*/
/*
static void	add_discovered_host_groups(zbx_uint64_t hostid, zbx_vector_uint64_t *groupids)
{
	const char	*__function_name = "add_discovered_host_groups";

	DB_RESULT	result;
	DB_ROW		row;
	zbx_uint64_t	groupid;
	char		*sql = NULL;
	size_t		sql_alloc = 256, sql_offset = 0;
	int		i;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	sql = zbx_malloc(sql, sql_alloc);

	zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
			"select groupid"
			" from hosts_groups"
			" where hostid=" ZBX_FS_UI64
				" and",
			hostid);
	DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "groupid", groupids->values, groupids->values_num);

	result = DBselect("%s", sql);

	zbx_free(sql);

	while (NULL != (row = DBfetch(result)))
	{
		ZBX_STR2UINT64(groupid, row[0]);

		if (FAIL == (i = zbx_vector_uint64_search(groupids, groupid, ZBX_DEFAULT_UINT64_COMPARE_FUNC)))
		{
			THIS_SHOULD_NEVER_HAPPEN;
			continue;
		}

		zbx_vector_uint64_remove_noorder(groupids, i);
	}
	DBfree_result(result);

	if (0 != groupids->values_num)
	{
		zbx_uint64_t	hostgroupid;
		zbx_db_insert_t	db_insert;

		hostgroupid = DBget_maxid_num("hosts_groups", groupids->values_num);

		zbx_db_insert_prepare(&db_insert, "hosts_groups", "hostgroupid", "hostid", "groupid", NULL);

		zbx_vector_uint64_sort(groupids, ZBX_DEFAULT_UINT64_COMPARE_FUNC);

		for (i = 0; i < groupids->values_num; i++)
		{
			zbx_db_insert_add_values(&db_insert, hostgroupid++, hostid, groupids->values[i]);
		}

		zbx_db_insert_execute(&db_insert);
		zbx_db_insert_clean(&db_insert);
	}

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}
*/
/******************************************************************************
 *                                                                            *
 * Function: add_discovered_host                                              *
 *                                                                            *
 * Purpose: add discovered host if it was not added already                   *
 *                                                                            *
 * Parameters: dhostid - discovered host id                                   *
 *                                                                            *
 * Return value: hostid - new/existing hostid                                 *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 ******************************************************************************/
static zbx_uint64_t	add_discovered_host(const DB_EVENT *event)
{
	const char		*__function_name = "add_discovered_host";

	DB_RESULT		result;
	DB_RESULT		result2;
	DB_ROW			row;
	DB_ROW			row2;
	zbx_uint64_t		dhostid, hostid = 0, proxy_hostid;
	char			*host = NULL, *host_esc, *host_unique;
	unsigned short		port;
	zbx_uint64_t		groupid;
	zbx_vector_uint64_t	groupids;
	unsigned char		svc_type, interface_type;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() eventid:" ZBX_FS_UI64, __function_name, event->eventid);

	zbx_vector_uint64_create(&groupids);

	if (0 == *(zbx_uint64_t *)DCconfig_get_config_data(&groupid, CONFIG_DISCOVERY_GROUPID))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot add discovered host: group for discovered hosts is not defined");
		goto clean;
	}

	zbx_vector_uint64_append(&groupids, groupid);

	if (EVENT_OBJECT_DHOST == event->object || EVENT_OBJECT_DSERVICE == event->object)
	{
		if (EVENT_OBJECT_DHOST == event->object)
		{
			result = DBselect(
					"select ds.dhostid,dr.proxy_hostid,ds.ip,ds.dns,ds.port,ds.type"
					" from drules dr,dchecks dc,dservices ds"
					" where dc.druleid=dr.druleid"
						" and ds.dcheckid=dc.dcheckid"
						" and ds.dhostid=" ZBX_FS_UI64
					" order by ds.dserviceid",
					event->objectid);
		}
		else
		{
			result = DBselect(
					"select ds.dhostid,dr.proxy_hostid,ds.ip,ds.dns,ds.port,ds.type"
					" from drules dr,dchecks dc,dservices ds,dservices ds1"
					" where dc.druleid=dr.druleid"
						" and ds.dcheckid=dc.dcheckid"
						" and ds1.dhostid=ds.dhostid"
						" and ds1.dserviceid=" ZBX_FS_UI64
					" order by ds.dserviceid",
					event->objectid);
		}

		while (NULL != (row = DBfetch(result)))
		{
			ZBX_STR2UINT64(dhostid, row[0]);
			ZBX_DBROW2UINT64(proxy_hostid, row[1]);
			svc_type = (unsigned char)atoi(row[5]);

			switch (svc_type)
			{
				case SVC_AGENT:
					port = (unsigned short)atoi(row[4]);
					interface_type = INTERFACE_TYPE_AGENT;
					break;
				case SVC_SNMPv1:
				case SVC_SNMPv2c:
				case SVC_SNMPv3:
					port = (unsigned short)atoi(row[4]);
					interface_type = INTERFACE_TYPE_SNMP;
					break;
				default:
					port = ZBX_DEFAULT_AGENT_PORT;
					interface_type = INTERFACE_TYPE_AGENT;
			}

			if (0 == hostid)
			{
				result2 = DBselect(
						"select distinct h.hostid"
						" from hosts h,interface i,dservices ds"
						" where h.hostid=i.hostid"
							" and i.ip=ds.ip"
							" and h.status in (%d,%d)"
							" and h.proxy_hostid%s"
							" and ds.dhostid=" ZBX_FS_UI64
						" order by h.hostid",
						HOST_STATUS_MONITORED, HOST_STATUS_NOT_MONITORED,
						DBsql_id_cmp(proxy_hostid), dhostid);

				if (NULL != (row2 = DBfetch(result2)))
					ZBX_STR2UINT64(hostid, row2[0]);

				DBfree_result(result2);
			}

			if (0 == hostid)
			{
				hostid = DBget_maxid("hosts");

				/* for host uniqueness purposes */
				host = zbx_strdup(host, '\0' != *row[3] ? row[3] : row[2]);

				make_hostname(host);	/* replace not-allowed symbols */
				host_unique = DBget_unique_hostname_by_sample(host);
				host_esc = DBdyn_escape_string(host_unique);

				zbx_free(host);

#ifdef HAVE_MYSQL
				/* MySQL: BLOB and TEXT columns doesn't have a default value; */
				/* we shall add them into an insert statement */
				DBexecute("insert into hosts (hostid,proxy_hostid,host,name,description)"
						" values (" ZBX_FS_UI64 ",%s,'%s','%s','')",
						hostid, DBsql_id_ins(proxy_hostid), host_esc, host_esc);
#else
				DBexecute("insert into hosts (hostid,proxy_hostid,host,name)"
						" values (" ZBX_FS_UI64 ",%s,'%s','%s')",
						hostid, DBsql_id_ins(proxy_hostid), host_esc, host_esc);
#endif

				DBadd_interface(hostid, interface_type, 1, row[2], row[3], port);

				zbx_free(host_unique);
				zbx_free(host_esc);

				add_discovered_host_groups(hostid, &groupids);
			}
			else
			{
				DBadd_interface(hostid, interface_type, 1, row[2], row[3], port);
			}
		}
		DBfree_result(result);
	}
	else if (EVENT_OBJECT_ZABBIX_ACTIVE == event->object)
	{
		result = DBselect(
				"select proxy_hostid,host,listen_ip,listen_dns,listen_port"
				" from autoreg_host"
				" where autoreg_hostid=" ZBX_FS_UI64,
				event->objectid);

		if (NULL != (row = DBfetch(result)))
		{
			char		*sql = NULL;
			zbx_uint64_t	host_proxy_hostid;

			ZBX_DBROW2UINT64(proxy_hostid, row[0]);
			host_esc = DBdyn_escape_string_len(row[1], HOST_HOST_LEN);
			port = (unsigned short)atoi(row[4]);

			result2 = DBselect(
					"select null"
					" from hosts"
					" where host='%s'"
						" and status=%d",
					host_esc, HOST_STATUS_TEMPLATE);

			if (NULL != (row2 = DBfetch(result2)))
			{
				zabbix_log(LOG_LEVEL_WARNING, "cannot add discovered host \"%s\":"
						" template with the same name already exists", row[1]);
				DBfree_result(result2);
				goto out;
			}
			DBfree_result(result2);

			sql = zbx_dsprintf(sql,
					"select hostid,proxy_hostid"
					" from hosts"
					" where host='%s'"
						" and flags<>%d"
						" and status in (%d,%d)"
					" order by hostid",
					host_esc, ZBX_FLAG_DISCOVERY_PROTOTYPE,
					HOST_STATUS_MONITORED, HOST_STATUS_NOT_MONITORED);

			result2 = DBselectN(sql, 1);

			zbx_free(sql);

			if (NULL == (row2 = DBfetch(result2)))
			{
				hostid = DBget_maxid("hosts");

#ifdef HAVE_MYSQL
				/* MySQL: BLOB and TEXT columns doesn't have a default value; */
				/* we shall add them into an insert statement */
				DBexecute("insert into hosts (hostid,proxy_hostid,host,name,description)"
						" values (" ZBX_FS_UI64 ",%s,'%s','%s','')",
						hostid, DBsql_id_ins(proxy_hostid), host_esc, host_esc);
#else
				DBexecute("insert into hosts (hostid,proxy_hostid,host,name)"
						" values (" ZBX_FS_UI64 ",%s,'%s','%s')",
						hostid, DBsql_id_ins(proxy_hostid), host_esc, host_esc);
#endif

				DBadd_interface(hostid, INTERFACE_TYPE_AGENT, 1, row[2], row[3], port);

				add_discovered_host_groups(hostid, &groupids);
			}
			else
			{
				ZBX_STR2UINT64(hostid, row2[0]);
				ZBX_DBROW2UINT64(host_proxy_hostid, row2[1]);

				if (host_proxy_hostid != proxy_hostid)
				{
					DBexecute("update hosts"
							" set proxy_hostid=%s"
							" where hostid=" ZBX_FS_UI64,
							DBsql_id_ins(proxy_hostid), hostid);
				}

				DBadd_interface(hostid, INTERFACE_TYPE_AGENT, 1, row[2], row[3], port);
			}
			DBfree_result(result2);
out:
			zbx_free(host_esc);
		}
		DBfree_result(result);
	}
clean:
	zbx_vector_uint64_destroy(&groupids);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);

	return hostid;
}

/******************************************************************************
 *                                                                            *
 * Function: op_host_add                                                      *
 *                                                                            *
 * Purpose: add discovered host                                               *
 *                                                                            *
 * Parameters: trigger - trigger data                                         *
 *             action  - action data                                          *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_host_add(const DB_EVENT *event)
{
	const char	*__function_name = "op_host_add";

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY && event->source != EVENT_SOURCE_AUTO_REGISTRATION)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE && event->object != EVENT_OBJECT_ZABBIX_ACTIVE)
		return;

	add_discovered_host(event);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_host_del                                                      *
 *                                                                            *
 * Purpose: delete host                                                       *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_host_del(const DB_EVENT *event)
{
	const char		*__function_name = "op_host_del";

	zbx_vector_uint64_t	hostids;
	zbx_uint64_t		hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE)
		return;

	if (0 == (hostid = select_discovered_host(event)))
		return;

	zbx_vector_uint64_create(&hostids);

	zbx_vector_uint64_append(&hostids, hostid);

	DBdelete_hosts_with_prototypes(&hostids);

	zbx_vector_uint64_destroy(&hostids);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_host_enable                                                   *
 *                                                                            *
 * Purpose: enable discovered                                                 *
 *                                                                            *
 * Author: Alexander Vladishev                                                *
 *                                                                            *
 ******************************************************************************/
void	op_host_enable(const DB_EVENT *event)
{
	const char	*__function_name = "op_host_enable";
	zbx_uint64_t	hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE)
		return;

	if (0 == (hostid = add_discovered_host(event)))
		return;

	DBexecute(
			"update hosts"
			" set status=%d"
			" where hostid=" ZBX_FS_UI64,
			HOST_STATUS_MONITORED,
			hostid);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_host_disable                                                  *
 *                                                                            *
 * Purpose: disable host                                                      *
 *                                                                            *
 * Author: Alexander Vladishev                                                *
 *                                                                            *
 ******************************************************************************/
void	op_host_disable(const DB_EVENT *event)
{
	const char	*__function_name = "op_host_disable";
	zbx_uint64_t	hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY && event->source != EVENT_SOURCE_AUTO_REGISTRATION)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE && event->object != EVENT_OBJECT_ZABBIX_ACTIVE)
		return;

	if (0 == (hostid = add_discovered_host(event)))
		return;

	DBexecute(
			"update hosts"
			" set status=%d"
			" where hostid=" ZBX_FS_UI64,
			HOST_STATUS_NOT_MONITORED,
			hostid);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_groups_add                                                    *
 *                                                                            *
 * Purpose: add groups to discovered host                                     *
 *                                                                            *
 * Parameters: event    - [IN] event data                                     *
 *             groupids - [IN] IDs of groups to add                           *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_groups_add(const DB_EVENT *event, zbx_vector_uint64_t *groupids)
{
	const char	*__function_name = "op_groups_add";
	zbx_uint64_t	hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY && event->source != EVENT_SOURCE_AUTO_REGISTRATION)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE && event->object != EVENT_OBJECT_ZABBIX_ACTIVE)
		return;

	if (0 == (hostid = add_discovered_host(event)))
		return;

	add_discovered_host_groups(hostid, groupids);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_groups_del                                                    *
 *                                                                            *
 * Purpose: delete groups from discovered host                                *
 *                                                                            *
 * Parameters: event    - [IN] event data                                     *
 *             groupids - [IN] IDs of groups to delete                        *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_groups_del(const DB_EVENT *event, zbx_vector_uint64_t *groupids)
{
	const char	*__function_name = "op_groups_del";

	DB_RESULT	result;
	zbx_uint64_t	hostid;
	char		*sql = NULL;
	size_t		sql_alloc = 256, sql_offset = 0;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE)
		return;

	if (0 == (hostid = select_discovered_host(event)))
		return;

	sql = zbx_malloc(sql, sql_alloc);

	/* make sure host belongs to at least one hostgroup */
	zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
			"select groupid"
			" from hosts_groups"
			" where hostid=" ZBX_FS_UI64
				" and not",
			hostid);
	DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "groupid", groupids->values, groupids->values_num);

	result = DBselectN(sql, 1);

	if (NULL == DBfetch(result))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot remove host \"%s\" from all host groups:"
				" it must belong to at least one", zbx_host_string(hostid));
	}
	else
	{
		sql_offset = 0;
		zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
				"delete from hosts_groups"
				" where hostid=" ZBX_FS_UI64
					" and",
				hostid);
		DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "groupid", groupids->values, groupids->values_num);

		DBexecute("%s", sql);
	}
	DBfree_result(result);

	zbx_free(sql);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_template_add                                                  *
 *                                                                            *
 * Purpose: link host with template                                           *
 *                                                                            *
 * Parameters: event           - [IN] event data                              *
 *             lnk_templateids - [IN] array of template IDs                   *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_template_add(const DB_EVENT *event, zbx_vector_uint64_t *lnk_templateids)
{
	const char	*__function_name = "op_template_add";
	zbx_uint64_t	hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY && event->source != EVENT_SOURCE_AUTO_REGISTRATION)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE && event->object != EVENT_OBJECT_ZABBIX_ACTIVE)
		return;

	if (0 == (hostid = add_discovered_host(event)))
		return;

	DBcopy_template_elements(hostid, lnk_templateids);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: op_template_del                                                  *
 *                                                                            *
 * Purpose: unlink and clear host from template                               *
 *                                                                            *
 * Parameters: event           - [IN] event data                              *
 *             lnk_templateids - [IN] array of template IDs                   *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 ******************************************************************************/
void	op_template_del(const DB_EVENT *event, zbx_vector_uint64_t *del_templateids)
{
	const char	*__function_name = "op_template_del";
	zbx_uint64_t	hostid;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (event->source != EVENT_SOURCE_DISCOVERY)
		return;

	if (event->object != EVENT_OBJECT_DHOST && event->object != EVENT_OBJECT_DSERVICE)
		return;

	if (0 == (hostid = select_discovered_host(event)))
		return;

	DBdelete_template_elements(hostid, del_templateids);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}
