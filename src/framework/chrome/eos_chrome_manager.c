/**
 * @file eos_chrome_manager.c
 * @brief System chrome manager implementation
 */

#include "eos_chrome_manager.h"

/* Includes ---------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EOS_LOG_TAG "ChromeMgr"
#include "eos_log.h"
#include "eos_mem.h"
#include "eos_control_center.h"
#include "eos_msg_list.h"
#include "eos_flash_light.h"
#include "eos_activity.h"
#include "eos_app_list.h"
#include "eos_service_pm.h"
#include "eos_stack.h"

/* Macros and Definitions -------------------------------------*/

#define _MAX_OVERLAYS 16

/* Variables --------------------------------------------------*/
static const eos_chrome_overlay_t *_overlays[_MAX_OVERLAYS];
static uint32_t _overlay_count = 0;
static eos_stack_t *_overlay_stack = NULL;

/* Function Implementations -----------------------------------*/

void eos_chrome_manager_register_overlay(const eos_chrome_overlay_t *overlay)
{
    if (!overlay || !overlay->is_open || !overlay->pull_back || !overlay->hide)
    {
        EOS_LOG_E("Invalid overlay registration");
        return;
    }

    if (_overlay_count >= _MAX_OVERLAYS)
    {
        EOS_LOG_E("Max overlays reached (%d)", _MAX_OVERLAYS);
        return;
    }

    _overlays[_overlay_count++] = overlay;
    EOS_LOG_I("Overlay registered: %s (total=%d)", overlay->name, _overlay_count);
}

static void _sync_stack_state(void)
{
    if (!_overlay_stack)
        return;

    const eos_chrome_overlay_t *top;
    while ((top = (const eos_chrome_overlay_t *)eos_stack_peek(_overlay_stack)) != NULL)
    {
        if (!top->is_open())
        {
            EOS_LOG_I("Sync stack: removing closed overlay %s", top->name);
            eos_stack_pop(_overlay_stack);
        }
        else
        {
            break;
        }
    }
}

bool eos_chrome_manager_any_overlay_open(void)
{
    _sync_stack_state();
    return eos_stack_get_size(_overlay_stack) > 0;
}

const eos_chrome_overlay_t *eos_chrome_manager_get_top_overlay(void)
{
    _sync_stack_state();
    return (const eos_chrome_overlay_t *)eos_stack_peek(_overlay_stack);
}

void eos_chrome_manager_pull_back_top(void)
{
    _sync_stack_state();

    const eos_chrome_overlay_t *top = (const eos_chrome_overlay_t *)eos_stack_peek(_overlay_stack);
    if (top)
    {
        EOS_LOG_I("Pulling back top overlay: %s", top->name);
        top->pull_back();
    }
}

void eos_chrome_manager_pull_back_all(void)
{
    _sync_stack_state();

    const eos_chrome_overlay_t *overlay;
    while ((overlay = (const eos_chrome_overlay_t *)eos_stack_pop(_overlay_stack)) != NULL)
    {
        if (overlay->is_open())
        {
            EOS_LOG_I("Pulling back overlay: %s", overlay->name);
            overlay->pull_back();
        }
    }
}

void eos_chrome_manager_push_overlay(const eos_chrome_overlay_t *overlay)
{
    if (!overlay)
        return;

    _sync_stack_state();

    const eos_chrome_overlay_t *top = (const eos_chrome_overlay_t *)eos_stack_peek(_overlay_stack);
    if (top == overlay)
    {
        EOS_LOG_W("Overlay %s is already on top of stack", overlay->name);
        return;
    }

    EOS_LOG_I("Pushing overlay to stack: %s", overlay->name);
    eos_stack_push(_overlay_stack, (void *)overlay);
}

void eos_chrome_manager_handle_crown_click(void)
{
    if (eos_pm_get_state() == EOS_PM_SLEEP)
    {
        eos_pm_wake_up();
        return;
    }

    eos_pm_reset_timer();

    if (eos_chrome_manager_any_overlay_open())
    {
        eos_chrome_manager_pull_back_top();
        return;
    }

    bool from_watchface = (eos_activity_get_current() == eos_activity_get_watchface());

    if (from_watchface)
    {
        eos_app_list_enter();
    }
    else
    {
        eos_activity_back();
    }
}

