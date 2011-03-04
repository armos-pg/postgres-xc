/*-------------------------------------------------------------------------
 *
 * gtm.h
 *
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2010-2011 Nippon Telegraph and Telephone Corporation
 *
 * src/include/gtm/gtm.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _GTM_H
#define _GTM_H

#include <setjmp.h>

#include "gtm/gtm_c.h"
#include "gtm/palloc.h"
#include "gtm/gtm_lock.h"
#include "gtm/gtm_conn.h"
#include "gtm/elog.h"
#include "gtm/gtm_list.h"

extern char *GTMLogFile;

typedef enum GTM_ThreadStatus
{
	GTM_THREAD_STARTING,
	GTM_THREAD_RUNNING,
	GTM_THREAD_EXITING,
	/* Must be the last */
	GTM_THREAD_INVALID
} GTM_ThreadStatus;

struct GTM_ConnectionInfo;

#define ERRORDATA_STACK_SIZE  5

typedef struct GTM_ThreadInfo
{
	/*
	 * Thread specific information such as connection(s) served by it
	 */
	GTM_ThreadID		thr_id;
	uint32				thr_localid;
	void * (* thr_startroutine)(void *);
	
	MemoryContext	thr_thread_context;
	MemoryContext	thr_message_context;	
	MemoryContext	thr_current_context;
	MemoryContext	thr_error_context;
	MemoryContext	thr_parent_context;

	sigjmp_buf		*thr_sigjmp_buf;

	ErrorData		thr_error_data[ERRORDATA_STACK_SIZE];
	int				thr_error_stack_depth;
	int				thr_error_recursion_depth;
	int				thr_criticalsec_count;

	GTM_ThreadStatus	thr_status;
	GTM_ConnectionInfo	*thr_conn;

	GTM_RWLock			thr_lock;
	List				*thr_cached_txninfo;

} GTM_ThreadInfo;

typedef struct GTM_Threads
{
	uint32				gt_thread_count;
	uint32				gt_array_size;
	GTM_ThreadInfo		**gt_threads;
	GTM_RWLock			gt_lock;
} GTM_Threads;

extern GTM_Threads *GTMThreads;

int GTM_ThreadAdd(GTM_ThreadInfo *thrinfo);
int GTM_ThreadRemove(GTM_ThreadInfo *thrinfo);
int GTM_ThreadJoin(GTM_ThreadInfo *thrinfo);
void GTM_ThreadExit(void);
void ConnFree(Port *port);

GTM_ThreadInfo *GTM_ThreadCreate(GTM_ConnectionInfo *conninfo,
				  void *(* startroutine)(void *));
GTM_ThreadInfo * GTM_GetThreadInfo(GTM_ThreadID thrid);

/*
 * pthread keys to get thread specific information
 */
extern pthread_key_t					threadinfo_key;
extern MemoryContext					TopMostMemoryContext;
extern GTM_ThreadID						TopMostThreadID;

#define SetMyThreadInfo(thrinfo)		pthread_setspecific(threadinfo_key, (thrinfo))
#define GetMyThreadInfo					((GTM_ThreadInfo *)pthread_getspecific(threadinfo_key))

#define TopMemoryContext		(GetMyThreadInfo->thr_thread_context)
#define ThreadTopContext		(GetMyThreadInfo->thr_thread_context)
#define MessageContext			(GetMyThreadInfo->thr_message_context)
#define CurrentMemoryContext	(GetMyThreadInfo->thr_current_context)
#define ErrorContext			(GetMyThreadInfo->thr_error_context)
#define errordata				(GetMyThreadInfo->thr_error_data)
#define recursion_depth			(GetMyThreadInfo->thr_error_recursion_depth)
#define errordata_stack_depth	(GetMyThreadInfo->thr_error_stack_depth)
#define CritSectionCount		(GetMyThreadInfo->thr_criticalsec_count)

#define PG_exception_stack		(GetMyThreadInfo->thr_sigjmp_buf)
#define MyConnection			(GetMyThreadInfo->thr_conn)
#define MyPort					((GetMyThreadInfo->thr_conn != NULL) ?	\
									GetMyThreadInfo->thr_conn->con_port :	\
									NULL)
#define MyThreadID				(GetMyThreadInfo->thr_id)
#define IsMainThread()			(GetMyThreadInfo->thr_id == TopMostThreadID)

#define GTM_CachedTransInfo				(GetMyThreadInfo->thr_cached_txninfo)
#define GTM_HaveFreeCachedTransInfo()	(list_length(GTM_CachedTransInfo))

#define GTM_MAX_CACHED_TRANSINFO		0
#define GTM_HaveEnoughCachedTransInfo()	(list_length(GTM_CachedTransInfo) >= GTM_MAX_CACHED_TRANSINFO)

#define START_CRIT_SECTION()  (CritSectionCount++)

#define END_CRIT_SECTION() \
	do { \
		    Assert(CritSectionCount > 0); \
		    CritSectionCount--; \
	} while(0)


#if 0

/* Coordinator registration */
int GTM_RegisterCoordinator(GTM_CoordInfo *cinfo);
int GTM_UnregisterCoordinator(GTM_PGXCNodeId cid);

#endif

#endif
