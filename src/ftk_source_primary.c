/*
 * File: ftk_source_primary.c    
 * Author:  Li XianJing <xianjimli@hotmail.com>
 * Brief:   gui primary source.
 *
 * Copyright (c) 2009 - 2010  Li XianJing <xianjimli@hotmail.com>
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * History:
 * ================================================================
 * 2009-10-03 Li XianJing <xianjimli@hotmail.com> created
 * 2010-10-02 Jiao JinXing <jiaojinxing1987@gmail.com> add ftk_pipe_check for rt-thread support.
 *
 */

#include "ftk_pipe.h"
#include "ftk_globals.h"
#include "ftk_source_primary.h"
#include "ftk_sources_manager.h"

#define MAX_EVENTS 128

typedef struct _SourcePrimaryPrivInfo
{
	FtkPipe* pipe;
	FtkOnEvent on_event;
	void* user_data;
}PrivInfo;

static int ftk_source_primary_get_fd(FtkSource* thiz)
{
	DECL_PRIV(thiz, priv);
	return_val_if_fail(priv != NULL, -1);

	return ftk_pipe_get_read_handle(priv->pipe);
}

static int ftk_source_primary_check(FtkSource* thiz)
{
#ifndef RT_THREAD
	return -1;
#else
	DECL_PRIV(thiz, priv);
	return_val_if_fail(priv != NULL, -1);

	return ftk_pipe_check(priv->pipe);
#endif
}

static Ret ftk_source_primary_dispatch(FtkSource* thiz)
{
	FtkEvent event;
	DECL_PRIV(thiz, priv);
	int ret = ftk_pipe_read(priv->pipe, &event, sizeof(FtkEvent));
	return_val_if_fail(ret == sizeof(FtkEvent), RET_REMOVE);

	switch(event.type)
	{
		case FTK_EVT_NOP:
		{
			break;
		}
		case FTK_EVT_ADD_SOURCE:
		{
			ftk_sources_manager_add(ftk_default_sources_manager(), (FtkSource*)event.u.extra);
			break;
		}
		case FTK_EVT_REMOVE_SOURCE:
		{
			ftk_sources_manager_remove(ftk_default_sources_manager(), (FtkSource*)event.u.extra);
			break;
		}
		default:
		{
			if(priv->on_event != NULL)
			{
				priv->on_event(priv->user_data, &event);
			}
		}
	}

	return RET_OK;
}

static void ftk_source_primary_destroy(FtkSource* thiz)
{
	if(thiz != NULL)
	{
		DECL_PRIV(thiz, priv);
		ftk_pipe_destroy(priv->pipe);
		FTK_ZFREE(thiz, sizeof(FtkSource) + sizeof(PrivInfo));
	}

	return;
}

FtkSource* ftk_source_primary_create(FtkOnEvent on_event, void* user_data)
{
	FtkSource* thiz = (FtkSource*)FTK_ZALLOC(sizeof(FtkSource) + sizeof(PrivInfo));

	if(thiz != NULL)
	{
		DECL_PRIV(thiz, priv);
		
		thiz->get_fd   = ftk_source_primary_get_fd;
		thiz->check    = ftk_source_primary_check;
		thiz->dispatch = ftk_source_primary_dispatch;
		thiz->destroy  = ftk_source_primary_destroy;

		thiz->ref = 1;
		priv->pipe = ftk_pipe_create();
		priv->on_event  = on_event;
		priv->user_data = user_data;
	}

	return thiz;
}

Ret ftk_source_queue_event(FtkSource* thiz, FtkEvent* event)
{
	int ret = 0;
	DECL_PRIV(thiz, priv);
	return_val_if_fail(thiz != NULL && event != NULL, RET_FAIL);

	ret = ftk_pipe_write(priv->pipe, event, sizeof(FtkEvent));

	return ret == sizeof(FtkEvent) ? RET_OK : RET_FAIL;
}

