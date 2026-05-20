/**
 * @file script_engine_manager.c
 * @brief Script engine manager - high-level semantic operations
 */

#include "script_engine_manager.h"

/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#define EOS_LOG_TAG "ScriptEngineMgr"
#include "eos_log.h"
#include "eos_mem.h"

/* Static Variables ------------------------------------------*/

static script_runtime_context_t *_wf_context = NULL;

/* Function Implementations -----------------------------------*/

script_engine_result_t script_engine_watchface_start(const script_pkg_t *pkg)
{
    if (!pkg || !pkg->script_str) {
        return -SE_ERR_NULL_PACKAGE;
    }

    // Reset the context handle before stopping the engine.
    // If the engine is SUSPENDED, script_engine_stop() will clean up
    // all saved contexts including the one _wf_context pointed to.
    _wf_context = NULL;

    // Ensure engine is in a clean state
    script_engine_stop();

    // Create a new context handle
    _wf_context = script_runtime_context_create();
    if (!_wf_context) {
        EOS_LOG_E("watchface_start: failed to create runtime context");
        return -SE_ERR_MALLOC;
    }

    _wf_context->owner_type = SCRIPT_TYPE_WATCHFACE;
    if (pkg->id) {
        snprintf(_wf_context->owner_id, sizeof(_wf_context->owner_id), "%s", pkg->id);
    }

    return script_engine_run(pkg);
}

script_engine_result_t script_engine_watchface_pause(void)
{
    if (!_wf_context) {
        EOS_LOG_E("watchface_pause: no active watchface context");
        return -SE_ERR_INVALID_STATE;
    }

    script_state_t state = script_engine_get_state();
    EOS_LOG_I("watchface_pause: current engine state=%d", state);

    // If the engine is in IDLE state (main script done), save the context
    if (state == SCRIPT_STATE_IDLE) {
        return script_runtime_context_save(_wf_context);
    }

    // If already SUSPENDED, no-op (already paused)
    if (state == SCRIPT_STATE_SUSPENDED) {
        EOS_LOG_D("watchface_pause: already paused");
        return SE_OK;
    }

    // If the engine is in RUNNING state (script still executing main),
    // wait... actually we shouldn't be pausing during main execution.
    // For now, request stop and save.
    if (state == SCRIPT_STATE_RUNNING) {
        EOS_LOG_W("watchface_pause: engine still RUNNING, requesting stop first");
        script_engine_result_t ret = script_engine_request_stop();
        if (ret != SE_OK) {
            EOS_LOG_E("watchface_pause: failed to request stop");
            return ret;
        }
        // After stop, engine should be in STOPPED state
        // Save context (but realm is gone, so this just records metadata)
        return script_runtime_context_save(_wf_context);
    }

    // Any other state (STOPPED, ERROR) -> can't save meaningful context
    EOS_LOG_W("watchface_pause: cannot pause from state %d", state);
    return -SE_ERR_INVALID_STATE;
}

script_engine_result_t script_engine_watchface_resume(void)
{
    if (!_wf_context) {
        EOS_LOG_E("watchface_resume: no active watchface context");
        return -SE_ERR_NO_SAVED_CONTEXT;
    }

    script_state_t state = script_engine_get_state();
    EOS_LOG_I("watchface_resume: current engine state=%d", state);

    if (state == SCRIPT_STATE_SUSPENDED) {
        return script_runtime_context_restore(_wf_context);
    }

    if (state == SCRIPT_STATE_STOPPED && _wf_context->is_valid) {
        // App finished, engine is STOPPED but watchface context is still
        // saved (realm preserved). Transition to SUSPENDED then restore.
        script_engine_set_script_state(SCRIPT_STATE_SUSPENDED);
        return script_runtime_context_restore(_wf_context);
    }

    if (state == SCRIPT_STATE_IDLE) {
        // Already running, no-op
        EOS_LOG_D("watchface_resume: already idle");
        return SE_OK;
    }

    EOS_LOG_W("watchface_resume: unexpected state %d", state);
    return -SE_ERR_INVALID_STATE;
}

script_engine_result_t script_engine_watchface_destroy(void)
{
    EOS_LOG_I("watchface_destroy: cleaning up watchface context");

    // Stop the engine — this calls _context_list_destroy_all() which frees
    // all saved contexts (including g_wf_context) from the linked list.
    script_engine_stop();

    // Simply NULL our pointer; the context struct was already freed.
    _wf_context = NULL;

    return SE_OK;
}

bool script_engine_watchface_has_context(void)
{
    return _wf_context != NULL;
}


script_engine_result_t script_engine_app_run(const script_pkg_t *pkg)
{
    if (!pkg || !pkg->script_str) {
        return -SE_ERR_NULL_PACKAGE;
    }

    return script_engine_run(pkg);
}

script_engine_result_t script_engine_app_stop(void)
{
    return script_engine_stop();
}
