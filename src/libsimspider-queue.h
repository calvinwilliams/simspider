/*
 * libsimspider-queue - Web Spider Engine Queue Library
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_LIBSIMSPIDER_QUEUE_
#define _H_LIBSIMSPIDER_QUEUE_

#include "libsimspider.h"

#include "memque.h"
#include "HashX.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIMSPIDER_DEFAULT_REQUESTQUEUE_SIZE		1*1024*1024

_WINDLL_FUNC int BindDefaultRequestQueueHandler( struct SimSpiderEnv *penv );
_WINDLL_FUNC void UnbindDefaultRequestQueueHandler( struct SimSpiderEnv *penv );

_WINDLL_FUNC int BindDefaultDoneQueueHandler( struct SimSpiderEnv *penv );
_WINDLL_FUNC void UnbindDefaultDoneQueueHandler( struct SimSpiderEnv *penv );

#ifdef __cplusplus
}
#endif

#endif