void eos_chrome_manager_handle_activity_switch(void)
{
    eos_chrome_manager_pull_back_all();

    for (uint32_t i = 0; i < _overlay_count; i++)
    {
        _overlays[i]->hide();
    }
}

/************************** Built-in Overlay Wrappers **************************/

static bool _cc_is_open(void)
{
    eos_control_center_t *cc = eos_control_center_get_instance();
    if (!cc || !cc->swipe_panel || !cc->swipe_panel->sw)
        return false;
    return cc->swipe_panel->sw->state == EOS_SLIDE_WIDGET_STATE_OPEN;
}

static void _cc_pull_back(void)
{
    eos_control_center_t *cc = eos_control_center_get_instance();
    if (cc && cc->swipe_panel)
    {
        eos_swipe_panel_pull_back(cc->swipe_panel);
    }
}

static void _cc_hide(void)
{
    eos_control_center_hide();
}

static void _cc_on_hidden(void)
{
}

static bool _msg_list_is_open(void)
{
    eos_msg_list_t *msg_list = eos_msg_list_get_instance();
    if (!msg_list || !msg_list->swipe_panel || !msg_list->swipe_panel->sw)
        return false;
    return msg_list->swipe_panel->sw->state == EOS_SLIDE_WIDGET_STATE_OPEN;
}

static void _msg_list_pull_back(void)
{
    eos_msg_list_close_detail();
    eos_msg_list_t *msg_list = eos_msg_list_get_instance();
    if (msg_list && msg_list->swipe_panel)
    {
        eos_swipe_panel_pull_back(msg_list->swipe_panel);
    }
}

static void _msg_list_hide(void)
{
    eos_msg_list_close_detail();
    eos_msg_list_hide();
}

static void _msg_list_on_hidden(void)
{
}

static bool _flash_light_is_open(void)
{
    return eos_flash_light_is_open();
}

static void _flash_light_pull_back(void)
{
    eos_flash_light_pull_back();
}

static void _flash_light_hide(void)
{
    eos_flash_light_hide();
}

static void _flash_light_on_hidden(void)
{
}

static const eos_chrome_overlay_t _cc_overlay = {
    .is_open = _cc_is_open,
    .pull_back = _cc_pull_back,
    .hide = _cc_hide,
    .on_hidden = _cc_on_hidden,
    .name = "control_center",
};

static const eos_chrome_overlay_t _msg_list_overlay = {
    .is_open = _msg_list_is_open,
    .pull_back = _msg_list_pull_back,
    .hide = _msg_list_hide,
    .on_hidden = _msg_list_on_hidden,
    .name = "msg_list",
};

static const eos_chrome_overlay_t _flash_light_overlay = {
    .is_open = _flash_light_is_open,
    .pull_back = _flash_light_pull_back,
    .hide = _flash_light_hide,
    .on_hidden = _flash_light_on_hidden,
    .name = "flash_light",
};

void eos_chrome_manager_init(void)
{
    _overlay_count = 0;

    if (_overlay_stack)
        eos_stack_destroy(_overlay_stack);
    _overlay_stack = eos_stack_create(4);

    eos_chrome_manager_register_overlay(&_cc_overlay);
    eos_chrome_manager_register_overlay(&_msg_list_overlay);
    eos_chrome_manager_register_overlay(&_flash_light_overlay);
    EOS_LOG_I("Chrome manager initialized");
}

/* Helper functions for overlays to notify opening */
void eos_chrome_manager_notify_overlay_opened(const char *name)
{
    for (uint32_t i = 0; i < _overlay_count; i++)
    {
        if (strcmp(_overlays[i]->name, name) == 0)
        {
            eos_chrome_manager_push_overlay(_overlays[i]);
            return;
        }
    }
    EOS_LOG_W("Overlay not found: %s", name);
}
